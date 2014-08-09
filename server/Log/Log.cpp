#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread.hpp>
#include "internal.h"
#include "LogLevel.h"
#include "SimpleIni.h"
#include "LogFile.h"
#include <sstream>
#include <vector>
#include <memory>
#ifndef _LOG_DLL_
#include "NetLib/NetLib.h"
#endif
#include "to_LS_ClientDelegate.h"
#include "Counter.h"
#include "ToAbsolutePath.h"
#include "../protobuf/src-gen/cpp/log/log.pb.h"
#include "HeaderDefine.h"
#include "Log.h"

ioservice_thread __log_ioservice_th;

std::string __LS_ServerTypeNameForQuery = "ls";
std::string __LS_ServerTypeName = "ls";

boost::shared_mutex _log_shared_mutex;

std::string FileNamePrefix;

bool UserDefinedFilenamePrefix = false;

//没有调用LogInitializeLocalOptions前的默认值
bool EnableToConsole = true;
bool EnableToFile = true;
bool SplitByDate = true;
int LogTotalMegaBytesLimitWithinDir = 32 * 1024;
int LogTotalFileLimitWithinDir = 200;

unsigned global_LogLevel = LOGLV_INFO;
#ifndef _LOG_DLL_
unsigned global_CountInterval = 24 * 3600;
#else
unsigned global_CountInterval = 0;
#endif

unsigned global_EnableDetail = 1;
std::string global_index1, global_index2; //index1为空时，写往文件和网络的Log会暂存

std::map<std::string, LogOption> options_map;

LogFile file;

#ifndef _LOG_DLL_

void SetLSServerTypeNameForQuery(const char* LS_ServerTypeNameForQuery)
{
	__LS_ServerTypeNameForQuery = LS_ServerTypeNameForQuery;
}

void SetLSServerTypeName(const char* LS_ServerTypeName)
{
	__LS_ServerTypeName = LS_ServerTypeName;
	__LS_ServerTypeNameForQuery = LS_ServerTypeName;
}

void _InitializeLogNetDelegate() //TODO: 带ioservice_thread参数的方法
{
	NetLibPlus_InitializeClients<to_LS_ClientsDelegate>(__LS_ServerTypeName.c_str());
	if (__LS_ServerTypeNameForQuery != __LS_ServerTypeName)
	{
		NetLibPlus_InitializeClients<to_LS_ClientsDelegate>(__LS_ServerTypeNameForQuery.c_str());
	}
};

#else

#define RAW_UDP_MTU_ (1550)
char receive_buffer[RAW_UDP_MTU_];
boost::asio::ip::udp::endpoint udp_remote_endpoint_when_receive;

bool udp_listenning = true;

uint16_t udp_current_trans_id = 0;

void raw_udp_async_receive();

void raw_udp_receive_packet(const boost::system::error_code& error, std::size_t bytes_transferred)
{	
	if (udp_listenning) //只在UnInitialize的时候置为false。这个变量暂时没有用锁保护，应该没事。
	{
		if (!error) //有错忽略，依然继续收
		{
			if (bytes_transferred >= 4) //包头4字节，头两字节是命令，后两字节是trans_id
			{
				//返回的包暂时只可能是ID_LOGSVR_LOGOPTIONS。别的包也是完全有可能的，说不定别人正好突然发进来
				if (*(uint16_t*)receive_buffer == ID_LOGSVR_LOGOPTIONS && *(uint16_t*)(receive_buffer + 2) == udp_current_trans_id) //得trans_id匹配了才进，否则可能是前一次SetIndex12返回的结果
				{
					if (ProcessLogOptions(receive_buffer + 4, bytes_transferred - 4))
					{
						//LOGEVENTL("Log_Debug", "Receive LogOptions");
					}
				}
			}
		}
			
		boost::lock_guard<boost::mutex> lock(_udpsocket_mutex);
		raw_udp_async_receive();
	}
}

void raw_udp_async_receive() //要锁内调用
{
	udpsocket_4log->async_receive_from(boost::asio::buffer(receive_buffer, RAW_UDP_MTU_), udp_remote_endpoint_when_receive,
		boost::bind(&raw_udp_receive_packet, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void SetLogServerAddress(const char* IP, int port)
{	
	boost::lock_guard<boost::mutex> lock(_udpsocket_mutex);
	udpsocket_4log = std::make_shared<boost::asio::ip::udp::socket>(__log_ioservice_th.get_ioservice());
	
	boost::system::error_code error;
	udpsocket_4log->open(boost::asio::ip::udp::v4(), error);
	if (error)
	{
		LOGEVENTL("Log_Error", "raw udp open error: " << error.value());
	}

	boost::system::error_code error2;
	udpsocket_4log->bind(boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0), error2);
	if (error2)
	{
		LOGEVENTL("Log_Error", "udp bind error: " << error2.value());
	}

	udpendpoint_4log.address(boost::asio::ip::address::from_string(IP));
	udpendpoint_4log.port(port);

	raw_udp_async_receive();
}

#endif

void SendLoginToLS() 
{	
	_log_shared_mutex.lock_shared();

	kit::log::Login pb;
	pb.set_index1(global_index1);
	pb.set_index2(global_index2);

	_log_shared_mutex.unlock_shared();

	int pb_size = pb.ByteSize();

#ifndef _LOG_DLL_
	
	char* send_buf = new char[CMDEX0_HEAD_SIZE + pb_size];
	CMD_SIZE(send_buf)			= CMDEX0_HEAD_SIZE + pb_size;					// 数据包的字节数（含msghead）
	CMD_FLAG(send_buf)			= 0;							// 标志位，表示数据是否压缩、加密等
	CMD_CAT(send_buf)			= CAT_LOGSVR;				// 命令分类
	CMD_ID(send_buf)			= ID_LOGSVR_LOGIN;

	pb.SerializeWithCachedSizesToArray((google::protobuf::uint8*)CMDEX0_DATA(send_buf));

	//TODO: 应该给所有ls发
	std::shared_ptr<NetLibPlus_Client> client = NetLibPlus_get_first_Client(__LS_ServerTypeName.c_str()); //LogOptions只对写Log有效，所以不用对ServerTypeForQuery发
	if (client) 
	{
		client->SendAsync(send_buf);
	}
	else
	{
		delete[] send_buf;
	}

#else
	
	if (udp_listenning && udpsocket_4log) //还要判一下udp_listenning是因为针对刚一启动就退的情况，这时候udpsocket_4log还在
	{
		char* send_buf = new char[4 + pb_size];
		*(uint16_t*)send_buf = ID_LOGSVR_LOGIN;
		*(uint16_t*)(send_buf + 2) = ++udp_current_trans_id; //这个变量也受ioservice的保护，几个访问者都只可能在ioservice中执行

		pb.SerializeWithCachedSizesToArray((google::protobuf::uint8*)(send_buf + 4));

		boost::system::error_code error;
		boost::lock_guard<boost::mutex> lock(_udpsocket_mutex);

		udpsocket_4log->send_to(boost::asio::buffer(send_buf, 4 + pb_size), udpendpoint_4log, 0, error);

		delete[] send_buf;
	}
	
#endif
}

void LogInitializeLocalOptions(bool enable_to_console, bool enable_to_file, const char* filename_prefix, bool split_by_date, int log_total_mega_bytes_limit_within_dir, int log_total_file_limit_within_dir, const char* config_file_path)
{
	boost::lock_guard<boost::shared_mutex> lock(_log_shared_mutex);

	FileNamePrefix = filename_prefix;
	EnableToConsole = enable_to_console;
	EnableToFile = enable_to_file;
	SplitByDate = split_by_date;
	LogTotalMegaBytesLimitWithinDir = log_total_mega_bytes_limit_within_dir;
	LogTotalFileLimitWithinDir = log_total_file_limit_within_dir;

	CSimpleIni ini;
	if (ini.LoadFile(utility3::ToAbsolutePath(config_file_path).c_str()) == SI_OK)
	{
		FileNamePrefix = ini.GetValue("Log", "FileNamePrefix", filename_prefix);
		EnableToConsole = ini.GetBoolValue("Log", "EnableConsole", EnableToConsole);
		EnableToFile = ini.GetBoolValue("Log", "EnableFile", EnableToFile);
		SplitByDate = ini.GetBoolValue("Log", "SplitByDate", SplitByDate);
		LogTotalMegaBytesLimitWithinDir = ini.GetLongValue("Log", "LogTotalMegaBytesLimitWithinDir", LogTotalMegaBytesLimitWithinDir);
		LogTotalFileLimitWithinDir = ini.GetLongValue("Log", "LogTotalFileLimitWithinDir", LogTotalFileLimitWithinDir);
	}
	
	if (FileNamePrefix.empty())
	{
		UserDefinedFilenamePrefix = false;
	}
	else
	{
		UserDefinedFilenamePrefix = true;
		FileNamePrefix = utility3::ToAbsolutePath(FileNamePrefix);
	}

	file.CloseIfNeeded(); //将旧的Log文件关了，因为模块名改了，接下来的第一条log会开文件的。
}

void LogSetIdentifier(const char* ServerType, int ServerID)
{
	if (strlen(ServerType) == 0)
	{
		printf("[Log_Error] LogSetIdentifier: can't accept an empty string as ServerType.\n");
		return;
	}

	std::ostringstream os;
	os << ServerID;
	LogSetIndex12(ServerType, os.str().c_str());
}

void LogSetIndex12(const char* index1, const char* index2)
{
	{
		boost::lock_guard<boost::shared_mutex> lock(_log_shared_mutex);
		
		if (strlen(index1) == 0)
		{
			printf("[Log_Error] LogSetIndex12: can't accept an empty string as index1.\n");
			return;
		}

		global_index1 = index1;
		global_index2 = index2;

		if (! UserDefinedFilenamePrefix) //如果用户没有在外部设置文件名前缀，则用global_index1和global_index2拼装一个
		{
			if (global_index2.empty()) //默认值，因此直接不把index2放文件名里
			{
				FileNamePrefix = global_index1;
			}
			else
			{
				FileNamePrefix = global_index1 + "_" + global_index2;
			}

			FileNamePrefix = utility3::ToAbsolutePath(FileNamePrefix);
			
			file.CloseIfNeeded(); //将旧的Log文件关了，因为拼装版的FileNamePrefix改了，接下来的第一条log会开文件的。	
		}

		__log_ioservice_th.start();	//重复start没关系
	}

	__log_ioservice_th.get_ioservice().post(boost::bind(&SendLoginToLS));
}

void LogUnInitialize()
{
	LogStopAllCounters();

#ifndef _LOG_DLL_
	NetLibPlus_UnInitializeClients(__LS_ServerTypeName.c_str());
	if (__LS_ServerTypeNameForQuery != __LS_ServerTypeName)
	{
		NetLibPlus_UnInitializeClients(__LS_ServerTypeNameForQuery.c_str());
	}
#else
	
	udp_listenning = false;

	boost::system::error_code ignored_error;
	if (udpsocket_4log)
	{
		udpsocket_4log->shutdown(boost::asio::socket_base::shutdown_both, ignored_error);
		udpsocket_4log->close(ignored_error);
	}

#endif
	
	__log_ioservice_th.stop_when_no_work(); //这句不能在LogStopAllCounters之前执行
	__log_ioservice_th.wait_for_stop(); //下列东西使用了ioservice: counter's timer / udpsocket_4log(DLL版本) / udp_login_timer(DLL版本) / SendLoginToLS
	
	//等wait_for_stop之后再释放各对象

#ifdef _LOG_DLL_
	udpsocket_4log.reset(); //这一行如果不加，用这个dll的exe就会退不出
	
	google::protobuf::ShutdownProtobufLibrary();
#endif
}

#ifdef _LOG_DLL_

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#endif
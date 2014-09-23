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
#include "NetLib/NetLib.h"
#include "to_LS_ClientDelegate.h"
#include "Counter.h"
#include "ToAbsolutePath.h"
#include "../protobuf/log/log.pb.h"
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
unsigned global_CountInterval = 24 * 3600;

unsigned global_EnableDetail = 1;
std::string global_index1, global_index2; //index1为空时，写往文件和网络的Log会暂存

std::map<std::string, LogOption> options_map;

LogFile file;

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
	//TODO TOMODIFY
	/*
	NetLibPlus_InitializeClients<to_LS_ClientsDelegate>(__LS_ServerTypeName.c_str());
	if (__LS_ServerTypeNameForQuery != __LS_ServerTypeName)
	{
		NetLibPlus_InitializeClients<to_LS_ClientsDelegate>(__LS_ServerTypeNameForQuery.c_str());
	}
	*/
};

void SendLoginToLS() 
{	
	_log_shared_mutex.lock_shared();

	kit::log::Login pb;
	pb.set_index1(global_index1);
	pb.set_index2(global_index2);

	_log_shared_mutex.unlock_shared();

	int pb_size = pb.ByteSize();
		
	char* send_buf = new char[CMDEX0_HEAD_SIZE + pb_size];
	CMD_SIZE(send_buf)			= CMDEX0_HEAD_SIZE + pb_size;					// 数据包的字节数（含msghead）
	CMD_FLAG(send_buf)			= 0;							// 标志位，表示数据是否压缩、加密等
	CMD_CAT(send_buf)			= CAT_LOGSVR;				// 命令分类
	CMD_ID(send_buf)			= ID_LOGSVR_LOGIN;

	pb.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)CMDEX0_DATA(send_buf));

	//TODO TOMODIFY

	//TODO: 应该给所有ls发
	/*
	std::shared_ptr<NetLibPlus_Client> client = NetLibPlus_get_first_Client(__LS_ServerTypeName.c_str()); //LogOptions只对写Log有效，所以不用对ServerTypeForQuery发
	if (client) 
	{
		client->SendAsync(send_buf);
	}
	else
	{
		delete[] send_buf;
	}
	*/
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

	//TODO TOMODIFY
	/*
	NetLibPlus_UnInitializeClients(__LS_ServerTypeName.c_str());
	if (__LS_ServerTypeNameForQuery != __LS_ServerTypeName)
	{
		NetLibPlus_UnInitializeClients(__LS_ServerTypeNameForQuery.c_str());
	}
	*/

	__log_ioservice_th.stop_when_no_work(); //这句不能在LogStopAllCounters之前执行
	__log_ioservice_th.wait_for_stop(); //下列东西使用了ioservice: counter's timer / udpsocket_4log(DLL版本) / udp_login_timer(DLL版本) / SendLoginToLS
	
	//等wait_for_stop之后再释放各对象
}
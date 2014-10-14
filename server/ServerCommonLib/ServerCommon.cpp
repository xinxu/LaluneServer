#include "NetLib.h"
#include "include/ToAbsolutePath.h"
#include "ServerCommon.h"
#include "include/SimpleIni.h"
#include "include/utility1.h"
#include "include/ioservice_thread.h"
#include "Log/Log.h"
#include <map>
#include <boost/asio.hpp>

CommonLibDelegate* __commonlib_delegate;
int __my_listening_port, __my_server_type, __my_server_id = -1;
NetLib_Client_ptr __conn2controlserver;

std::string __control_server_ip = CONTROL_SERVER_DEFAULT_IP; //默认值
int __control_server_port = CONTROL_SERVER_DEFAULT_PORT;

bool is_config_initialized = false;
bool is_addr_info_initialized = false;

void SayHello2ControlServer(int is_server_start = 1)
{
	common::Hello hello;
	hello.set_my_listening_port(__my_listening_port);
	hello.set_server_type(__my_server_type);
	hello.set_is_server_start(1);
	SendMsg(CONTROL_SERVER_ID, MSG_TYPE_CONTROL_SERVER_SAY_HELLO, hello);
}

class Conn2ControlServerDelegate : public NetLibPlus_Client_Delegate
{
public:
	void ReconnectedHandler(std::shared_ptr<NetLibPlus_Client> clientptr)
	{
		SayHello2ControlServer(0);
	}

	void RecvFinishHandler(std::shared_ptr<NetLibPlus_Client> clientptr, char* data)
	{
		if (SERVER_MSG_LENGTH(data) >= SERVER_MSG_HEADER_BASE_SIZE)
		{
			switch (SERVER_MSG_TYPE(data))
			{
			case MSG_TYPE_CONTROL_SERVER_SAY_HELLO_RESULT:
				{
					common::HelloResult result;
					if (ParseMsg(data, result))
					{
						__my_server_id = result.server_id();
					}
				}
				break;
			case MSG_TYPE_CONTROL_SERVER_ADDR_INFO_REFRESH:
				{
					common::AddressList list;
					if (ParseMsg(data, list))
					{
						LOGEVENTL("DEBUG", "MSG_TYPE_CONTROL_SERVER_ADDR_INFO_REFRESH");
						for (int i = 0; i < list.addr_size(); ++i)
						{
							const common::AddressInfo& addr = list.addr(i);
							if (addr.server_id() != __my_server_id)
							{
								_NetLibPlus_UpdateServerInfo(addr.server_id(), addr.ip(), addr.port(), addr.server_type());
								__commonlib_delegate->onServerAdded(addr.server_type(), addr.server_id());
							}
						}
						if (!is_addr_info_initialized)
						{
							is_addr_info_initialized = true;
							__commonlib_delegate->onAddrInitialized();
						}
					}
				}
				break;
			case MSG_TYPE_CONTROL_SERVER_ADDR_INFO_ADD:
				{
					common::AddressInfo addr;
					if (ParseMsg(data, addr))
					{
						if (addr.server_id() != __my_server_id)
						{
							LOGEVENTL("DEBUG", "MSG_TYPE_CONTROL_SERVER_ADDR_INFO_ADD");

							_NetLibPlus_UpdateServerInfo(addr.server_id(), addr.ip(), addr.port(), addr.server_type());
							__commonlib_delegate->onServerAdded(addr.server_type(), addr.server_id());
						}
					}
				}
				break;
			case MSG_TYPE_CONTROL_SERVER_ADDR_INFO_REMOVE:
				{
					common::AddressInfo addr;
					if (ParseMsg(data, addr))
					{
						LOGEVENTL("DEBUG", "MSG_TYPE_CONTROL_SERVER_ADDR_INFO_REMOVE");
						_NetLibPlus_RemoveServerInfo(addr.server_id(), addr.ip(), addr.port(), addr.server_type());
						__commonlib_delegate->onServerRemoved(addr.server_type(), addr.server_id());
					}
				}
				break;
			case MSG_TYPE_CONTROL_SERVER_ADDR_INFO_RESTART:
				LOGEVENTL("DEBUG", "MSG_TYPE_CONTROL_SERVER_ADDR_INFO_RESTART");
				//RESTART先啥也不干
				break;
			case MSG_TYPE_CMD2SERVER:
				{
					common::Cmd2Server cmd;
					if (ParseMsg(data, cmd))
					{
						__commonlib_delegate->onReceiveCmd(cmd.cmd_type(), cmd.data());
					}
				}
				break;
			case MSG_TYPE_REFRESH_CONFIG:
				{
					common::RefreshConfig rc;
					if (ParseMsg(data, rc))
					{
						for (int i = 0; i < rc.file_size(); ++i)
						{
							__commonlib_delegate->onConfigRefresh(rc.file(i).file_name(), rc.file(i).content());
						}
					}
					if (!is_config_initialized)
					{
						is_config_initialized = true;
						__commonlib_delegate->onConfigInitialized();
					}
				}
				break;
			default:
				__commonlib_delegate->onReceiveOtherDataFromControlServer(SERVER_MSG_TYPE(data), SERVER_MSG_AFTER_HEADER_BASE(data), SERVER_MSG_LENGTH(data) - SERVER_MSG_HEADER_BASE_SIZE);
				break;
			}
		}
		else
		{
			LOGEVENTL("ERROR", "message not enough size: " << SERVER_MSG_LENGTH(data));
		}
	}
};

void InitializeCommonLib(ioservice_thread& thread, CommonLibDelegate* d, int my_server_type, int argc, char* argv[])
{
	_initialize_thread(&thread);

	__commonlib_delegate = d;
	__my_server_type = my_server_type;

	if (argc >= 3)
	{
		//从启动参数里读

		__control_server_ip = argv[1];
		__control_server_port = utility1::str2int(argv[2]);
	}
	else
	{
		//从ini里面读
		CSimpleIni ini;
		if (ini.LoadFile(utility3::ToAbsolutePath("local_config.ini").c_str()) == SI_OK)
		{
			__control_server_ip = ini.GetValue("ControlServer", "ControlServerIP", CONTROL_SERVER_DEFAULT_IP);
			__control_server_port = ini.GetLongValue("ControlServer", "ControlServerPortP", CONTROL_SERVER_DEFAULT_PORT);
		}
	}

	NetLibPlus_InitializeClients<Conn2ControlServerDelegate>(SERVER_TYPE_CONTROL_SERVER);
	_NetLibPlus_UpdateServerInfo(CONTROL_SERVER_ID, boost::asio::ip::address_v4::from_string(__control_server_ip).to_ulong(), __control_server_port, SERVER_TYPE_CONTROL_SERVER);

	if (my_server_type == SERVER_TYPE_BACKGROUND)
	{ 
		//TODO 发另外的权限验证包。专门用于后台之类的程序
	}

	common::Initialize init;
	init.set_server_type(__my_server_type);
	SendMsg(CONTROL_SERVER_ID, MSG_TYPE_CONTROL_SERVER_INITIALIZE, init);
}

void ServerStarted(int my_listening_port)
{
	__my_listening_port = my_listening_port;
	SayHello2ControlServer();
}

void ReportLoad(float load_factor)
{
	common::ReportLoad load;
	load.set_load(load_factor);
	SendMsg(CONTROL_SERVER_ID, MSG_TYPE_CONTROL_SERVER_REPORT_LOAD, load);
}

bool ParseHeaderEx(char* data, common::HeaderEx& proto)
{
	return proto.ParseFromArray(SERVER_MSG_AFTER_HEADER_BASE(data), SERVER_MSG_HEADER_EX_LEN(data));
}

//TODO
//std::map<int, common::CorrespondingServer> corresponding_server_map;

//假设不会频繁调用。网关只会在知道CorrespondingServer有修改时才发，其它各服也只会在收到CorrespondingServer非空时才Update
/*
void UpdateCorrespondingServer(uint64_t user_id, const common::CorrespondingServer& cs)
{
	//这里直接把一个复杂结构放map里了。优化的时候可以考虑把这里改成指针什么的，或者用unordered_map
	corresponding_server_map[user_id] = cs;
}
*/ 

void ReplyEmptyMsg(NetLib_ServerSession_ptr sessionptr, uint32_t msg_type)
{
	char* send_buf = new char[SERVER_MSG_HEADER_BASE_SIZE];
	SERVER_MSG_LENGTH(send_buf) = SERVER_MSG_HEADER_BASE_SIZE;		// 数据包的字节数（含msghead）
	SERVER_MSG_TYPE(send_buf) = msg_type;
	SERVER_MSG_ERROR(send_buf) = 0;
	SERVER_MSG_HEADER_EX_LEN(send_buf) = 0;
	SERVER_MSG_RESERVED(send_buf) = 0;

	sessionptr->SendAsync(send_buf);
}

void RefreshConfig(int server_type, const std::string& file_name, const std::string& content)
{
	common::RefreshConfig rc;
	rc.set_server_type(server_type);
	common::ConfigFile* file = rc.add_file();
	file->set_file_name(file_name);
	file->set_content(content);
	SendMsg(CONTROL_SERVER_ID, MSG_TYPE_REFRESH_CONFIG, rc);
}
#include "include/ToAbsolutePath.h"
#include "include/ioservice_thread.h"
#include "Log/Log.h"
#include "NetLib/NetLib.h"
#include "ServerCommonLib/ServerCommon.h"
#include "ControlServerSessionDelegate.h"
#include "include/SimpleIni.h"
#include "include/utility1.h"
#include <google/protobuf/stubs/common.h>
#include "ControlServerConfig.h"
#include <boost/asio.hpp>
#include "ControlServer.h"
#include "MessageTypeDef.h"
#include "Config.h"

ioservice_thread thread;

std::string config_file_path;
ControlServerConfig config;

bool during_startup = true;

AvailableIDs<uint32_t> available_ids;

std::map<IPPort, NetLib_ServerSession_ptr> server2session;
std::map<IPPort, ServerInfo*> servers_info;
std::map<int, server_group*> server_groups;

//这个的timeout只会因为timer而触发。如果不是的话，还得keep_alive一份timer
void ServerTimeout(IPPort ip_port, const boost::system::error_code& error)
{
	if (!error)
	{
		LOGEVENTL("Debug", "Server timeout");

		//有服务超时了，得把他从数据结构里移除，并告知他人
		auto info_it = servers_info.find(ip_port);
		if (info_it != servers_info.end())
		{
			informAddressInfo(info_it->second->addr, MSG_TYPE_CONTROL_SERVER_ADDR_INFO_REMOVE);
			LOGEVENTL("INFO", "Send remove to all. " << _ln("server_id") << info_it->second->addr.server_id());

			auto it_group = server_groups.find(info_it->second->addr.server_type());
			if (it_group != server_groups.end())
			{
				it_group->second->erase(ip_port);
			}

			available_ids.releaseId(info_it->second->addr.server_id());

			delete info_it->second;
			servers_info.erase(info_it);
		}
		else
		{
			LOGEVENTL("ERROR", "When a server timeout, can't find it in servers_info. " << log_::n("ip") << ip_port.first << log_::n("port") << ip_port.second);
		}
	}
}

void Initialize()
{
	for (int s = 0; s <= SERVER_TYPE_MAX; ++s)
	{
		server_groups[s] = new server_group();
	}
}

extern std::map<std::pair<int, std::string>, std::string*> configs;

void UnInitliaze()
{
	for (int s = 0; s <= SERVER_TYPE_MAX; ++s)
	{
		delete server_groups[s];
	}

	for (auto it = servers_info.begin(); it != servers_info.end(); ++it)
	{
		delete it->second;
	}

	unInitializeConfigs();
}

//反正是在同一个线程里的，就直接改config的值了
void LoadConfig()
{
	CSimpleIni ini;
	if (ini.LoadFile(utility3::ToAbsolutePath(config_file_path).c_str()) == SI_OK)
	{
		config.startup_ms = ini.GetLongValue("ControlServer", "StartupMS", _CONFIG_DEFAULT_STARTUP_MS); //这个参数目前只在启动的时候有效，Reload了也不管用
		config.timeout_sec = ini.GetLongValue("ControlServer", "TimeoutSec", _CONFIG_DEFAULT_TIMEOUT_SEC); //这个参数目前只在启动的时候有效，Reload了也不管用
		config.server_configs_list_file = ini.GetValue("ControlServer", "ServerConfigsListFile", "configs/list.json"); //这个参数目前只在启动的时候有效，Reload了也不管用
	}

	initializeConfigs();
}

void GenerateAddressList(common::AddressList& list)
{
	for (auto it = servers_info.begin(); it != servers_info.end(); ++it)
	{
		*list.add_addr() = it->second->addr;
	}
}

void StartupTimer(const boost::system::error_code& error)
{
	if (!error)
	{
		LOGEVENTL("INFO", "Server startup timer triggered.");
		during_startup = false;

		//告知各服务完整地址信息
		common::AddressList addr_list;
		GenerateAddressList(addr_list);
		informAddressInfo(addr_list, MSG_TYPE_CONTROL_SERVER_ADDR_INFO_REFRESH);
	}
}

int main(int argc, char* argv[])
{
	//Check Memory Leaks
#if WIN32
	// Get the current bits
	int tmp = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);

	tmp |= _CRTDBG_LEAK_CHECK_DF;

	// Set the new bits
	_CrtSetDbgFlag(tmp);
#endif

	NETLIB_CHECK_VERSION;
	LogInitializeLocalOptions(true, true, "control");

	if (argc >= 2)
	{
		config_file_path = argv[1];
	}
	else
	{
		config_file_path = "ControlServer.ini";
	}

	thread.start();

	Initialize();

	LoadConfig();

	NetLib_Server_ptr server = NetLib_NewServer<ControlServerSessionDelegate>(&thread);

	if (!server->StartTCP(CONTROL_SERVER_DEFAULT_PORT, 1, config.timeout_sec, NETLIB_SERVER_LISTEN_KEEP_ALIVE_EVENT)) //端口，线程数，超时时间
	{
		LOGEVENTL("Error", "Server Start Failed !");

		NetLib_Servers_WaitForStop();

		LogUnInitialize();

		thread.stop_when_no_work();
		thread.wait_for_stop();

		google_lalune::protobuf::ShutdownProtobufLibrary();

		return 0;
	}

	LOGEVENTL("Info", "Server Start Success. " << _ln("Port") << CONTROL_SERVER_DEFAULT_PORT);

	boost::asio::deadline_timer timer(thread.get_ioservice());
	timer.expires_from_now(boost::posix_time::milliseconds(config.startup_ms));
	timer.async_wait(boost::bind(&StartupTimer, boost::asio::placeholders::error));

	for (;;)
	{
		char tmp[200];
		if (scanf("%s", tmp) <= 0)
		{
			boost::this_thread::sleep(boost::posix_time::hours(1));
			continue;
		}

		if (strcmp(tmp, "stop") == 0)
		{
			if (server)
			{
				server->Stop();
				server.reset();
			}
		}
		else if (strcmp(tmp, "exit") == 0)
		{
			break;
		} 
	}

	UnInitliaze();

	NetLib_Servers_WaitForStop();

	LogUnInitialize();

	thread.stop_when_no_work();
	thread.wait_for_stop();

	google_lalune::protobuf::ShutdownProtobufLibrary();

	return 0;
}

//待增加测试：
/*

关一个服务器，是否能如期触发超时（查ControlServer的数据结构）  PASS

...

*/

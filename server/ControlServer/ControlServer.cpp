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
#include "Header.h"

ioservice_thread thread;

std::string config_file_path;
ControlServerConfig config;

bool during_startup = true;

uint32_t next_server_id = 1;
std::vector<uint32_t> available_ids;

uint32_t getNextId()
{
	if (available_ids.size())
	{
		int _id = available_ids.back();
		available_ids.pop_back();
		return _id;
	}
	else
	{
		return next_server_id++;
	}
}

void releaseId(uint32_t _id)
{
	if (_id + 1 == next_server_id)
	{
		next_server_id--;
	}
	else
	{
		available_ids.push_back(_id);
	}
}

std::map<NetLib_ServerSession_ptr, std::pair<int, int>> session2server;
std::map<std::pair<int, int>, std::pair<common::AddressInfo, boost::asio::deadline_timer>> servers_info;
std::set<NetLib_ServerSession_ptr> gateways;

//反正是在同一个线程里的，就直接改config的值了
void LoadConfig()
{
	CSimpleIni ini;
	if (ini.LoadFile(utility3::ToAbsolutePath(config_file_path).c_str()) == SI_OK)
	{
		config.startup_ms = ini.GetLongValue("ControlServer", "StartupMS", 8000); //这个参数目前只在启动的时候有效，Reload了也不管用
		config.timeout_sec = ini.GetLongValue("ControlServer", "TimeoutSec", 7); //这个参数目前只在启动的时候有效，Reload了也不管用
	}
}

void StartupTimer(const boost::system::error_code& error)
{
	if (error)
	{
		during_startup = false;

		//告知各服务完整地址信息

		common::AddressList addr_list;
		for (auto it = servers_info.begin(); it != servers_info.end(); ++it)
		{
			*addr_list.add_addr() = it->second.first;
		}
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

	thread.get_ioservice().post(boost::bind(&LoadConfig));

	NetLib_Server_ptr server = NetLib_NewServer<ControlServerSessionDelegate>(&thread);

	if (!server->StartTCP(CONTROL_SERVER_DEFAULT_PORT, 1, config.timeout_sec, NETLIB_SERVER_LISTEN_KEEP_ALIVE_EVENT)) //端口，线程数，超时时间
	{
		LOGEVENTL("Error", "Server Start Failed !");

		NetLib_Servers_WaitForStop();

		LogUnInitialize();

		google_lalune::protobuf::ShutdownProtobufLibrary();

		return 0;
	}

	LOGEVENTL("Info", "Server Start Success");

	boost::asio::deadline_timer timer(thread.get_ioservice(), boost::posix_time::milliseconds(config.startup_ms));
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

	NetLib_Servers_WaitForStop();

	LogUnInitialize();

	google_lalune::protobuf::ShutdownProtobufLibrary();

	return 0;
}


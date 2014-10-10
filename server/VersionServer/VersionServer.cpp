#include <boost/asio.hpp>
#include "include/ioservice_thread.h"
#include "Log/Log.h"
#include "NetLib/NetLib.h"
#include "memory.h"
#include "string.h"
#include <google/protobuf/stubs/common.h>
#include "../../LaluneCommon/include/MessageTypeDef.h"
#include "ServerCommonLib/ServerCommon.h"
#include "VersionServerSessionDelegate.h"

#define VERSION_SERVER_PORT (5577)

ioservice_thread thread;

NetLib_Server_ptr server;

class VersionServerCommonLibDelegate : public CommonLibDelegate
{
public:
	void onConfigInitialized()
	{
		server = NetLib_NewServer<VersionServerSessionDelegate>(&thread);

		//可以不指定端口 TODO (主要是内部端口)
		if (!server->StartTCP(VERSION_SERVER_PORT, 1, 120)) //端口，线程数，超时时间
		{
			LOGEVENTL("Error", "Server Start Failed !");

			NetLib_Servers_WaitForStop();

			LogUnInitialize();

			google_lalune::protobuf::ShutdownProtobufLibrary();

			exit(0);
		}

		LOGEVENTL("Info", "Server Start Success. " << _ln("Port") << VERSION_SERVER_PORT);

		ServerStarted(VERSION_SERVER_PORT);
	}

	void onConfigRefresh(const std::string& file_name, const std::string& content)
	{
		LOGEVENTL("CONFIG_REFRESH", _ln("file_name") << file_name << _ln("content") << content);
		//TODO blablabla 收到配置文件
	}
	
	void onReceiveCmd(int cmd_type, const std::string& data)
	{
		LOGEVENTL("onReceiveCmd", cmd_type);
	}
};

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
    LogInitializeLocalOptions(true, true, "version_server");

	thread.start();

	VersionServerCommonLibDelegate* cl_delegate = new VersionServerCommonLibDelegate();
	InitializeCommonLib(thread, cl_delegate, SERVER_TYPE_VERSION_SERVER, argc, argv);
	
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


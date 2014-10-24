#include <boost/asio.hpp>
#include "include/ioservice_thread.h"
#include "Log/Log.h"
#include "NetLib/NetLib.h"
#include "memory.h"
#include "string.h"
#include <google/protobuf/stubs/common.h>
#include "MessageTypeDef.h"
#include "ServerCommonLib/ServerCommon.h"
#include "BasicInfoServerSessionDelegate.h"

#define BASIC_INFO_SERVER_PORT (8432)

ioservice_thread thread;

NetLib_Server_ptr server;

class BasicInfoServerCommonLibDelegate : public CommonLibDelegate
{
public:
	void onConfigRefresh(const std::string& content)
	{
	}

	void onConfigInitialized()
	{
		server = NetLib_NewServer<BasicInfoServerSessionDelegate>(&thread);

		//可以不指定端口 TODO (主要是内部端口)
		if (!server->StartTCP(BASIC_INFO_SERVER_PORT, 1, 120)) //端口，线程数，超时时间
		{
			LOGEVENTL("Error", "Server Start Failed !");

			LogUnInitialize();

			exit(0);
		}

		LOGEVENTL("Info", "Server Start Success");

		ServerStarted(BASIC_INFO_SERVER_PORT);
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
    LogInitializeLocalOptions(true, true, "gateway");

	thread.start();

	BasicInfoServerCommonLibDelegate* cl_delegate = new BasicInfoServerCommonLibDelegate();
	InitializeCommonLib(thread, cl_delegate, SERVER_TYPE_BASIC_INFO_SERVER, argc, argv);
	
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


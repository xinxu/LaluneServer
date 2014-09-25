#include <boost/asio.hpp>
#include "include/ioservice_thread.h"
#include "Log/Log.h"
#include "NetLib/NetLib.h"
#include "memory.h"
#include "string.h"
#include <google/protobuf/stubs/common.h>
#include "MessageTypeDef.h"
#include "ServerCommonLib/ServerCommon.h"
#include "GatewaySessionDelegate.h"
#include "GatewayUserSessionDelegate.h"

#define GATEWAY_OUTER_PORT (6677) //服务于用户的
#define GATEWAY_INNER_PORT (9432) //内部通信用的

ioservice_thread thread;

NetLib_Server_ptr server4user;

class GatewayCommonLibDelegate : public CommonLibDelegate
{
public:
	void onInitialized()
	{
		server4user	= NetLib_NewServer<GatewayUserSessionDelegate>(&thread);

		//超时时间得可以中途重设 TODO
		if (!server4user->StartTCP(GATEWAY_OUTER_PORT, 1, 120)) //端口，线程数，超时时间
		{
			LOGEVENTL("Error", "Server4User Start Failed !");

			//TODO 发出警报，或者退出程序
		}

		LOGEVENTL("Info", "Server4User Start Success");
	}

	void onConfigRefresh(const std::string& content)
	{

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

	NetLib_Server_ptr server = NetLib_NewServer<GatewaySessionDelegate>(&thread);

	//可以不指定端口 TODO (主要是内部端口)
	if (!server->StartTCP(GATEWAY_INNER_PORT, 1, 120)) //端口，线程数，超时时间
	{
		LOGEVENTL("Error", "Server Start Failed !");

		NetLib_Servers_WaitForStop();

		LogUnInitialize();

		google_lalune::protobuf::ShutdownProtobufLibrary();

		return 0;
	}
	
	LOGEVENTL("Info", "Server Start Success");

	GatewayCommonLibDelegate* cl_delegate = new GatewayCommonLibDelegate();
	InitializeCommonLib(thread, cl_delegate, GATEWAY_INNER_PORT, SERVER_TYPE_GATEWAY_SERVER, argc, argv);
	
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


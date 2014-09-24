#include <boost/asio.hpp>
#include "include/ioservice_thread.h"
#include "Log/Log.h"
#include "NetLib/NetLib.h"
#include "memory.h"
#include "string.h"
#include <google/protobuf/stubs/common.h>
#include "../../LaluneCommon/include/Header.h"
#include "ServerCommonLib/ServerCommon.h"
#include "BasicInfoServerSessionDelegate.h"

#define BASIC_INFO_SERVER_PORT (8432)

ioservice_thread thread;

NetLib_Server_ptr server4user;

class BasicInfoServerCommonLibDelegate : public CommonLibDelegate
{
public:
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
	if (!server->StartTCP(BASIC_INFO_SERVER_PORT, 1, 120)) //端口，线程数，超时时间
	{
		LOGEVENTL("Error", "Server Start Failed !");

		NetLib_Servers_WaitForStop();

		LogUnInitialize();

		google_lalune::protobuf::ShutdownProtobufLibrary();

		return 0;
	}
	
	LOGEVENTL("Info", "Server Start Success");

	BasicInfoServerCommonLibDelegate* cl_delegate = new BasicInfoServerCommonLibDelegate();
	InitializeCommonLib(thread, cl_delegate, BASIC_INFO_SERVER_PORT, SERVER_TYPE_BASIC_INFO_SERVER, argc, argv);
	
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


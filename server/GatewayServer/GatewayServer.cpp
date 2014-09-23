#include <boost/asio.hpp>
#include "include/ioservice_thread.h"
#include "Log/Log.h"
#include "NetLib/NetLib.h"
#include "memory.h"
#include "string.h"
#include <google/protobuf/stubs/common.h>
#include "../../LaluneCommon/include/Header.h"
#include "ServerCommonLib/ServerCommon.h"

//实现一个SessionDelegate,至少要实现RecvFinishHandler
class MySessionDelegate : public NetLib_ServerSession_Delegate
{	
public:	
	void ConnectedHandler(NetLib_ServerSession_ptr sessionptr)
	{

	}

	//RecvFinishHandler一旦返回，data的内容就会被释放
	void RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
	{
		uint32_t len = MSG_LENGTH(data);

		//switch (
	}
};

class GatewayCommonLibDelegate : public CommonLibDelegate
{
public:
	void onConfigRefresh(const std::string& content)
	{

	}
};

ioservice_thread thread;

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

	NetLib_Server_ptr server = NetLib_NewServer<MySessionDelegate>(&thread);

	//可以不指定端口 TODO (主要是内部端口)
	//超时时间得可以中途重设 TODO
	if (!server->StartTCP(4531, 1, 120)) //端口，线程数，超时时间
	{
		LOGEVENTL("Error", "Server Start Failed !");

		NetLib_Servers_WaitForStop();

		LogUnInitialize();

		google_lalune::protobuf::ShutdownProtobufLibrary();

		return 0;
	}
	
	LOGEVENTL("Info", "Server Start Success");

	GatewayCommonLibDelegate* cl_delegate = new GatewayCommonLibDelegate();
	InitializeCommonLib(thread, cl_delegate, 4531, SERVER_TYPE_GATEWAY_SERVER, argc, argv);
	
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


#include <boost/asio.hpp>
#include "include/ioservice_thread.h"
#include "Log/Log.h"
#include "NetLib/NetLib.h"
#include "memory.h"
#include "string.h"
#include <google/protobuf/stubs/common.h>
#include "MessageTypeDef.h"
#include "ServerHeaderDef.h"
#include "InnerServerSessionDelegate.h"
#include "UserSessionDelegate.h"
#include <vector>
#include "include/utility1.h"

#define _OUTER_PORT (6677) //服务于用户的
#define _INNER_PORT (9432) //内部通信用的，目前主要是PvP战斗服会连上来

ioservice_thread thread;

NetLib_Server_ptr server, server4user;

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
	LogInitializeLocalOptions(true, true, "server");

	server = NetLib_NewServer<InnerServerSessionDelegate>(&thread);

	//可以不指定端口 TODO (主要是内部端口)
	if (!server->StartTCP(_INNER_PORT, 1, 120)) //端口，线程数，超时时间
	{
		LOGEVENTL("Error", "Server Start Failed !");

		NetLib_Servers_WaitForStop();

		LogUnInitialize();

		google_lalune::protobuf::ShutdownProtobufLibrary();

		exit(0);
	}

	LOGEVENTL("Info", "Server(for inner servers) Start Success. " << _ln("Port") << _INNER_PORT);

	server4user = NetLib_NewServer<UserSessionDelegate>(&thread);

	//超时时间得可以中途重设 TODO
	if (!server4user->StartTCP(_OUTER_PORT, 1, 25)) //端口，线程数，超时时间  （客户端现在是15秒发个心跳包）
	{
		LOGEVENTL("Error", "Server4User Start Failed !");

		server->Stop();
		server.reset();

		NetLib_Servers_WaitForStop();

		LogUnInitialize();

		google_lalune::protobuf::ShutdownProtobufLibrary();

		exit(0);
	}

	LOGEVENTL("Info", "Server4User Start Success. " << _ln("Port") << _OUTER_PORT);

	thread.start();

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

				server4user->Stop();
				server4user.reset();
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


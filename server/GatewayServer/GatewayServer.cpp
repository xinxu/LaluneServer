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
#include "toBackEndDelegate.h"
#include <vector>
#include "include/utility1.h"

#define GATEWAY_OUTER_PORT (6677) //服务于用户的
#define GATEWAY_INNER_PORT (9432) //内部通信用的

ioservice_thread thread;

NetLib_Server_ptr server4user;

std::map<int, NetLib_ServerSession_ptr> user2session;
AvailableIDs<int> available_tmp_ids; //其实不一定需要搞这么一套available_id，直接让operation_id加到从0开始也可以。但id小一点好像能省一点流量。。

int GetTmpUserId()
{
	return available_tmp_ids.getId();
}

void ReleaseTmpUserId(int _id)
{
	available_tmp_ids.releaseId(_id);
}

void UpdateUserSession(int uid, NetLib_ServerSession_ptr session)
{
	user2session[uid] = session;
}

void UserSessionLeft(int uid)
{
	user2session.erase(uid);
}

NetLib_ServerSession_ptr GetSessionById(int uid)
{
	auto it = user2session.find(uid);
	if (it == user2session.end())
	{
		return nullptr;
	}
	else
	{
		return it->second;
	}
}

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

	NetLibPlus_InitializeClients<toBackEndDelegate>();
	
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


#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "Log/Log.h"
#include "NetLib/NetLib.h"
#include "memory.h"
#include "string.h"
#include <google/protobuf/stubs/common.h>



//实现一个SessionDelegate,至少要实现RecvFinishHandler
class MySessionDelegate : public NetLib_ServerSession_Delegate
{	
public:	
	//RecvFinishHandler一旦返回，data的内容就会被释放
	void RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
	{
		std::string ip;
		uint16_t port;
		sessionptr->GetRemoteAddress(ip, port);
		LOGEVENTL("address", log_::n("ip") << ip << log_::n("port") << port);
		int len = *(int32_t*)data;
		std::string s(data + 4, len - 4);
		for (int i = 0; i < s.size(); ++i) s[i] += 48;
		LOGEVENTL("info", log_::n("len") << len << log_::n("content") << s);

		char* reply_data = new char[20];
		memset(reply_data, 0, sizeof(reply_data));

		*(int*)reply_data = 20;
		*(reply_data + 4) = 'A';
		*(reply_data + 5) = 'B';
		*(reply_data + 6) = 'C';

		sessionptr->SendCopyAsync(reply_data);
	}
};

boost::asio::deadline_timer* timer;

void Handler(const boost::system::error_code& error)
{
	if (!error)
	{
		//do something

		//set the timer again
		timer->expires_from_now(boost::posix_time::milliseconds(500));
		timer->async_wait(boost::bind(&Handler, boost::asio::placeholders::error));
	}
	else
	{
		//timer has been canceled
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

	NetLib_Server_ptr server = NetLib_NewServer<MySessionDelegate>();
	timer = new boost::asio::deadline_timer(*server->GetWorkIoService());

	if (!server->StartTCP(23, 1, 300)) //端口，线程数，超时时间
	{
		LOGEVENTL("Error", "Server Start Failed !");
	}
	else
	{
		LOGEVENTL("Info", "Server Start Success");
	}
	
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
				server->Stop(); //不阻塞 应该叫Let's Stop
				server.reset();
			}
		}
		else if (strcmp(tmp, "wait") == 0)
		{
			NetLib_Servers_WaitForStop(); //Wait until really stopped
			LOGEVENTL("Info", "Wait OK.");
		}
		else if (strcmp(tmp, "exit") == 0)
		{			
			break;
		}
		else if (strcmp(tmp, "start") == 0)
		{			
			int tcp_port, work_thread, timeout_seconds;
			scanf("%d%d%d", &tcp_port, &work_thread, &timeout_seconds);
			if (server)
			{
				server->Stop();
			}
			server = NetLib_NewServer<MySessionDelegate>();
			if (!server->Start(tcp_port, work_thread, timeout_seconds))
			{
				LOGEVENTL("Error", "Server Start Failed !");
			}
			else
			{
				LOGEVENTL("Info", "Server Start Success");
			}
		}
	}

	NetLib_Servers_WaitForStop();

	LogUnInitialize();
	
	google::protobuf::ShutdownProtobufLibrary(); 

	return 0;
}


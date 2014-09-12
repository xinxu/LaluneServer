// tNetLib2.cpp : Defines the entry point for the console application.
//
//#include "stdafx.h"

#include <boost/asio.hpp>
#include "Log/Log.h"
#include "NetLib/NetLib.h"
#include "memory.h"
#include "string.h"
#include "include/ioservice_thread.h"
#include <google/protobuf/stubs/common.h>

ioservice_thread ioservice_th, th2;

void send_back_something2(NetLib_ServerSession_ptr sessionptr, char* data)
{
	sessionptr->SendAsync(data);
}


void send_back_something(NetLib_ServerSession_ptr sessionptr, char* data)
{
	th2.get_ioservice().post(boost::bind(&send_back_something2, sessionptr, data));
}

//实现一个SessionDelegate,至少要实现RecvFinishHandler和SendFinishHandler两个方法
class MySessionDelegate : public NetLib_ServerSession_Delegate
{	
protected:
	int m_port;

public:	
	MySessionDelegate() : m_port(-1) //使用默认构造函数的情况不会使用m_port这个变量
	{
	}

	MySessionDelegate(int port) : m_port(port)
	{
	}

	//这个方法可以不重写，默认是空
	void ConnectedHandler(NetLib_ServerSession_ptr sessionptr)
	{
		LOGEVENTL("Connected", "someone connected. " << _ln("ptr") << hex((std::size_t)sessionptr.get()));

		/*std::string remote_ip;
		uint16_t remote_port;
		sessionptr->GetRemoteAddress(remote_ip, remote_port);
		LOGEVENTL("Connected", log_::n("LocalIP") << sessionptr->GetLocalAddress() << log_::n("LocalPort") << m_port
							<< log_::n("RemoteIP") << remote_ip << log_::n("RemotePort") << remote_port);*/
	}

	//这个方法可以不重写，默认是空
	virtual void DisconnectedHandler(NetLib_ServerSession_ptr sessionptr, NetLib_Error error, int inner_error_code)
	{	
		if (sessionptr == sessionptr)
		{
			int a = 1;
		}
		//LOGEVENTL("Connected", "someone disconnected.");

		/*
		std::string remote_ip;
		uint16_t remote_port;
		sessionptr->GetRemoteAddress(remote_ip, remote_port);
		LOGEVENTL("Disconnected", log_::n("LocalPort") << m_port << log_::n("RemoteIP") << remote_ip << log_::n("RemotePort") << remote_port
								<< log_::n("error") << error << log_::n("inner_error") << inner_error_code);*/
	}

	//SendAsync方法传的data参数必须在SendFinishHandler触发后才能删除
	void SendFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data, void* pHint)
	{
		//LOGEVENTL("Debug", "server_session send finish. Hint: " << (std::size_t)pHint);
		delete[] data;
	}

	//RecvFinishHandler一旦返回，data的内容就会被释放
	void RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
	{
		uint32_t size = *(uint32_t*)data;

		if (size >= 8)
		{
			uint32_t cmd = *(uint32_t*)(data + 4);
			if (cmd == 240)
			{
				//240是测试指令：服务器回复一份数据后马上关闭本session.
				//send the data back to the client
				sessionptr->SendCopyAsync(data);
				sessionptr->Disconnect();
			}
			else if (cmd == 245) //命令245不给返回值
			{
				LOGEVENTL("SpecialCmd", 245);
			}			
			else
			{
				char* data_copy = new char[size];
				memcpy(data_copy, data, size);
				th2.get_ioservice().post(boost::bind(&send_back_something, sessionptr, data_copy));
			}
			/*else
			{
				//send the data back to the client
				//sessionptr->SendCopyAsync(data);

				char* newdata = new char[size];
				memcpy(newdata, data, size);
				sessionptr->SendAsync(newdata, (void*)155);
			}*/
		}
	}
};

void print1()
{
	LOGEVENTL("IoService1", "OK");
}

void print2()
{
	LOGEVENTL("IoService2", "OK");
}

int begin_port, end_port;
NetLib_Server_ptr* servers;
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
    LogInitializeLocalOptions(true, true, "server_sample");
#ifdef _DEBUG
	//NetLib_Set_UDP_ResendIntervalMS(10 * 1000);
	//NetLib_Set_UDP_MaxResendIntervalMS(10 * 60 * 1000); //仅调试中使用，避免一打断点就超时断线。生产场合请使用默认值，或者不要设置这么大的超时时间
#endif

	th2.start(4);

	//第一个是TCP端口，第二个是UDP端口
	NetLib_Server_ptr server = NetLib_NewServer<MySessionDelegate>();
	//if (!server->StartTCP(1248, 1, 5))
	//if (!server->StartUDP(2012, 1, 5))
	//if (!server->Start(1248, 2012, 3, 10))
	//if (!server->Start(1248, 2012, 4)) //第四个参数是Session超时时间
	if (!server->Start(1248, 2012, 1, 30)) //第四个参数是Session超时时间
	{
		LOGEVENTL("Error", "Server Start Failed !");
	}
	else
	{
		LOGEVENTL("Info", "Server Start Success");
	}

#ifndef _DYNAMIC_NETLIB_
	//DEBUG
	server->GetWorkIoService()->post(boost::bind(&print1));
	th2.get_ioservice().post(boost::bind(&print2));
#endif

	for (;;)
	{
		char tmp[200];
		if (scanf("%s", tmp) <= 0)
		{
#ifdef WIN32
			::Sleep(60000);
#else
			sleep(60);
#endif
		}

		if (strcmp(tmp, "stop") == 0)
		{
			if (server)
			{
				server->Stop();
				server.reset();
			}
		}
		else if (strcmp(tmp, "wait") == 0)
		{
			NetLib_Servers_WaitForStop();
			LOGEVENTL("Info", "Wait OK.");
		}
		else if (strcmp(tmp, "exit") == 0)
		{			
			break;
		}
		else if (strcmp(tmp, "start") == 0)
		{			
			int tcp_port, udp_port, work_thread, timeout_seconds;
			scanf("%d%d%d%d", &tcp_port, &udp_port, &work_thread, &timeout_seconds);
			if (server)
			{
				server->Stop();
			}
			server = NetLib_NewServer<MySessionDelegate>();
			if (!server->Start(tcp_port, udp_port, work_thread, timeout_seconds))
			{
				LOGEVENTL("Error", "Server Start Failed !");
			}
			else
			{
				LOGEVENTL("Info", "Server Start Success");
			}
		}
		else if (strcmp(tmp, "startrange") == 0)
		{
			scanf("%d%d", &begin_port, &end_port);

			LOGEVENTL("StartRange", log_::n("begin_port") << begin_port << log_::n("end_port") << end_port)

			servers = new NetLib_Server_ptr[end_port - begin_port + 1];

			ioservice_th.start(8); //8线程
			
			for (int i = begin_port; i <= end_port; ++i)
			{
				std::shared_ptr<MySessionDelegate> sd = std::make_shared<MySessionDelegate>(i);
				servers[i - begin_port] = NetLib_NewServer(sd, &ioservice_th);

				if (servers[i - begin_port]->StartTCP(i, 1, 60 * 60)) //外面已经把ioservice给启动了，里面的线程数就没有意义了
				{
					LOGEVENTL("BindSuccess", log_::n("Port") << i);					
				}
				else
				{
					LOGEVENTL("BindFail", log_::n("Port") << i);
				}
				/*if (i % 10 == 0)
				{
					boost::this_thread::sleep(boost::posix_time::milliseconds(200));
				}*/
			}
		}
		else if (strcmp(tmp, "stoprange") == 0)
		{
			LOGEVENTL("StopRange", "")
			for (int i = begin_port; i <= end_port; ++i)
			{
				servers[i - begin_port]->Stop();
			}
			delete[] servers;

			ioservice_th.stop_when_no_work();
			ioservice_th.wait_for_stop();

			LOGEVENTL("Info", "All Servers Stopped.");
		}
	}

	th2.stop_when_no_work();
	th2.wait_for_stop();

	NetLib_Servers_WaitForStop();

	LogUnInitialize();
	
	google_lalune::protobuf::ShutdownProtobufLibrary(); 

#ifndef _STATIC_NETLIB_
	NetLib_Shutdown();
#endif
	return 0;
}


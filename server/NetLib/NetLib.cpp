// NetLib.cpp : Defines the exported functions for the DLL application.
//
//#include "stdafx.h"
#include "NetLib.h"
#include "NetLib_Client_Imp.h"
#include "NetLib_Server_Imp.h"
#include "NetLib_Params.h"
#include <boost/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <string>
#include <set>
#include "Log/Log.h"
#include "../include/ioservice_thread.h"

int clients_running_threads_count = 0;
int clients_running_objs_count = 0;
int servers_running_objs_count = 0;
boost::mutex _NetLib_mutex;

NETLIB_API void NetLib_Set_UDP_ReplyIntervalMS(int UDP_ReplyIntervalMS)
{
	UDP_REPLY_INTERVAL_MS = UDP_ReplyIntervalMS;
}

NETLIB_API void NetLib_Set_UDP_MaxConsecutiveResendTimes(int UDP_MaxConsecutiveResendTimes)
{
	UDP_MAX_CONSECUTIVE_RESEND_TIMES = UDP_MaxConsecutiveResendTimes;
}

NETLIB_API void NetLib_Set_MaxPacketSize(unsigned int MaxPacketSize)
{
	MAX_PACKET_SIZE = MaxPacketSize;
}

NETLIB_API void NetLib_Set_Server_Recv_Buffer_Size(int Server_Recv_Buffer_Size)
{
	SERVER_RECV_BUFFER_SIZE = Server_Recv_Buffer_Size;
}

NETLIB_API void NetLib_Set_Client_Recv_Buffer_Size(int Client_Recv_Buffer_Size)
{
	CLIENT_RECV_BUFFER_SIZE = Client_Recv_Buffer_Size;
}

class thread_withioservice //这个类没有什么封装意义，纯粹是为了使用this指针，利用this删成员。
{
public:
	boost::asio::io_service client_ioservice; 
	boost::thread _thread;

protected:
	void run()
	{
		client_ioservice.run();
	
		//进到这里说明virtual_work已经被释放了，在某些情况下可能另一个线程正在启动，但他们已经用的是另外的thread和ioservice了。即利用class将本线程相关的放在一起。
		//不会发生client_ioservice用的时候被释放的情况，因为一旦进到这里，都说明virtual_work被释放过了，要再有别的工作要做，也会是在新的线程、新的ioservice里
				
		delete this;
		
		LOGEVENTL("NetLib_Info", "client thread finish.");

		boost::lock_guard<boost::mutex> lock(_NetLib_mutex); 
		clients_running_threads_count --;	
	}

public:
	thread_withioservice(boost::asio::io_service::work*& virtual_work): client_ioservice(1)
	{
		virtual_work = new boost::asio::io_service::work(client_ioservice);
		clients_running_threads_count ++;
		_thread = boost::thread(boost::bind(&thread_withioservice::run, this));
	}
};

thread_withioservice* th = nullptr; //这个指针是为了访问用的，外部不用管释放，线程会delete this。

boost::asio::io_service::work* client_ioservice_virtual_work = nullptr;

int ioservice_ref_count = 0; //变减到0时会释放一遍ioservice和线程，再增加就要另起

void stop_ioservice()  //need in lock
{	
	if (client_ioservice_virtual_work != nullptr)
	{
		delete client_ioservice_virtual_work;
		client_ioservice_virtual_work = nullptr;

		LOGEVENTL("NetLib_Debug", "virtual_work deleted.");
	}
}

void start_ioservice_ifnot() //need in lock
{
	if (client_ioservice_virtual_work == nullptr) //如果 == nullptr了，说明第一次start，或者之前被stop了。前一个线程可能还在跑，但我们不用管直接起个新的。反正这两个线程上面跑的对象没有交集。
	{
		th = new thread_withioservice(client_ioservice_virtual_work);
	}
}

NETLIB_API NetLib_Client_ptr NetLib_NewClient(std::shared_ptr<NetLib_Client_Delegate> d)
{	
	{
		boost::lock_guard<boost::mutex> lock(_NetLib_mutex); 
	
		clients_running_objs_count ++;
		ioservice_ref_count ++;

		start_ioservice_ifnot(); //先启动线程，否则接下来的NetLib_Client_Imp可能会被意外放到上一个即将结束(work已经释放)的线程上
	}

	return NetLib_Client_ptr(new NetLib_Client_Imp(d, th->client_ioservice),
		[] (NetLib_Client_Imp* c_raw_ptr)
		{
			th->client_ioservice.post(					//这里要post一下是为了让client和delegate的析构彻底在锁外。
				[c_raw_ptr]()							//不用担心ioservice已经无效，因为唯一可能导致释放ioservice的地方stop_ioserivce()在下面，还没执行呢。
				{										//要到下面，ioservice_ref_count减到0了，ioservice才有可能释放
					delete c_raw_ptr;					

					LOGEVENTL("NetLib_Trace", "NetLib_Client (" << log_::h((std::size_t)c_raw_ptr) << ") deconstruction finished");
					
					boost::lock_guard<boost::mutex> lock(_NetLib_mutex); 
					if (--ioservice_ref_count == 0)
					{
						stop_ioservice();
					}

					clients_running_objs_count --;
				}
			);
		}
	);
}

NETLIB_API NetLib_Client_ptr NetLib_NewClient(std::shared_ptr<NetLib_Client_Delegate> d, ioservice_thread* thread)
{
	if (! thread) //thread为空表示用内部的ioservice
	{
		return NetLib_NewClient(d);
	}

	{
		boost::lock_guard<boost::mutex> lock(_NetLib_mutex);
		clients_running_objs_count ++;
	}

	return NetLib_Client_ptr(new NetLib_Client_Imp(d, thread->get_ioservice()),
		[=] (NetLib_Client_Imp* c_raw_ptr)
		{
			thread->get_ioservice().post([c_raw_ptr]()				//这里要post一下是为了让client和delegate的析构彻底在锁外。										
				{										
					delete c_raw_ptr;					

					LOGEVENTL("NetLib_Trace", "NetLib_Client(outer ioservice) (" << log_::h((std::size_t)c_raw_ptr) << ") deconstruction finished.");

					{
						boost::lock_guard<boost::mutex> lock(_NetLib_mutex);
						clients_running_objs_count --;
					}
				}
			);
		}
	);
}

NETLIB_API void NetLib_Clients_WaitForStop()
{
	for (;;)
	{
		bool still_some_threads_running;
		{
			boost::lock_guard<boost::mutex> lock(_NetLib_mutex); 
			still_some_threads_running = clients_running_threads_count != 0 || clients_running_objs_count != 0;
		}
		if (still_some_threads_running)
		{
			boost::this_thread::sleep(boost::posix_time::milliseconds(15));
		}
		else
		{
			break;
		}
	}
}

NETLIB_API NetLib_Server_ptr NetLib_NewServer(std::shared_ptr<NetLib_Server_Delegate> d)
{
	{	
		boost::lock_guard<boost::mutex> lock(_NetLib_mutex); 
		servers_running_objs_count ++;
	}

	ioservice_thread* thread_for_server = new ioservice_thread();
	return NetLib_Server_ptr(new NetLib_Server_Imp(d, *thread_for_server),
		[=](NetLib_Server_Imp* raw_svr_ptr) 
		{
			ioservice_thread* _thread_for_server = thread_for_server;
			boost::thread th(			//其实只要Server用外部的ioservice，就可以不用开线程删。开线程是为了避免出现在ioservice里面删自己的情况
				[=] ()
				{ 
					delete raw_svr_ptr;

					_thread_for_server->wait_for_stop(); //这里应该不会阻塞了，ioservice的队列里面应该已经没东西了，这里只是等几个线程优雅结束而已
					delete _thread_for_server;

					boost::lock_guard<boost::mutex> lock(_NetLib_mutex); 
					servers_running_objs_count --;
				} 
			); 
			th.detach();
		}
	);
}

class NetLib_Server_Delegate_all_session_one_delegate : public NetLib_Server_Delegate
{
protected:
	std::shared_ptr<NetLib_ServerSession_Delegate> session_delegate;

public:
	NetLib_Server_Delegate_all_session_one_delegate(std::shared_ptr<NetLib_ServerSession_Delegate> d) : session_delegate(d)
	{
	}

	NetLib_ServerSession_Delegate* New_SessionDelegate()
	{
		return session_delegate.get();
	}

	void Release_SessionDelegate(NetLib_ServerSession_Delegate* d)
	{
		//这里不用做任何事。外部会保证NetLib_Server_Delegate比所有NetLib_ServerSession_Delegate都要晚释放
	}
};

NETLIB_API NetLib_Server_ptr NetLib_NewServer(std::shared_ptr<NetLib_ServerSession_Delegate> d)
{
	return NetLib_NewServer(std::make_shared<NetLib_Server_Delegate_all_session_one_delegate>(d));
}

NETLIB_API NetLib_Server_ptr NetLib_NewServer(std::shared_ptr<NetLib_ServerSession_Delegate> d, ioservice_thread* thread)
{
	if (! thread)
	{
		return NetLib_NewServer(d);
	}
	else
	{
		return NetLib_NewServer(std::make_shared<NetLib_Server_Delegate_all_session_one_delegate>(d), thread);
	}
}

NETLIB_API NetLib_Server_ptr NetLib_NewServer(std::shared_ptr<NetLib_Server_Delegate> d, ioservice_thread* thread)
{
	if (! thread)
	{
		return NetLib_NewServer(d);
	}

	{	
		boost::lock_guard<boost::mutex> lock(_NetLib_mutex); 
		servers_running_objs_count ++;
	}

	return NetLib_Server_ptr(new NetLib_Server_Imp(d, *thread), 
		[=](NetLib_Server_Imp* raw_svr_ptr) 
		{
			thread->get_ioservice().post( [raw_svr_ptr] ()
				{ 
					delete raw_svr_ptr;

					boost::lock_guard<boost::mutex> lock(_NetLib_mutex); 
					servers_running_objs_count --;
				} 
			); 
		}
	);
}

NETLIB_API void NetLib_Servers_WaitForStop()
{
	for (;;)
	{
		bool still_some_objs_running;
		{
			boost::lock_guard<boost::mutex> lock(_NetLib_mutex); 
			still_some_objs_running = servers_running_objs_count != 0;
		}
		if (still_some_objs_running)
		{
			boost::this_thread::sleep(boost::posix_time::milliseconds(15));
		}
		else
		{
			break;
		}
	}
}

NETLIB_API bool NetLib_CheckVersion(const char* version)
{
	if (strcmp(version, _NETLIB_VERSION_) == 0)
	{
		return true;
	}
	else
	{
		LOGEVENTL("Fatal", "NetLib Version NOT corrected !! got " << _NETLIB_VERSION_ << ", expected " << version);
		return false;
	}
}

#ifndef _STATIC_NETLIB_

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#endif
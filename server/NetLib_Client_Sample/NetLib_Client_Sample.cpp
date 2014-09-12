// tNetLib.cpp : Defines the entry point for the console application.
//

//#include "stdafx.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "NetLib/NetLib.h"
#include <memory.h>
#include <google/protobuf/stubs/common.h>
#include <boost/thread.hpp>
#include "interactive_input.h"
#include "utility1.h"
#include "Log/Log.h"

#define _TEST_SIZE (4000)
#define _TEST_SIZE2 (6000)
#define _SMALL_TEST_SIZE (30)

NetLib_Client_ptr client;

class MyClientDelegate : public NetLib_Client_Delegate
{
public:
	~MyClientDelegate()
	{
		LOGEVENTL("Debug", "Delegate released");	
	}
	void ConnectedHandler(NetLib_Client_ptr clientptr)
	{
		LOGEVENTL("Info", "connected");	
		//
		int i;

		char* data = new char[_TEST_SIZE];

		int cmd = (std::size_t)(this) % 120 + 1;

		for (i = 8; i < _TEST_SIZE; ++i)
		{
			data[i] = i % cmd; //用于测试正确性
		}
		*(uint32_t*)data = _TEST_SIZE; //最头上4字节是整个包的长度(包含这4个字节)
		*(uint32_t*)(data + 4) = cmd;
		
		LOGEVENTLSTR("Debug", "on connect, send data: 0x" << std::hex << (std::size_t)data);
		clientptr->SendAsync(data);
	}

	void SendFinishHandler(NetLib_Client_ptr clientptr, char* data, void* pHint)
	{
		LOGEVENTLSTR("Debug", "send finish. data: 0x" << std::hex << (std::size_t)data << ", Hint: " << std::dec << (std::size_t)pHint);
		delete[] data;
	}	
	
	void SendCopyFinishHandler(NetLib_Client_ptr clientptr, char* data, void* pHint)
	{
		LOGEVENTLSTR("Debug", "sendcopy finish. data: 0x" << std::hex << (std::size_t)data << ", Hint: " << std::dec << (std::size_t)pHint);
	}

	void RecvFinishHandler(NetLib_Client_ptr clientptr, char* data)
	{
		//data在RecvFinishHandler返回后会释放

		uint32_t size = *(uint32_t*)data;

		
		if (size >= 8)
		{
			uint32_t cmd = *(uint32_t*)(data + 4);

			if (cmd == 240)
			{
				char* data_cpy = new char[size];
				memcpy(data_cpy, data, size);
				
				clientptr->SendAsync(data_cpy, (void*)567);

				printf("test: send back again\n");
			}
			else
			{
				bool correct = true;
				int i;
				for (i = 8; i < size; ++i)
				{
					if (data[i] != i % cmd) 
					{
						correct = false;
						break;
					}
				}
				if (correct)
				{
					printf("data correct.\n");
				}
				else
				{
					printf("data WRONG!!!! (%d)\n", i);
				}
			}
		}
		else
		{
			printf("size less than 8.\n");
		}
	}

	void ReconnectedHandler(NetLib_Client_ptr clientptr)
	{	
		//重连成功
		LOGEVENTL("Info", "reconnect success !");
	}

	void ReconnectFailedHandler(NetLib_Client_ptr clientptr, bool& will_continue_reconnect)
	{
		if (will_continue_reconnect)
		{
			printf("reconnecting...\n");
		}
	}

	void DisconnectedHandler(NetLib_Client_ptr clientptr, NetLib_Error error, int error_code, bool& will_reconnect)
	{
		if (error == close_by_local)
		{
			LOGEVENTL("Info", "disconnected by local.");
		}
		else
		{
			LOGEVENTL("Info", "disconnect: " << error << ", " << error_code);
			will_reconnect = false;
			/*if (will_reconnect)
			{
				printf("reconnecting..\n");
			}*/
		}
	}

	bool SendFailedHandler(NetLib_Client_ptr clientptr, const char* data, void* pHint)
	{
		LOGEVENTL("Debug", "send failed. data: " << hex((std::size_t)data) << ", Hint: " << (std::size_t)pHint);
		//delete[] data; 返回true了就不用delete
		return true;
	}

	bool SendCopyFailedHandler(NetLib_Client_ptr clientptr, const char* data_copy, void* pHint)
	{
		LOGEVENTL("Debug", "send copy failed. data: " << hex((std::size_t)data_copy) << ", Hint: " << (std::size_t)pHint);
		return true;
	}
};

char destip[100];
int tcpport = 1248;
int udpport = 2012;

#ifdef _STATIC_NETLIB_

class Test1_Delegate : public NetLib_Client_Delegate
{
protected:
	boost::asio::deadline_timer* timer;

	void SendSomething(NetLib_Client_ptr clientptr)
	{
		int i;
		char* data = new char[_TEST_SIZE];

		int cmd = (std::size_t)(this) % 120 + 1;

		for (i = 8; i < _TEST_SIZE; ++i)
		{
			data[i] = i % cmd; //用于测试正确性
		}
		*(uint32_t*)data = _TEST_SIZE; //最头上4字节是整个包的长度(包含这4个字节)
		*(uint32_t*)(data + 4) = cmd;

		clientptr->SendAsync(data);
	}

public:
	Test1_Delegate() : timer(nullptr) {}

	void call_SendSomething(NetLib_Client_ptr clientptr, const boost::system::error_code& error)
	{
		if (!error)
		{
			SendSomething(clientptr);
		}
	}

	void ConnectedHandler(NetLib_Client_ptr clientptr)
	{
		LOGEVENTL("Debug", "(test1) connected.");
		timer = new boost::asio::deadline_timer(*clientptr->GetWorkIoService());
		SendSomething(clientptr);
	}

	void SendFinishHandler(NetLib_Client_ptr clientptr, char* data, void* pHint)
	{
		//LOGEVENTL("Debug", "send finish. data: 0x" << std::hex << (std::size_t)data << ", Hint: " << std::dec << (std::size_t)pHint);
		delete[] data;
		timer->expires_from_now(boost::posix_time::milliseconds(100));
		timer->async_wait(boost::bind(&Test1_Delegate::call_SendSomething, this, clientptr, boost::asio::placeholders::error));
	}

	void RecvFinishHandler(NetLib_Client_ptr clientptr, char* data)
	{
		//data在RecvFinishHandler返回后会释放

		uint32_t size = *(uint32_t*)data;

		
		if (size >= 8)
		{
			uint32_t cmd = *(uint32_t*)(data + 4);

			if (cmd == 240)
			{
				char* data_cpy = new char[size];
				memcpy(data_cpy, data, size);
				
				clientptr->SendAsync(data_cpy, (void*)567);

				printf("test: send back again\n");
			}
			else
			{
				bool correct = true;
				int i;
				for (i = 8; i < size; ++i)
				{
					if (data[i] != i % cmd) 
					{
						correct = false;
						break;
					}
				}
				if (correct)
				{
					//printf("data correct.\n");
				}
				else
				{					
					LOGEVENTL("Debug", "DATA WRONG. pos:" << i);
				}
			}
		}
		else
		{
			printf("size less than 8.\n");
		}
	}

	void ReconnectedHandler(NetLib_Client_ptr clientptr)
	{	
		//重连成功
		SendSomething(clientptr);
	}

	void DisconnectedHandler(NetLib_Client_ptr clientptr, NetLib_Error error, int error_code, bool& will_reconnect)
	{
		if (error == close_by_local)
		{
			LOGEVENTL("Info", "(test1) disconnected by local.");
			if (timer)
			{
				delete timer;
			}
		}
		else
		{
			LOGEVENTL("Info", "(test1) disconnect: " << error << ", " << error_code);
			if (will_reconnect)
			{
				LOGEVENTL("Debug", "(test1) reconnecting..");
			}
			else
			{
				if (timer)
				{
					delete timer;
				}
			}
		}
	}

	bool SendFailedHandler(NetLib_Client_ptr clientptr, const char* data, void* pHint)
	{
		LOGEVENTLSTR("Debug", "send failed. data: 0x" << std::hex << (std::size_t)data << ", Hint: " << (std::size_t)pHint);
		//delete[] data; 返回true了就不用delete
		return true;
	}

	bool SendCopyFailedHandler(NetLib_Client_ptr clientptr, const char* data_copy, void* pHint)
	{
		LOGEVENTLSTR("Debug", "send copy failed. data: 0x" << std::hex << (std::size_t)data_copy << ", Hint: " << (std::size_t)pHint);
		return true;
	}
};

#endif

class Test2_Delegate : public NetLib_Client_Delegate
{
protected:
	void SendSomething(NetLib_Client_ptr clientptr)
	{
		int i;
		char* data = new char[_TEST_SIZE];

		int cmd = (std::size_t)(this) % 120 + 1;

		for (i = 8; i < _TEST_SIZE; ++i)
		{
			data[i] = i % cmd; //用于测试正确性
		}
		*(uint32_t*)data = _TEST_SIZE; //最头上4字节是整个包的长度(包含这4个字节)
		*(uint32_t*)(data + 4) = cmd;

		clientptr->SendAsync(data);
	}

public:
	void ConnectedHandler(NetLib_Client_ptr clientptr)
	{
		LOGEVENTL("Debug", "(test2) connected.");
		SendSomething(clientptr);
	}

	void SendFinishHandler(NetLib_Client_ptr clientptr, char* data, void* pHint)
	{
		//LOGEVENTL("Debug", "send finish. data: 0x" << std::hex << (std::size_t)data << ", Hint: " << std::dec << (std::size_t)pHint);
		delete[] data;
	}

	void RecvFinishHandler(NetLib_Client_ptr clientptr, char* data)
	{
		//data在RecvFinishHandler返回后会释放

		uint32_t size = *(uint32_t*)data;

		
		if (size >= 8)
		{
			uint32_t cmd = *(uint32_t*)(data + 4);

			if (cmd == 240)
			{
				char* data_cpy = new char[size];
				memcpy(data_cpy, data, size);
				
				clientptr->SendAsync(data_cpy, (void*)567);

				printf("test: send back again\n");
			}
			else
			{
				bool correct = true;
				int i;
				for (i = 8; i < size; ++i)
				{
					if (data[i] != i % cmd) 
					{
						correct = false;
						break;
					}
				}
				if (correct)
				{
					//printf("data correct.\n");
				}
				else
				{					
					LOGEVENTL("Debug", "DATA WRONG. pos:" << i);
				}
			}
		}
		else
		{
			printf("size less than 8.\n");
		}
	}

	void ReconnectedHandler(NetLib_Client_ptr clientptr)
	{	
		//重连成功
		SendSomething(clientptr);
	}

	void DisconnectedHandler(NetLib_Client_ptr clientptr, NetLib_Error error, int error_code, bool& will_reconnect)
	{
		if (error == close_by_local)
		{
			LOGEVENTL("Info", "(test2) disconnected by local.");
		}
		else
		{
			LOGEVENTL("Info", "(test2) disconnect: " << error << ", " << error_code);
			if (will_reconnect)
			{
				LOGEVENTL("Debug", "(test2) reconnecting..");
			}
		}
	}

	bool SendFailedHandler(NetLib_Client_ptr clientptr, const char* data, void* pHint)
	{
		LOGEVENTLSTR("Debug", "send failed. data: 0x" << std::hex << (std::size_t)data << ", Hint: " << (std::size_t)pHint);
		//delete[] data; 返回true了就不用delete
		return true;
	}

	bool SendCopyFailedHandler(NetLib_Client_ptr clientptr, const char* data_copy)
	{
		LOGEVENTLSTR("Debug", "send copy failed. data: 0x" << std::hex << (std::size_t)data_copy);
		return true;
	}
};

NetLib_Client_ptr* test1_client_ptrs;
int test1_conn_count = 500;

void test1(bool test2 = false)
{
	test1_client_ptrs = new NetLib_Client_ptr[test1_conn_count];
	for (int i = 0; i < test1_conn_count; ++i)
	{
		if (test2)
		{
			test1_client_ptrs[i] = NetLib_NewClient(std::shared_ptr<Test2_Delegate>(new Test2_Delegate));
		}
		else
		{
#ifdef _STATIC_NETLIB_
			test1_client_ptrs[i] = NetLib_NewClient(std::shared_ptr<Test1_Delegate>(new Test1_Delegate));
#endif
		}

		if (i % 2 == 0)
		{
			test1_client_ptrs[i]->ConnectAsync(destip, tcpport, udpport);
		}
		else
		{
			test1_client_ptrs[i]->ConnectAsyncTCP(destip, tcpport);
		}
	}
}

void test1_stop()
{
	for (int i = 0; i < test1_conn_count; ++i)
	{
		test1_client_ptrs[i]->Disconnect();
	}
	delete[] test1_client_ptrs;
}

bool test3tcp = false;
int test3_conn_count, test3_interval, test3_packet_size, test3_packet_count_one_time, test3_timeout_packet_count;

int test3_packet_send_count;
int test3_packet_send_finish_count;

NetLib_Client_ptr* test3_client_ptrs;

#ifdef _STATIC_NETLIB_

#include "../include/ptime2.h"

//#define SEND_DESIGNED_DATA

//#define CHECK_IS_CORRECT

//#define LOG_DETAIL

#define DIST_COUNT (20)

int distribution[DIST_COUNT + 1];

class Test3_Delegate : public NetLib_Client_Delegate
{
protected:
	boost::asio::deadline_timer timer;
	int need_reply_count;

	void SendSomething(NetLib_Client_ptr clientptr)
	{
		if (need_reply_count >= test3_timeout_packet_count)
		{
			LOGEVENTL("ClientTimeout", log_::n("ptr") << hex((std::size_t)clientptr.get()) << log_::n("need_reply_count") << need_reply_count);
			clientptr->Disconnect();
		}
		else
		{			
			timer.expires_from_now(boost::posix_time::seconds(test3_interval));
			timer.async_wait(boost::bind(&Test3_Delegate::call_SendSomething, this, clientptr, boost::asio::placeholders::error));

			for (int x = 0; x < test3_packet_count_one_time; ++x)
			{
				int i;
				char* data = new char[test3_packet_size];

				*(uint32_t*)data = test3_packet_size; //最头上4字节是整个包的长度(包含这4个字节)
				*(uint32_t*)(data + 4) = 230; //表示测试延时
				//+8 这个位置可以放个id，暂时没用
				*(uint64_t*)(data + 12) = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();

#ifdef LOG_DETAIL
				LOGEVENTL("SendAsync", log_::n("ptr") << hex((std::size_t)clientptr.get()));
#endif

				clientptr->SendAsync(data);
			}

			need_reply_count += test3_packet_count_one_time;

			test3_packet_send_count += test3_packet_count_one_time;

			if (test3_packet_send_count % 1000 == 0)
			{
				LOGEVENTL("SendCount", test3_packet_send_count);
			}
		}
	}

public:
	Test3_Delegate(boost::asio::io_service& ioservice) : timer(ioservice), need_reply_count(0) {}

	void call_SendSomething(NetLib_Client_ptr clientptr, const boost::system::error_code& error)
	{
		if (!error)
		{
			SendSomething(clientptr);
		}
	}

	void ConnectedHandler(NetLib_Client_ptr clientptr)
	{
		SendSomething(clientptr);
	}

	void SendFinishHandler(NetLib_Client_ptr clientptr, char* data, void* pHint)
	{
		test3_packet_send_finish_count ++;
		if (test3_packet_send_finish_count % 1000 == 0)
		{
			LOGEVENTL("SendFinishCount", test3_packet_send_finish_count);
		}
		//LOGEVENTL("Debug", "send finish. data: 0x" << std::hex << (std::size_t)data << ", Hint: " << std::dec << (std::size_t)pHint);
		delete[] data;
	}

	void RecvFinishHandler(NetLib_Client_ptr clientptr, char* data)
	{
		uint32_t size = *(uint32_t*)data;
		
		if (size >= 8)
		{
			need_reply_count --;

			uint32_t cmd = *(uint32_t*)(data + 4);
			
#ifdef LOG_DETAIL
			LOGEVENTL("RecvFinish", log_::n("ptr") << hex((std::size_t)clientptr.get()));
#endif

			if (cmd == 230 && size >= 20) //Round Trip Time
			{
				uint64_t duration = (ptime2(boost::posix_time::microsec_clock::local_time()).get_u64() - *(uint64_t*)(data + 12)) / 1000; //微秒除1000，转为毫秒
				if (duration >= DIST_COUNT * 10)
				{
					distribution[DIST_COUNT] ++;
				}
				else
				{
					distribution[duration / 10] ++;
				}
			}
		}
		else
		{
			LOGEVENTL("Error", "size less than 8.");
		}
	}

	void ReconnectedHandler(NetLib_Client_ptr clientptr)
	{	
		//重连成功
		SendSomething(clientptr);
	}

	void DisconnectedHandler(NetLib_Client_ptr clientptr, NetLib_Error error, int error_code, bool& will_reconnect)
	{
		if (error == close_by_local)
		{
			LOGEVENTL("ClientDisconnect", "(test3) disconnected by local. " << log_::n("ptr") << hex((std::size_t)clientptr.get()));
		}
		else
		{
			LOGEVENTL("ClientDisconnect", "(test3) disconnect. " << log_::n("error") << error << log_::n("inner_error") << error_code << log_::n("ptr") << hex((std::size_t)clientptr.get()));
			if (will_reconnect)
			{
				LOGEVENTL("Info", "will reconnect.. " << log_::n("ptr") << hex((std::size_t)clientptr.get()));
			}
		}
		
		if (! will_reconnect)
		{
			boost::system::error_code ignored_error;
			timer.cancel(ignored_error);
		}
	}
};

std::shared_ptr<boost::asio::deadline_timer> show_dist_timer;

void show_dist_later();

void show_dist(const boost::system::error_code& error)
{
	if (!error)
	{
		LogStream ___ls("Dist");
		for (int i = 0; i < DIST_COUNT; ++i)
		{
			___ls << log_::n("<" + utility1::int2str((i + 1) * 10) + "ms") << distribution[i];
		}
		___ls << log_::n(">=" + utility1::int2str(DIST_COUNT * 10) + "ms") << distribution[DIST_COUNT];
	
		___ls.Log(true);

		memset(distribution, 0, sizeof(distribution));

		show_dist_later();
	}
}

#define SHOW_DIST_INTERVAL (5)

void show_dist_later()
{
	boost::system::error_code ignored_error;
	show_dist_timer->expires_from_now(boost::posix_time::seconds(SHOW_DIST_INTERVAL), ignored_error);
	show_dist_timer->async_wait(boost::bind(&show_dist, boost::asio::placeholders::error));
}

#endif

void test3()
{	
#ifdef _STATIC_NETLIB_
	test3_packet_send_count = 0;
	test3_packet_send_finish_count = 0;
	std::string title;
	if (test3tcp)
	{
		title = "Test3TCP";
	}
	else
	{
		title = "Test3UDP";
	}
	if (test3_packet_size < 20)
	{
		LOGEVENTL("Warn", log_::n("PacketSize") << test3_packet_size << " is too small, change to 20.");
		test3_packet_size = 20;
	}
	LOGEVENTL(title.c_str(), log_::n("ConnCount") << test3_conn_count << log_::n("SendInterval") << test3_interval 
				<< log_::n("PacketSize") << test3_packet_size << log_::n("PacketsCountPerTime") << test3_packet_count_one_time << log_::n("TimeoutPacketsCount") << test3_timeout_packet_count);

	test3_client_ptrs = new NetLib_Client_ptr[test3_conn_count];

	memset(distribution, 0, sizeof(distribution));

	show_dist_timer = std::make_shared<boost::asio::deadline_timer> (*(client->GetWorkIoService()));
	
	show_dist_later();

	int i;
	for (i = 0; i < test3_conn_count; ++i)
	{
		test3_client_ptrs[i] = NetLib_NewClient(std::make_shared<Test3_Delegate>(*(client->GetWorkIoService())));
		test3_client_ptrs[i]->DisableReconnect();
	}

	int conns_per_hundred_milliseconds = test3_conn_count / (test3_interval * 10);
	int remainder = test3_conn_count % conns_per_hundred_milliseconds;
	if (conns_per_hundred_milliseconds)
	{
		for (i = 0; i < test3_conn_count / conns_per_hundred_milliseconds; ++i)
		{
			boost::posix_time::ptime t1 = boost::posix_time::microsec_clock::local_time();
			for (int j = 0; j < conns_per_hundred_milliseconds; ++j)
			{
				if (test3tcp)
				{
					test3_client_ptrs[i * conns_per_hundred_milliseconds + j]->ConnectAsyncTCP(destip, tcpport);
				}
				else
				{
					test3_client_ptrs[i * conns_per_hundred_milliseconds + j]->ConnectAsyncUDP(destip, udpport);
				}
			}
			boost::this_thread::sleep(boost::posix_time::milliseconds(100) - (boost::posix_time::microsec_clock::local_time() - t1));
		}
	}
	for (int j = 0; j < remainder; ++j)
	{
		if (test3tcp)
		{
			test3_client_ptrs[test3_conn_count - remainder + j]->ConnectAsyncTCP(destip, tcpport);
		}
		else
		{
			test3_client_ptrs[test3_conn_count - remainder + j]->ConnectAsyncUDP(destip, udpport);
		}
	}

	LOGEVENTL("Info", "All " << test3_conn_count << " Connections have been called ConnectAsync");

#endif
}

void test3_stop()
{
#ifdef _STATIC_NETLIB_
	show_dist_timer->cancel();
	for (int i = 0; i < test3_conn_count; ++i)
	{
		test3_client_ptrs[i]->Disconnect();
	}
	delete[] test3_client_ptrs;
#endif
}

#define TEST5_PACKET_SIZE (50)

int test5_conn_count = 5000;
int test5_interval = 50;
int test5_interval2 = 1;

NetLib_Client_ptr* test5_client_ptrs;

#ifdef _STATIC_NETLIB_
class Test5_Delegate : public NetLib_Client_Delegate
{
protected:
	boost::asio::deadline_timer timer;

	void SendSomething(NetLib_Client_ptr clientptr)
	{
		int i;
		char* data = new char[TEST5_PACKET_SIZE];

		int cmd = (std::size_t)(this) % 120 + 1;

		for (i = 8; i < TEST5_PACKET_SIZE; ++i)
		{
			data[i] = i % cmd; //用于测试正确性
		}
		*(uint32_t*)data = TEST5_PACKET_SIZE; //最头上4字节是整个包的长度(包含这4个字节)
		*(uint32_t*)(data + 4) = cmd;

		clientptr->SendAsync(data);
				
		timer.expires_from_now(boost::posix_time::milliseconds(test5_interval));
		timer.async_wait(boost::bind(&Test5_Delegate::do_disconnect, this, clientptr, boost::asio::placeholders::error));
	}

public:
	Test5_Delegate(boost::asio::io_service& ioservice) : timer(ioservice) {}

	void do_disconnect(NetLib_Client_ptr clientptr, const boost::system::error_code& error)
	{
		if (!error)
		{
			clientptr->Disconnect();
		}
	}

	void do_connect(NetLib_Client_ptr clientptr, const boost::system::error_code& error)
	{
		if (!error)
		{
			clientptr->ConnectAsyncTCP(destip, tcpport);
		}
	}

	void ConnectedHandler(NetLib_Client_ptr clientptr)
	{
		SendSomething(clientptr);
	}

	void SendFinishHandler(NetLib_Client_ptr clientptr, char* data, void* pHint)
	{
		//LOGEVENTL("Debug", "send finish. data: 0x" << std::hex << (std::size_t)data << ", Hint: " << std::dec << (std::size_t)pHint);
		delete[] data;
	}

	void RecvFinishHandler(NetLib_Client_ptr clientptr, char* data)
	{
	}

	void ReconnectedHandler(NetLib_Client_ptr clientptr)
	{	
		//重连成功
		SendSomething(clientptr);
	}

	void DisconnectedHandler(NetLib_Client_ptr clientptr, NetLib_Error error, int error_code, bool& will_reconnect)
	{		
		timer.expires_from_now(boost::posix_time::milliseconds(test5_interval2));
		timer.async_wait(boost::bind(&Test5_Delegate::do_connect, this, clientptr, boost::asio::placeholders::error));
	}
};
#endif

void test5()
{	
#ifdef _STATIC_NETLIB_
	LOGEVENTL("Test5", log_::n("ConnCount") << test5_conn_count << log_::n("IntervalBetweenSendAndDisconnect") << test5_interval << log_::n("IntervalBetweenDisconnectAndConnect") << test5_interval2);

	test5_client_ptrs = new NetLib_Client_ptr[test5_conn_count];

	int i;
	for (i = 0; i < test5_conn_count; ++i)
	{
		test5_client_ptrs[i] = NetLib_NewClient(std::make_shared<Test5_Delegate>(*(client->GetWorkIoService())));
		test5_client_ptrs[i]->DisableReconnect();
	}

	int conns_per_hundred_milliseconds;
	int remainder = 0;
	if (test5_interval < 100)
	{
		conns_per_hundred_milliseconds = 2000;
		remainder = test5_conn_count - test5_conn_count / conns_per_hundred_milliseconds * 2000;
	}
	else
	{
		conns_per_hundred_milliseconds = test5_conn_count / (test5_interval / 100);
		remainder = test5_conn_count - conns_per_hundred_milliseconds * (test5_interval * 10);
	}
	if (conns_per_hundred_milliseconds)
	{
		for (i = 0; i < test5_conn_count / conns_per_hundred_milliseconds; ++i)
		{
			boost::posix_time::ptime t1 = boost::posix_time::microsec_clock::local_time();
			for (int j = 0; j < conns_per_hundred_milliseconds; ++j)
			{
				test5_client_ptrs[i * conns_per_hundred_milliseconds + j]->ConnectAsyncTCP(destip, tcpport);
			}
			
			boost::this_thread::sleep(boost::posix_time::milliseconds(100) - (boost::posix_time::microsec_clock::local_time() - t1));
		}
	}
	for (int j = 0; j < remainder; ++j)
	{
		test5_client_ptrs[test5_conn_count - remainder + j]->ConnectAsyncTCP(destip, tcpport);
	}
#endif
}

void send(int hint = 55)
{
	int i;

	char* data = new char[_TEST_SIZE];
	for (i = 8; i < _TEST_SIZE; ++i)
	{
		data[i] = i % 77; //用于测试正确性
	}
	*(uint32_t*)data = _TEST_SIZE; //最头上4字节是整个包的长度(包含这4个字节)
	*(uint32_t*)(data + 4) = 77;

	client->SendAsync(data, (void*)hint);
}

void continuesend(int packet_count, int interval)
{
	for (int i = 0; i < packet_count; ++i)
	{
		send(i);
		boost::this_thread::sleep(boost::posix_time::milliseconds(interval));
	}
}

void test4(int transfer_size_MBytes)
{
	uint32_t packet_size = 1024;
	char* d = new char[packet_size];
	memset(d, 1, packet_size);
	*(uint32_t*)d = packet_size;
	*(uint32_t*)(d + 4) = 245;

	LOGEVENTL("Test4", log_::n("size") << transfer_size_MBytes);

	int t = transfer_size_MBytes * 1024 * 1024 / packet_size;
	int i;
	for (i = 0; i < t; ++i)
	{
		client->SendCopyAsync(d); //TODO: 改成非拷贝版的，弄个专门的delegate
		boost::this_thread::sleep(boost::posix_time::milliseconds(30));
	}
	*(uint32_t*)d = transfer_size_MBytes * 1024 * 1024 % packet_size;
	client->SendAsync(d);
}

#include <string.h>

/*
测试列表
……
连续多次sendsmall (current_netlib_packet_data_size相关)

*/

#include <set>

std::string test6_dest_ip;
int test6_begin_port;
int test6_end_port;
bool test6_send_something_when_connected;

#define TEST6_PACKET_SIZE (48)

class Test6_Delegate : public NetLib_Client_Delegate
{
protected:
	int m_remote_port;

public:
	Test6_Delegate(int remote_port) : m_remote_port(remote_port)
	{
	}

	void ConnectedHandler(NetLib_Client_ptr clientptr)
	{
		LOGEVENTL("Connected", log_::n("RemotePort") << m_remote_port);

		if (test6_send_something_when_connected)
		{
			int i;

			char* data = new char[TEST6_PACKET_SIZE];

			int cmd = (std::size_t)(this) % 120 + 1;

			for (i = 8; i < TEST6_PACKET_SIZE; ++i)
			{
				data[i] = i % cmd; //用于测试正确性
			}
			*(uint32_t*)data = TEST6_PACKET_SIZE; //最头上4字节是整个包的长度(包含这4个字节)
			*(uint32_t*)(data + 4) = cmd;

			clientptr->SendAsync(data);
		}
	}

	void RecvFinishHandler(NetLib_Client_ptr clientptr, char* data)
	{
		//data在RecvFinishHandler返回后会释放

		uint32_t size = *(uint32_t*)data;

		
		if (size >= 8)
		{
			uint32_t cmd = *(uint32_t*)(data + 4);

			bool correct = true;
			int i;
			for (i = 8; i < size; ++i)
			{
				if (data[i] != i % cmd) 
				{
					correct = false;
					break;
				}
			}
			if (correct)
			{
				LOGEVENTL("Recv_OK", log_::n("RemotePort") << m_remote_port);
			}
			else
			{
				LOGEVENTL("Recv_Wrong", log_::n("RemotePort") << m_remote_port << log_::n("wrong_pos") << i);
			}
		}
		else
		{
			LOGEVENTL("Recv_Wrong", "size less than 8. " << _ln("RemotePort") << m_remote_port);
		}
	}

	void DisconnectedHandler(NetLib_Client_ptr clientptr, NetLib_Error error, int error_code, bool& will_reconnect)
	{
		if (error == close_by_local)
		{
			LOGEVENTL("Disconnected", "by local. " << _ln("RemotePort") << m_remote_port);
		}
		else
		{
			LOGEVENTL("Disconnected", _ln("error") << error << _ln("error_code") << error_code << _ln("RemotePort") << m_remote_port);
		}
	}
};

NetLib_Client_ptr* test6_client_ptrs;

void test6()
{
	int test6_conn_count = test6_end_port - test6_begin_port + 1;
	test6_client_ptrs = new NetLib_Client_ptr[test6_conn_count];

	int p, j = 0;
	for (p = test6_begin_port; p <= test6_end_port; ++p)
	{
		test6_client_ptrs[j] = NetLib_NewClient(std::make_shared<Test6_Delegate>(p));
		test6_client_ptrs[j]->DisableReconnect();
		test6_client_ptrs[j]->ConnectAsyncTCP(test6_dest_ip.c_str(), p);

		++j;
		if (j % 10 == 0)
		{
			boost::this_thread::sleep(boost::posix_time::milliseconds(10));
		}
	}
}

void test6stop()
{	
	int test6_conn_count = test6_end_port - test6_begin_port + 1;
	for (int j = 0; j < test6_conn_count; ++j)
	{
		test6_client_ptrs[j]->Disconnect();
	}
	delete[] test6_client_ptrs;
}

int test7_conn_count;
int test7_thread_count;
int test7_packet_size;

ioservice_thread* test7_threads;

void test7_thread(int conn_count)
{	
#ifdef _STATIC_NETLIB_
	std::cout << "[" << boost::posix_time::microsec_clock::local_time() << "] A Test7 Thread Start" << std::endl;

	NetLib_Client_ptr* test7_client_ptrs;
	int p, j = 0;
	for (p = test6_begin_port; p <= test6_end_port; ++p)
	{
		test6_client_ptrs[j] = NetLib_NewClient(std::make_shared<Test6_Delegate>(p));
		test6_client_ptrs[j]->DisableReconnect();
		test6_client_ptrs[j]->ConnectAsyncTCP(test6_dest_ip.c_str(), p);

		++j;
		if (j % 10 == 0)
		{
			boost::this_thread::sleep(boost::posix_time::milliseconds(10));
		}
	}
#endif
}

void test7()
{
	/*int conn_per_thread = test7_conn_count / test7_thread_count;
	int conn_remainder = test7_conn_count % test7_thread_count;

	test7_threads = new ioservice_thread[test7_thread_count];
	
	int i;
	for (i = 0; i < test7_thread_count; ++i)
	{
		test7_threads[i]->start();
	}
	for (i = 0; i < test7_thread_count; ++i)
	{
		if (i < conn_remainder)
		{
			test7_threads[i]->get_ioservice().post(boost::bind(&test7_thread, conn_per_thread + 1));
		}
		else
		{
			test7_threads[i]->get_ioservice().post(boost::bind(&test7_thread, conn_per_thread));
		}
	}*/
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

	NETLIB_CHECK_VERSION;

#ifndef _STATIC_NETLIB_
	NetLib_Initialize();
#endif

    LogInitializeLocalOptions(true, true, "client_sample");
	
#ifdef _DEBUG
	//NetLib_Set_UDP_ResendIntervalMS(10 * 1000);         //
	//NetLib_Set_UDP_MaxResendIntervalMS(10 * 60 * 1000); //这两行参数设置，仅在调试时使用，避免一打断点就超时，生产场合请使用默认值，或者不要设置这么大的超时时间
#endif
	
	if (argc > 1)
	{
		strcpy(destip, argv[1]);
	}
	else
	{
		strcpy(destip, "127.0.0.1");
	}

	client = NetLib_NewClient(std::shared_ptr<MyClientDelegate>(new MyClientDelegate));
	
	for (;;)
	{
		std::string tmp;
		if (! std::getline(std::cin, tmp))
		{
#ifdef WIN32
			::Sleep(60000);
#else
			sleep(60);
#endif
		}

		std::set<NetLib_Client_ptr> old_ptrs;

		if (tmp == "new")
		{
			client = NetLib_NewClient(std::shared_ptr<MyClientDelegate>(new MyClientDelegate));
			//client->ConnectAsync("127.0.0.1", 1248, 2012); 
		}
		else if (tmp == "noreleasenew")
		{
			old_ptrs.insert(client);
			client = NetLib_NewClient(std::shared_ptr<MyClientDelegate>(new MyClientDelegate));
		}
		else if (tmp == "dest")
		{
			strcpy(destip, InputStr("DestinationIP", "10.0.18.31").c_str());
		}
		else if (tmp == "tcpport")
		{
			tcpport = Input("TCPPort", 1248);
		}
		else if (tmp == "udpport")
		{
			udpport = Input("UDPPort", 2012);
		}
		else if (tmp == "connecttcpport")
		{
			int port = Input("TempTCPPort", 11248);
			client->ConnectAsyncTCP(destip, port);
		}
		else if (tmp == "test1") //连上后定期发包，断了会重连
		{
			test1_conn_count = Input("Test1ConnectionCount", 1000);
			test1();
		}
		else if (tmp == "test1stop")
		{
			test1_stop();
		}
		else if (tmp == "test2") //连上后发一个包，断了会重连
		{
			test1_conn_count = Input("Test2ConnectionCount", 1000);
			test1(true);
		}
		else if (tmp == "test2stop")
		{
			test1_stop(); //test1和test2比较类似，就共用这部分代码了
		}
		else if (tmp == "test3")
		{
			test3tcp = true;
			test3_conn_count = Input("ConnectionCount", 15000);
			test3_interval = Input("SendInterval(second)", 15);
			test3_packet_size = Input("PacketSize(Byte)", 32);
			test3_packet_count_one_time = Input("PacketCountOneTime", 1);
			test3_timeout_packet_count = Input("TimeoutPacketsCount", 2);
			test3();
		}
		else if (tmp == "test3tcp") //连上后定期发指定数量的包。断开后不自动重连。缺K个包即认为该连接超时。带统计延时分布
		{
			test3tcp = false;
			test3_conn_count = Input("ConnectionCount", 15000);
			test3_interval = Input("SendInterval(second)", 15);
			test3_packet_size = Input("PacketSize(Byte)", 32);
			test3_packet_count_one_time = Input("PacketCountOneTime", 1);
			test3_timeout_packet_count = Input("TimeoutPacketsCount", 2);
			test3();
		}
		else if (tmp == "test3stop")
		{
			test3_stop();
		}
		else if (tmp == "test4") //发送大数据
		{
			int data_size = Input("DataSize(MegaBytes)", 16);
			test4(data_size);
		}
		else if (tmp == "test5") //连上发包，过会儿断，再过会儿又连
		{
			test5_conn_count = Input("ConnectionCount", 5000);
			test5_interval = Input("IntervalBetweenSendAndDisconnect(ms)", 0);
			test5_interval2 = Input("IntervalBetweenDisconnectAndConnect(ms)", 1000);
			test5();
		}
		else if (tmp == "test6") //扫端口，连上之后可以选择发个包（默认发）
		{
			test6_dest_ip = InputStr("Test6DestIP", "101.226.247.18");
			test6_begin_port = Input("Test6BeginPort", 27);
			test6_end_port = Input("Test6EndPort", 10000);
			test6_send_something_when_connected = Input("SendSomethingWhenConnected", true);

			test6();
		}
		else if (tmp == "test6stop")
		{
			test6stop();
		}
		else if (tmp == "test7") //tcp短连接
		{
			test7_conn_count = Input("ConnectionCount", 4000);
			test7_thread_count = Input("ThreadCount", 8);
			test7_packet_size = Input("PacketSize", 250);

			test7();
		}
		else if (tmp == "wait")
		{
			NetLib_Clients_WaitForStop();
			LOGEVENTL("Debug", "Wait Finish");
		}
		else if (tmp == "exit")
		{
			break;
		}
		else if (tmp == "ptr")
		{
			if (client)
			{
				printf("valid\n");
			}
			else
			{
				printf("empty\n");
			}
		}
		else if (tmp == "releasenew")
		{
			client->Disconnect();
			client = NetLib_NewClient(std::shared_ptr<MyClientDelegate>(new MyClientDelegate));
		}
		else if (tmp == "keepalivec")
		{
			client->SetKeepAliveIntervalSeconds(4);
			client->ConnectAsync(destip, tcpport, udpport, NETLIB_CLIENT_FLAG_KEEP_ALIVE);			
		}
		else if (tmp == "keepalivecc")
		{
			client->SetKeepAliveIntervalSeconds(4);
			client->ConnectAsyncTCP(destip, tcpport, NETLIB_CLIENT_FLAG_KEEP_ALIVE);
		}
		else if (tmp == "isconnected")
		{
			if (client->IsConnected())
			{
				printf("YES\n");
			}
			else
			{
				printf("NO\n");
			}
		}
		else if (tmp == "closeconnect")
		{
			client->Disconnect();
			client->ConnectAsync(destip, tcpport, udpport); 
		}
		else if (tmp == "badreconnecttcp")
		{
			client->ConnectAsyncTCP(destip, 11248, NETLIB_CLIENT_ENABLE_RECONNECT_ON_FIRST_CONNECT);
		}
		else if (tmp == "connect")
		{
			//client->ConnectAsync("10.0.30.66", 1234, 0, NETLIB_CLIENT_FLAG_PREFER_TCP);
			//client->ConnectAsync("127.0.0.1", 1234, 0, NETLIB_CLIENT_FLAG_PREFER_TCP);
			client->ConnectAsync(destip, tcpport, udpport); 
			//默认使用UDP，加了NETLIB_CLIENT_FLAG_PREFER_TCP参数则首先尝试TCP
			//NETLIB_FLAG_TCP_NODELAY: (仅对TCP有效)不在系统缓冲区停留，立即发包

			//client->ConnectAsync("127.0.0.1", 1248, 2012);
		}
		else if (tmp == "connect2")
		{
			client->ConnectAsync(destip, tcpport, 22012);
		}
		else if (tmp == "connectbadtcp")
		{
			//client->ConnectAsync("127.0.0.1", 1248, 2012, NETLIB_CLIENT_FLAG_PREFER_TCP | NETLIB_FLAG_TCP_NODELAY); 
			client->ConnectAsyncTCP(destip, 11248);
		}
		else if (tmp == "connectbadtcp2")
		{
			client->ConnectAsyncTCP("123.11.22.33", 11248);
		}
		else if (tmp == "connectbadudpclose")
		{
			client->ConnectAsyncUDP("123.11.22.33", 11245);
			client->Disconnect();
		}
		else if (tmp == "connectbadtcpconnect")
		{
			client->ConnectAsyncTCP(destip, 11248);
			client->ConnectAsyncTCP(destip, tcpport);
		}
		else if (tmp == "connecttcp")
		{
			client->ConnectAsyncTCP(destip, tcpport);
		}
		else if (tmp == "connecttcpdr")
		{
			client->DisableReconnect();
			client->ConnectAsyncTCP(destip, tcpport);
		}
		else if (tmp == "connectudpdr")
		{
			client->DisableReconnect();
			client->ConnectAsyncUDP(destip, udpport);
		}
		else if (tmp == "connectbadudp")
		{
			client->ConnectAsyncUDP(destip, 22012);
		}
		else if (tmp == "connectudp")
		{
			client->ConnectAsyncUDP(destip, udpport);
		}
		else if (tmp == "release")
		{
			client->Disconnect(); //用完client得记得调用Disconnect方法，他才会正确释放。
			client.reset();		//手动释放
			printf("release success\n");
		}
		else if (tmp == "send")
		{
			send();
		}
		else if (tmp == "continuesend")
		{
			int packet_count, interval;
			scanf("%d%d", &packet_count, &interval);
			continuesend(packet_count, interval);
		}
		else if (tmp == "sendsmall")
		{
			int i;

			char* data = new char[_SMALL_TEST_SIZE];
			for (i = 8; i < _SMALL_TEST_SIZE; ++i)
			{
				data[i] = i % 77; //用于测试正确性
			}
			*(uint32_t*)data = _SMALL_TEST_SIZE; //最头上4字节是整个包的长度(包含这4个字节)
			*(uint32_t*)(data + 4) = 77;

			client->SendAsync(data, (void*)88);
		}
		else if (tmp == "sendtest")
		{
			int i;

			char* data = new char[_TEST_SIZE];
			for (i = 8; i < _TEST_SIZE; ++i)
			{
				data[i] = i % 99; //用于测试正确性
			}
			*(uint32_t*)data = _TEST_SIZE; //最头上4字节是整个包的长度(包含这4个字节)
			*(uint32_t*)(data + 4) = 240; //240是特殊指令

			client->SendAsync(data, (void*)123);
		}
		else if (tmp == "sendsendclose")
		{
			int i;
			char* data = new char[_TEST_SIZE];
			for (i = 8; i < _TEST_SIZE; ++i)
			{
				data[i] = i % 77; //用于测试正确性
			}
			*(uint32_t*)data = _TEST_SIZE; //最头上4字节是整个包的长度(包含这4个字节)
			*(uint32_t*)(data + 4) = 77;

			client->SendCopyAsync(data);
			client->SendCopyAsync(data); 
			client->Disconnect();
		}
		else if (tmp == "sendclose")
		{
			int i;
			char* data = new char[_TEST_SIZE2];
			for (i = 8; i < _TEST_SIZE2; ++i)
			{
				data[i] = i % 77; //用于测试正确性
			}
			*(uint32_t*)data = _TEST_SIZE2; //最头上4字节是整个包的长度(包含这4个字节)
			*(uint32_t*)(data + 4) = 77;

			LOGEVENTLSTR("Debug", "sendclose data: 0x" << std::hex << (std::size_t)data);
			client->SendAsync(data, (void*)66);
			client->Disconnect();
		}
		else if (tmp == "sendrelease")
		{
			int i;
			char* data = new char[_TEST_SIZE];
			for (i = 8; i < _TEST_SIZE; ++i)
			{
				data[i] = i % 77; //用于测试正确性
			}
			*(uint32_t*)data = _TEST_SIZE; //最头上4字节是整个包的长度(包含这4个字节)
			*(uint32_t*)(data + 4) = 77;

			client->SendAsync(data);
			client->Disconnect();
			client.reset();
			printf("release success\n");
		}
		else if (tmp == "sendcopy")
		{			
			int i;
			char data[_TEST_SIZE];
			for (i = 8; i < _TEST_SIZE; ++i)
			{
				data[i] = i % 77; //用于测试正确性
			}
			*(uint32_t*)data = _TEST_SIZE; //最头上4字节是整个包的长度(包含这4个字节)
			*(uint32_t*)(data + 4) = 77;

			client->SendCopyAsync(data, (void*)77);
		}
		else if (tmp == "sendcopyclose")
		{			
			int i;
			char data[_TEST_SIZE];
			for (i = 8; i < _TEST_SIZE; ++i)
			{
				data[i] = i % 76; //用于测试正确性
			}
			*(uint32_t*)data = _TEST_SIZE; //最头上4字节是整个包的长度(包含这4个字节)
			*(uint32_t*)(data + 4) = 76;

			client->SendCopyAsync(data);
			client->Disconnect();
		}
		else if (tmp == "close")
		{
			client->Disconnect(); //Disconnect之后会触发DisconnectedHandler, error为0
		}
		else if (tmp == "resetfaildata")
		{
			client->ResetFailedData();
		}
		else if (tmp == "anotherconnect")
		{
			int port = Input("Port:", 0); 
			NetLib_Client_ptr client2 = NetLib_NewClient(std::shared_ptr<MyClientDelegate>(new MyClientDelegate));
			client2->ConnectAsyncTCP(destip, port);
		}
		else
		{
			printf("client_sample: unrecognized command: %s\n", tmp.c_str());
		}
	}
	
	google_lalune::protobuf::ShutdownProtobufLibrary(); 

#ifndef _STATIC_NETLIB_
	NetLib_Shutdown();
#endif

	return 0;
}


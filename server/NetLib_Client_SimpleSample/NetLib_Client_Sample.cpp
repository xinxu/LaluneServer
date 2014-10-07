#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "NetLib/NetLib.h"
#include <memory.h>
#include <google/protobuf/stubs/common.h>
#include <boost/thread.hpp>
#include "interactive_input.h"
#include "utility1.h"
#include "Log/Log.h"

#define _TEST_SIZE (20)

NetLib_Client_ptr client;

class MyClientDelegate : public NetLib_Client_Delegate
{
public:
	void ConnectedHandler(NetLib_Client_ptr clientptr)
	{
		LOGEVENTL("Info", "connected");	
	}

	void RecvFinishHandler(NetLib_Client_ptr clientptr, char* data)
	{
		//data在RecvFinishHandler返回后会释放

		/*uint32_t size = *(uint32_t*)data;
		
		if (size >= 8)
		{						
			uint32_t cmd = *(uint32_t*)(data + 4);

			bool correct = true;
			unsigned int i;
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
		else
		{
			printf("size less than 8.\n");
		}*/
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
			//will_reconnect = false;
			if (will_reconnect)
			{
				printf("reconnecting..\n");
			}
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

char destip[100] = "127.0.0.1";
int tcpport = 2345;

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

    LogInitializeLocalOptions(true, true, "client_simple_sample");
	
	//client = NetLib_NewClient(std::shared_ptr<MyClientDelegate>(new MyClientDelegate));
	client = NetLib_NewClient<MyClientDelegate>();
	
	for (;;)
	{
		std::string tmp;
		if (! std::getline(std::cin, tmp))
		{			
			boost::this_thread::sleep(boost::posix_time::hours(1));
			continue;
		}

		if (tmp == "new")
		{
			client = NetLib_NewClient(std::shared_ptr<MyClientDelegate>(new MyClientDelegate));
		}
		else if (tmp == "dest")
		{
			strcpy(destip, InputStr("DestinationIP", "127.0.0.1").c_str());
		}
		else if (tmp == "tcpport")
		{
			tcpport = Input("TCPPort", 1248);
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
		else if (tmp == "connect")
		{
			client->ConnectAsync(destip, tcpport);
		}
		else if (tmp == "release")
		{
			client->Disconnect(); //用完client得记得调用Disconnect方法，他才会正确释放。
			client.reset();		//使得外部没有引用计数
			printf("release success\n");
		}
		else if (tmp == "send")
		{
			int i;

			char* data = new char[_TEST_SIZE];
			//for (i = 8; i < _TEST_SIZE; ++i)
			//{
			//	data[i] = i % 77; //用于测试正确性
			//}
			//*(uint32_t*)data = _TEST_SIZE; //最头上4字节是整个包的长度(包含这4个字节)
			//*(uint32_t*)(data + 4) = 77; 

			strcpy(data + 4, "hello");
			*(uint32_t*)data = _TEST_SIZE;

			client->SendAsync(data);
		}
		else if (tmp == "close")
		{
			client->Disconnect(); //Disconnect之后会触发DisconnectedHandler, error为0
		}
		else
		{
			printf("client_sample: unrecognized command: %s\n", tmp.c_str());
		}
	}
	
	google_lalune::protobuf::ShutdownProtobufLibrary(); 

	return 0;
}


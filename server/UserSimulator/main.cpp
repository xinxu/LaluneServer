#include "UserSimulator.h"
#include <google/protobuf/stubs/common.h>
#include <boost/thread.hpp>
#include "Log/Log.h"
#include "include/ioservice_thread.h"
#include <iostream>
#include "include/ptime2.h"
#include "include/utility1.h"
extern ioservice_thread thread;

extern int ping[PING_RANGE_COUNT + 1];
extern std::string section[PING_RANGE_COUNT + 1];
extern uint64_t time_begin, time_now;


std::shared_ptr<UserSimulator> us;

#define GATEWAY_SERVER_DEFAULT_IP ("127.0.0.1")
#define GATEWAY_SERVER_DEFAULT_PORT (6677)


#define COMBATE_SERVER_DEFAULT_IP ("192.168.1.42")//("180.150.178.148")
#define COMBATE_SERVER_DEFAULT_PORT (5000)
void initialize()
{
	us = std::make_shared<UserSimulator>();
	us->Connect(GATEWAY_SERVER_DEFAULT_IP, GATEWAY_SERVER_DEFAULT_PORT);
}
//void initialize()
//{
//	us = std::make_shared<UserSimulator>();
//	us->Connect(GATEWAY_SERVER_DEFAULT_IP, GATEWAY_SERVER_DEFAULT_PORT);
//	
//}
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

	LogInitializeLocalOptions(true, true, "user_simulator");

	thread.start();

	thread.get_ioservice().post(boost::bind(&initialize));
	/*thread.get_ioservice().post(boost::bind(&initcombate));*/
	
	//TODO： 要支持跑简易脚本
	for (;;)
	{
		std::string tmp;
		if (!std::getline(std::cin, tmp))
		{
			boost::this_thread::sleep(boost::posix_time::hours(1));
			continue;
		}
		else if (tmp == "register")
		{
			//thread.get_ioservice().post(boost::bind(&UserSimulator::Register, us));
		}
		else if (tmp.compare(0, 7, "version") == 0)
		{
			//thread.get_ioservice().post(boost::bind(&UserSimulator::Version, us, tmp.substr(8)));
		}
		else if (tmp == "match")
		{
			thread.get_ioservice().post(boost::bind(&UserSimulator::Match, us));
		}
//		else if (tmp == "combat")
//		{
//			int id_num;
//			std::string str;
//			std::cin >> id_num;
//			getchar();
//			thread.get_ioservice().post(boost::bind(&initcombate,id_num));
//#ifdef WIN32
//			Sleep(1000);
//#else
//			sleep(1);
//#endif
//			memset(ping, 0, sizeof(ping));
//			for (int i = 0; i < PING_RANGE_COUNT; ++i)
//			{
//				section[i] = "[" + utility1::int2str(i * 10) + "ms-" + utility1::int2str((i + 1) * 10) + "ms]";
//			}
//			section[PING_RANGE_COUNT] = "[" + utility1::int2str(PING_RANGE_COUNT * 10) + "ms+]";
//			time_begin = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();
//			for (int i = 0; i < id_num;i++)
//				thread.get_ioservice().post(boost::bind(&UserSimulator::Combat, us[i],i));
//			while (1)
//			{
//				time_now = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();
//
//				if ((time_now - time_begin) / 1000000 >= 30)
//				{
//					int i;
//					time_begin = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();
//					for (i = 0; i <= PING_RANGE_COUNT; i++)
//					{
//						//std::cout << section[i] << ping[i] << std::endl;
//						LOGEVENTL("PING", _ln(section[i]) << ping[i]);
//					}
//				}
//			}
//
//		}
		else
		{
			printf("client_sample: unrecognized command: %s\n", tmp.c_str());
		}
	}

	google_lalune::protobuf::ShutdownProtobufLibrary();

	return 0;
}


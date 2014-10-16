#include "UserSimulator.h"
#include <google/protobuf/stubs/common.h>
#include <boost/thread.hpp>
#include "Log/Log.h"
#include "ioservice_thread.h"
#include <iostream>
extern ioservice_thread thread;

std::shared_ptr<UserSimulator> us[100];

#define GATEWAY_SERVER_DEFAULT_IP ("192.168.1.43")
#define GATEWAY_SERVER_DEFAULT_PORT (6677)


#define COMBATE_SERVER_DEFAULT_IP ("192.168.1.16")//("180.150.178.148")
#define COMBATE_SERVER_DEFAULT_PORT (5000)
/*void initialize()
{
	us = std::make_shared<UserSimulator>();
	us->Connect(GATEWAY_SERVER_DEFAULT_IP, GATEWAY_SERVER_DEFAULT_PORT);
}*/
void initcombate()
{
	int i;
	for (i = 0; i < 6; i++)
	{

		us[i] = std::make_shared<UserSimulator>();
		us[i]->Connect(COMBATE_SERVER_DEFAULT_IP, COMBATE_SERVER_DEFAULT_PORT);
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

	NETLIB_CHECK_VERSION;

	LogInitializeLocalOptions(true, true, "user_simulator");

	thread.start();

	//thread.get_ioservice().post(boost::bind(&initialize));
	thread.get_ioservice().post(boost::bind(&initcombate));
	
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
		else if (tmp == "combat")
		{
			int id_num;
			std::string str;
			std::cin >> id_num;
			getchar();
			for (int i = 0; i < id_num;i++)
				thread.get_ioservice().post(boost::bind(&UserSimulator::Combat, us[i],i));
		}
		else
		{
			printf("client_sample: unrecognized command: %s\n", tmp.c_str());
		}
	}

	google_lalune::protobuf::ShutdownProtobufLibrary();

	return 0;
}


#include "UserSimulator.h"
#include <google/protobuf/stubs/common.h>
#include <boost/thread.hpp>
#include "Log/Log.h"
#include "ioservice_thread.h"

extern ioservice_thread thread;

std::shared_ptr<UserSimulator> us;

#define GATEWAY_SERVER_DEFAULT_IP ("127.0.0.1")
#define GATEWAY_SERVER_DEFAULT_PORT (6677)

void initialize()
{
	us = std::make_shared<UserSimulator>();
	us->Connect(GATEWAY_SERVER_DEFAULT_IP, GATEWAY_SERVER_DEFAULT_PORT);
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

	thread.get_ioservice().post(boost::bind(&initialize));
	
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
			thread.get_ioservice().post(boost::bind(&UserSimulator::Register, us));
		}
		else if (tmp.compare(0, 7, "version") == 0)
		{
			thread.get_ioservice().post(boost::bind(&UserSimulator::Version, us, tmp.substr(8)));
		}
		else
		{
			printf("client_sample: unrecognized command: %s\n", tmp.c_str());
		}
	}

	google_lalune::protobuf::ShutdownProtobufLibrary();

	return 0;
}


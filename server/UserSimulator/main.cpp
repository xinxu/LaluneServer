#include "UserSimulator.h"
#include <google/protobuf/stubs/common.h>
#include <boost/thread.hpp>
#include "Log/Log.h"
#include "ioservice_thread.h"

extern ioservice_thread thread;

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
	
	auto us = std::make_shared<UserSimulator>();
	us->Connect("192.168.1.16", 6677);
	us->Register(); //TODO 这个也要改，到时候都得在thread里面跑

	thread.start();

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
			us->Register(); //TODO 这个也要改，到时候都得在thread里面跑
		}
		else
		{
			printf("client_sample: unrecognized command: %s\n", tmp.c_str());
		}
	}

	google_lalune::protobuf::ShutdownProtobufLibrary();

	return 0;
}


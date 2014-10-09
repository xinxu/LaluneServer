#include <boost/asio.hpp>
#include "include/ioservice_thread.h"
#include "Log/Log.h"
#include "NetLib/NetLib.h"
#include "memory.h"
#include "string.h"
#include <google/protobuf/stubs/common.h>
#include "../../LaluneCommon/include/MessageTypeDef.h"
#include "ServerCommonLib/ServerCommon.h"
#include "VersionServerSessionDelegate.h"
#include "Version.pb.h"
#include "VersionIformation.h"
#define VERSION_SERVER_PORT (5577)

ioservice_thread thread;

NetLib_Server_ptr server4user;

class VersionServerCommonLibDelegate : public CommonLibDelegate
{
public:
	void onInitialized()
	{
	}

	void onConfigRefresh(const std::string& content)
	{
		ptree pt;
		try{
			read_json(content, pt);
		}
		catch (ptree_error & e) {
		}
		ptree pt_root;
		pt_root = pt.get_child("root");

		VersionIformation version_temp;
		vector<VersionIformation> version_infor;
		BOOST_FOREACH(boost::property_tree::ptree::value_type &v, pt_root)
		{
			//std::stringstream s;
			//	write_json(s, v.second);
			//std::string image_item = s.str();
			//	cout << "xx" << endl;
			//cout << image_item << endl;

			version_temp.now_version = v.second.get<string>("now_ve");
			version_temp.next_version = v.second.get<string>("next_ve");
			version_temp.file_path = VersionIformation::GetVerctor( v.second.get<string>("path"));
			version_temp.file_url = VersionIformation::GetVerctor(v.second.get<string>("url"));
			version_infor.push_back(version_temp);
		}

	}
	
	void onReceiveCmd(int cmd_type, const std::string& data)
	{
		lalune::AddVersion mess;
		if (mess.ParseFromString(data))
		{

		}
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

	NetLib_Server_ptr server = NetLib_NewServer<VersionServerSessionDelegate>(&thread);

	//可以不指定端口 TODO (主要是内部端口)
	if (!server->StartTCP(VERSION_SERVER_PORT, 1, 120)) //端口，线程数，超时时间
	{
		LOGEVENTL("Error", "Server Start Failed !");

		NetLib_Servers_WaitForStop();

		LogUnInitialize();

		google_lalune::protobuf::ShutdownProtobufLibrary();

		return 0;
	}
	
	LOGEVENTL("Info", "Server Start Success");

	VersionServerCommonLibDelegate* cl_delegate = new VersionServerCommonLibDelegate();
	InitializeCommonLib(thread, cl_delegate, VERSION_SERVER_PORT, SERVER_TYPE_VERSION_SERVER, argc, argv);
	
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


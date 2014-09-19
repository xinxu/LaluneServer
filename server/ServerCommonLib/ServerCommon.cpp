#include "NetLib.h"
#include "include/ToAbsolutePath.h"
#include "ServerCommon.h"
#include "include/SimpleIni.h"
#include "include/utility1.h"
#include "include/ioservice_thread.h"

CommonLibDelegate* __commonlib_delegate;

NetLib_Client_ptr __conn2controlserver;

class Conn2ControlServerDelegate : public NetLib_Client_Delegate
{
public:
	void RecvFinishHandler(NetLib_Client_ptr clientptr, char* data);
};

void SayHello2ControlServer()
{

}

void InitializeCommonLib(ioservice_thread& thread, CommonLibDelegate* d, int argc, char* argv[])
{
	__commonlib_delegate = d;

	std::string control_server_ip = "192.168.1.16"; //默认值
	int control_server_port = 5432;

	if (argc >= 3)
	{
		//从启动参数里读

		control_server_ip = argv[1];
		control_server_port = utility1::str2int(argv[2]);
	}
	else
	{
		//从ini里面读
		CSimpleIni ini;
		if (ini.LoadFile(utility3::ToAbsolutePath("local_config.ini").c_str()) == SI_OK)
		{
			control_server_ip = ini.GetValue("ControlServer", "ControlServerIP", "192.168.1.16");
			control_server_port = ini.GetLongValue("ControlServer", "ControlServerPortP", 5432);
		}
	}

	__conn2controlserver = NetLib_NewClient<Conn2ControlServerDelegate>(&thread);
	__conn2controlserver->ConnectAsyncTCP(control_server_ip.c_str(), control_server_port);

	SayHello2ControlServer();
}

void ReportLoad(float load_factor)
{

}

void UpdateCorrespondingServer(uint64_t user_id, const CorrespondingServer& header_ex)
{

}

void Conn2ControlServerDelegate::RecvFinishHandler(NetLib_Client_ptr clientptr, char* data)
{

}
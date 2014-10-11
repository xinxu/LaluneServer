#include "MessageTypeDef.h"
#include "ServerCommon.h"
#include "commonlib/CommonLib.pb.h"
#include "controlserver/ControlServer.pb.h"
#include "ioservice_thread.h"
#include "UpdateVersion.h"

ioservice_thread _thread;

class BackgroundCommonLibDelegate : public CommonLibDelegate
{
public:
	void onReceiveOtherDataFromControlServer(int msg_type, const char* data, int data_len)
	{
		if (msg_type == MSG_TYPE_CONTROL_SERVER_FETCH_CONFIG_RESPONSE)
		{
			common::ConfigFile cf;
			if (cf.ParseFromArray(data, data_len))
			{
				if (cf.file_name() == "version_control.txt")
				{
					//TODO. blablabla

					RefreshConfig(SERVER_TYPE_VERSION_SERVER, "version_control.txt", cf.content() + "\nhaha");
				}
			}
		}
	}
};
int main(int argc, char* argv[])
{
	_thread.start();
	InitializeCommonLib(_thread, new BackgroundCommonLibDelegate(), SERVER_TYPE_BACKGROUND, argc, argv);
	
	RefreshConfig(SERVER_TYPE_VERSION_SERVER, "version_control.txt", "lalala");

	control_server::FetchConfigRequest fc;
	fc.set_server_type(SERVER_TYPE_VERSION_SERVER);
	fc.set_file_name("version_control.txt");
	SendMsg2ControlServer(MSG_TYPE_CONTROL_SERVER_FETCH_CONFIG_REQUEST, fc);

	UpdateVersion Up;
	//Up = new UpdateVersion();
	Up.Input();
	if (Up.op_command == "add")
	{
		Up.Uploading();
		Up.SendIformation();
	}
	else if (Up.op_command == "del")
	{
		Up.DelFile();
		Up.SendIformation();
	}
	//delete Up;
	return 0;
}
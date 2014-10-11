﻿#include "MessageTypeDef.h"
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
					UpdateVersion Up;
					//Up = new UpdateVersion();
					Up.Input();
					if (Up.op_command == "add")
					{
						Up.Uploading();
						Up.SendIformation(cf.content());
					}
					else if (Up.op_command == "del")
					{
						Up.DelFile();
						Up.SendIformation(cf.content());
					}
				}
			}
		}
	}
};
int main(int argc, char* argv[])
{
	_thread.start();
	InitializeCommonLib(_thread, new BackgroundCommonLibDelegate(), SERVER_TYPE_BACKGROUND, argc, argv);//初始化
	
	//RefreshConfig(SERVER_TYPE_VERSION_SERVER, "version_control.txt", "lalala");  发给control

	//请求服务
	control_server::FetchConfigRequest fc;
	fc.set_server_type(SERVER_TYPE_VERSION_SERVER);
	fc.set_file_name("version_control.txt");
	SendMsg2ControlServer(MSG_TYPE_CONTROL_SERVER_FETCH_CONFIG_REQUEST, fc);

	//Up = new UpdateVersion();
	
	//delete Up;
	Sleep(-1);
	return 0;
}
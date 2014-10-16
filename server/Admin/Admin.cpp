#include "MessageTypeDef.h"
#include "ServerCommon.h"
#include "commonlib/CommonLib.pb.h"
#include "controlserver/ControlServer.pb.h"
#include "ioservice_thread.h"
#include "Log/Log.h"
#include <boost/asio.hpp>
#include "utility1.h"

ioservice_thread _thread;

class AdminCommonLibDelegate : public CommonLibDelegate
{
public:
	void onReceiveOtherDataFromControlServer(int msg_type, const char* data, int data_len)
	{
		if (msg_type == MSG_TYPE_CONTROL_SERVER_CMD_RESULT)
		{
			control_server::CommandResult cr;
			if (cr.ParseFromArray(data, data_len))
			{
				LOGEVENTL("RESULT", cr.result());
			}
		}
	}
};

void SendCmd(std::string cmd_line)
{
	std::vector<std::string> args;
	utility1::split(cmd_line, args, ' ');

	control_server::Command cmd;
	cmd.set_command_name(args[0]);
	for (unsigned int i = 1; i < args.size(); ++i)
	{
		cmd.add_args(args[i]);
	}
	SendMsg2ControlServer(MSG_TYPE_CONTROL_SERVER_CMD, cmd);
}

int main(int argc, char* argv[])
{
	_thread.start();
	InitializeCommonLib(_thread, new AdminCommonLibDelegate(), SERVER_TYPE_BACKGROUND, argc, argv);//初始化

	for (;;)
	{
		std::string cmd_line;
		if (!std::getline(std::cin, cmd_line))
		{
			boost::this_thread::sleep(boost::posix_time::hours(1));
			continue;
		}

		_thread.get_ioservice().post(boost::bind(&SendCmd, cmd_line));
	}

	google_lalune::protobuf::ShutdownProtobufLibrary();

	return 0;
}
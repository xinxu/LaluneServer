#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "NetLib/NetLib.h"
#include <memory.h>
#include <google/protobuf/stubs/common.h>
#include <boost/thread.hpp>
#include "interactive_input.h"
#include "utility1.h"
#include "Log/Log.h"
#include "UserSimulator.h"
#include "Account.pb.h"
#include "MessageTypeDef.h"
#include "ioservice_thread.h"
#include "Version.pb.h"

ioservice_thread thread;

void UserSimulator::Connect(const std::string& ip, int port)
{
	_client = NetLib_NewClient(shared_from_this(), &thread);
	_client->ConnectAsync(ip.c_str(), port, NETLIB_CLIENT_ENABLE_RECONNECT_ON_FIRST_CONNECT | NETLIB_CLIENT_FLAG_KEEP_ALIVE);
	_client->SetKeepAliveIntervalSeconds(30);
}

void UserSimulator::Register()
{
	lalune::AutoRegisterRequest auto_register;
	auto_register.set_nick(utility1::generateRandomString(6));
	SendMsg(MSG_TYPE_AUTOREGISTER_REQUEST, auto_register);
}
void UserSimulator::Version()
{
	lalune::CheckVersion now_version;
	//now_version.set_nick(utility1::generateRandomString(6));
	SendMsg(MSG_CHECK_VERSION, now_version);
}

void UserSimulator::ConnectedHandler(NetLib_Client_ptr clientptr)
{

}

void UserSimulator::RecvFinishHandler(NetLib_Client_ptr clientptr, char* data)
{
	if (MSG_LENGTH(data) >= MSG_HEADER_BASE_SIZE)
	{
		switch (MSG_TYPE(data))
		{
		case MSG_TYPE_AUTOREGISTER_RESPONSE:
		{
			lalune::AutoRegisterResponce response;
			if (ParseMsg(data, response))
			{
				LOGEVENTL("INFO", "AutoRegisterResponse. " << _ln("code") << response.code()
					<< _ln("uid") << response.uid() << _ln("pwd") << response.pwd()
					<< _ln("errStr") << response.errstr());
			}
		}
			break;
		default:
			break;
		}
	}
}
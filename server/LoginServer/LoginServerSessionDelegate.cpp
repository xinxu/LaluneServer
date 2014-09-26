#include "LoginServerSessionDelegate.h"
#include "ServerCommon.h"
#include "MessageTypeDef.h"
#include "Account.pb.h"
#include "Log/Log.h"
#include "include/utility1.h"

int debug_user_id_4_register = 10000;

void LoginServerSessionDelegate::RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
{
	if (SERVER_MSG_LENGTH(data) >= SERVER_MSG_HEADER_BASE_SIZE)
	{
		switch (SERVER_MSG_TYPE(data))
		{
		case MSG_TYPE_AUTOREGISTER_REQUEST:
		{
			lalune::AutoRegisterRequest auto_register;
			if (ParseMsg(data, auto_register))
			{
				lalune::AutoRegisterResponce response;
				response.set_uid(debug_user_id_4_register++);
				response.set_pwd(utility1::generateRandomString());
				ReplyMsg(sessionptr, MSG_TYPE_AUTOREGISTER_RESPONSE, response);
			}
			break;
		}
		default:
			LOGEVENTL("WARN", "unrecognized message type. " << SERVER_MSG_TYPE(data));
			break;
		}
	}
	else
	{
		LOGEVENTL("WARN", "message length not enough: " << SERVER_MSG_LENGTH(data));
	}
}
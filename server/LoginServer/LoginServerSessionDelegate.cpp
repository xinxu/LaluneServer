#include "LoginServerSessionDelegate.h"
#include "ServerCommon.h"
#include "MessageTypeDef.h"
#include "Account.pb.h"
#include "Log/Log.h"

int debug_user_id_4_register = 10000;

void LoginServerSessionDelegate::RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
{
	if (SERVER_MSG_LENGTH(data) >= SERVER_MSG_HEADER_BASE_SIZE)
	{
		switch (SERVER_MSG_TYPE(data))
		{
		case MSG_TYPE_AUTOREGISTER:
		{
			lalune::AutoRegisterRequest auto_register;
			if (ParseMsg(data, auto_register))
			{
				lalune::AutoRegisterResponce response;
				response.set_uid(debug_user_id_4_register++);
				ReplyMsg(sessionptr, MSG_TYPE_AUTOREGISTER_RESULT, response);
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
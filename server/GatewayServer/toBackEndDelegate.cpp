#include "toBackEndDelegate.h"
#include "ServerCommon.h"
#include "Log/Log.h"
#include "GatewayServer.h"
#include "MessageTypeDef.h"

void toBackEndDelegate::RecvFinishHandler(std::shared_ptr<NetLibPlus_Client> clientptr, char* data)
{
	if (SERVER_MSG_LENGTH(data) >= SERVER_MSG_HEADER_BASE_SIZE)
	{
		common::HeaderEx ex;
		if (ParseHeaderEx(data, ex))
		{
			int _id;
			if (ex.has_uid())
			{
				_id = ex.uid();
			}
			else if (ex.has_operation_id()) //如果没有uid有operation_id，那么通常是登陆前的消息
			{
				_id = -(int)ex.operation_id(); //operation_id（即tmp_id）在gateway的map里是负数表示的，以和user_id进行区分。。
			}
			else
			{
				LOGEVENTL("ERROR", "I don't know who I should send to");
				return;
			}

			NetLib_ServerSession_ptr user_session = GetSessionById(_id);
			if (user_session)
			{
				int len = SERVER_MSG_HEADER_BASE_SIZE + SERVER_MSG_DATA_LEN(data);
				char* send_buf = new char[len];
				MSG_LENGTH(send_buf) = len;		// 数据包的字节数（含msghead）
				MSG_TYPE(send_buf) = SERVER_MSG_TYPE(data);
				MSG_ERROR(send_buf) = 0;
				MSG_ENCRYPTION_TYPE(send_buf) = 0;
				MSG_RESERVED(send_buf) = 0;

				memcpy(MSG_DATA(send_buf), SERVER_MSG_DATA(data), SERVER_MSG_DATA_LEN(data));

				LOGEVENTL("DEBUG", "Reply to user: " << log_::bytes_display(send_buf, len));

				user_session->SendAsync(send_buf);
			}
			else
			{
				LOGEVENTL("ERROR", "user_session not found. " << _ln("id") << _id);
			}
		}
		else
		{
			LOGEVENTL("ERROR", "parse HeaderEx failed");
		}
	}
	else
	{
		LOGEVENTL("ERROR", "msg too short");
	}
}
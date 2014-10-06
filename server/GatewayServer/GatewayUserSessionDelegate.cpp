#include "GatewayUserSessionDelegate.h"
#include "ServerCommon.h"
#include "MessageTypeDef.h"
#include "Log/Log.h"
#include "GatewayServer.h"

void GatewayUserSessionDelegate::ConnectedHandler(NetLib_ServerSession_ptr sessionptr)
{

}

void GatewayUserSessionDelegate::DisconnectedHandler(NetLib_ServerSession_ptr sessionptr)
{
	int _id = sessionptr->GetAttachedData();
	if (_id != 0)
	{
		UserSessionLeft(_id);
		if (_id < 0)
		{
			ReleaseTmpUserId(-_id); //因为tmp_id存到map里是取反用的
		}
	}
}

void GatewayUserSessionDelegate::RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
{
	if (MSG_LENGTH(data) >= MSG_HEADER_BASE_SIZE)
	{
		int server_type = MSG_TYPE(data) / MSG_TYPE_SPAN;
		switch (server_type)
		{
		case SERVER_TYPE_GATEWAY_SERVER:
			//需要自己处理的
			LOGEVENTL("Warn", "ServerType=GatewayServer haven't implemented");
			break;
		case SERVER_TYPE_BASIC_INFO_SERVER:
		case SERVER_TYPE_LEAGUE_SERVER:
		case SERVER_TYPE_ASYNC_BATTLE_SERVER:
			//需要哈希的
			LOGEVENTL("Warn", "ServerType haven't implemented");
			break;
		default:
			//转发的
			{
				//这里还有待优化。不用返回个map出来，在里面就选好就行了。就是可能会有个几乎是Gateway专用的方法实现在ServerCommon里
				std::map<int, NetLibPlus_ServerInfo> info = NetLibPlus_getClientsInfo(server_type);
				if (info.size()) //TODO 这里先临时往第一个发了，之后再改。。
				{
					common::HeaderEx ex;
					char* send_buf;
					if (server_type == SERVER_TYPE_VERSION_SERVER || server_type == SERVER_TYPE_LOGIN_SERVER) //这两个服务是在登陆以前，于是要加上operation_id（否则不知道回给哪个用户）。否则的话是加user_id
					{
						int tmp_id = sessionptr->GetAttachedData();
						if (tmp_id == 0)
						{
							tmp_id = GetTmpUserId();
							sessionptr->SetAttachedData(tmp_id);
							UpdateUserSession(-tmp_id, sessionptr); //tmp_id在map里是用负数表示的，以和真正的user_id做区分
						}
						ex.set_operation_id(tmp_id);
					}
					else
					{
						int uid = sessionptr->GetAttachedData();
						if (uid == 0)
						{
							LOGEVENTL("ERROR", "UNEXPECTED. Attached uid = 0. " << _ln("MsgType") << MSG_TYPE(data));
							return;
						}
						ex.set_uid(uid);
					}
					int ex_len = ex.ByteSize();
					int len = SERVER_MSG_HEADER_BASE_SIZE + ex_len + MSG_DATA_LEN(data);
					send_buf = new char[len];
					SERVER_MSG_LENGTH(send_buf) = len;
					SERVER_MSG_TYPE(send_buf) = MSG_TYPE(data);
					SERVER_MSG_ERROR(send_buf) = 0;
					SERVER_MSG_HEADER_EX_LEN(send_buf) = ex_len;
					SERVER_MSG_RESERVED(send_buf) = 0;

					ex.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)SERVER_MSG_AFTER_HEADER_BASE(send_buf));
					memcpy(SERVER_MSG_DATA(send_buf), MSG_DATA(data), MSG_DATA_LEN(data));

					auto client = NetLibPlus_getClient(info.begin()->first);
					if (client)
					{
						client->SendAsync(send_buf);
					}
					else
					{
						delete[] send_buf;
					}
				} //else的情况里面打过log了，就不打了
			}
			break;
		}
	}
}
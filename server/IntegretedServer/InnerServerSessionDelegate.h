#ifndef __INNER_SERVER_SESSION_DELEGATE_H_
#define __INNER_SERVER_SESSION_DELEGATE_H_

#include "ServerHeaderDef.h"
#include "AutoMatchServer.h"

class InnerServerSessionDelegate : public NetLib_ServerSession_Delegate
{
public:

	BEGIN_HANDLER(InnerServerSession, NetLib_ServerSession_ptr)

	HANDLE_MSG(boids::PVP_SERVER_CREATE_GAME_RESPONSE, boids::CreateGameResponse, ams.CreateGameResponseGot) //战斗服反馈成功或失败给匹配服
	HANDLE_MSG_SESSION(boids::PVP_SERVER_REGISTER_REQUEST, boids::PvPServerRegister, ams.ServerRegister) //向匹配服注册战斗服
	HANDLE_MSG_SESSION(boids::PVP_SERVER_HEART_BEAT, boids::PvPServerHeartBeat, ams.ServerHeartBeat)

	END_HANDLER(InnerServerSession)

	void DisconnectedHandler(NetLib_ServerSession_ptr sessionptr, NetLib_Error error, int inner_error_code)
	{
		LOGEVENTL("INFO", "InnerServerSession disconnected. " << _ln("error") << error << _ln("inner_error") << inner_error_code);
	}
};

#endif
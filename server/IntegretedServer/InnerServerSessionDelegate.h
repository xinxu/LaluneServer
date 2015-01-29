#ifndef __INNER_SERVER_SESSION_DELEGATE_H_
#define __INNER_SERVER_SESSION_DELEGATE_H_

#include "ServerHeaderDef.h"
#include "AutoMatchServer.h"

class InnerServerSessionDelegate : public NetLib_ServerSession_Delegate
{
public:

	BEGIN_HANDLER(NetLib_ServerSession_ptr)

	HANDLE_MSG(boids::PVP_SERVER_CREATE_GAME_REQUEST, boids::CreateGameResponse, ams.CreateGameResponseGot) //战斗服反馈成功或失败给匹配服
	HANDLE_MSG_SESSION(boids::PVP_SERVER_REGISTER_REQUEST, boids::PvPServerRegister, ams.ServerRegister) //向匹配服注册战斗服
	HANDLE_MSG_SESSION(boids::PVP_SERVER_HEART_BEAT, boids::PvPServerHeartBeat, ams.ServerHeartBeat)

	END_HANDLER(InnerServerSessionDelegate)
};

#endif
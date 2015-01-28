#include "InnerServerSessionDelegate.h"
#include "ServerHeaderDef.h"
#include "Log/Log.h"
#include "AutoMatchServer.h"
#include "boids.pb.h"
#include "Battle.pb.h"

void InnerServerSessionDelegate::RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
{
	BEGIN_SWITCH
		
	HANDLE_MSG(boids::MessageType::PVP_SERVER_CREATE_GAME_REQUEST, boids::CreateGameResponse, ams.CreateGameResponseGot) //战斗服反馈成功或失败给匹配服
	HANDLE_MSG(boids::MessageType::PVP_SERVER_REGISTER, boids::PvPServerRegister, ams.ServerRegister) //向匹配服注册战斗服
	HANDLE_MSG(boids::MessageType::PVP_SERVER_HEART_BEAT, boids::PvPServerHeartBeat, ams.ServerBeat)

	END_SWITCH("InnerServerSessionDelegate")
}
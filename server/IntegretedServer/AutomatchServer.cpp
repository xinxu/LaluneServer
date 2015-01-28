#include "AutoMatchServer.h"

//来自用户
void AutoMatchServer::MatchRequest(const boids::MatchRequest& user_req)
{

}

//来自战斗服务
void AutoMatchServer::CreateGameResponseGot(const boids::CreateGameResponse& res)
{

}

void AutoMatchServer::ServerRegister(const boids::PvPServerRegister& reg)
{

}

void AutoMatchServer::ServerHeartBeat(const boids::PvPServerHeartBeat& beat)
{

}

AutoMatchServer ams;
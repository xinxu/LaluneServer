#ifndef __Boids_AutoMatchServer_h_
#define __Boids_AutoMatchServer_h_

#include "NetLib/NetLib.h"
#include "battle.pb.h"
#include "boost/asio/deadline_timer.hpp"
#include "Timer.h"
#include <map>

struct IpPort
{
	std::string Ip;
	int port;

	IpPort()
	{
	}

	IpPort(const std::string& ip_, int port_) : Ip(ip_), port(port_)
	{
	}

	bool operator == (const IpPort& rhs)
	{
		return Ip == rhs.Ip && port == rhs.port;
	}

	bool operator < (const IpPort& rhs)
	{
		return Ip < rhs.Ip || Ip == rhs.Ip && port < rhs.port;
	}
};

struct PvPServerInfo
{
	IpPort id;
	NetLib_ServerSession_ptr session;
	std::shared_ptr<OneOffTimer> timer;
	int region;
	int priority;

	PvPServerInfo()
	{
	}
};

typedef std::vector<PvPServerInfo> ServerList;

typedef std::pair<int, std::string> MatchKey; //region, map_name
typedef std::pair<NetLib_ServerSession_ptr, std::string> WaitingUser; //session, user_id

struct GameInfo
{
	IpPort server;
	boids::GameInitData game_init_data;
	NetLib_ServerSession_ptr sessions[2]; //现在最多2人局，人多再数组开大点就好
};

class AutoMatchServer
{
protected:
	std::map<int, ServerList*> servers_by_region; //用于找服务器
	std::map<MatchKey, WaitingUser> waiting_users; //用于匹配。以后如果有按积分匹配了，这里就要改，现在算是临时方案
	std::map<NetLib_ServerSession_ptr, MatchKey> session2matchkey; //反查matchkey的表，用于取消匹配
	std::map<std::string, GameInfo*> games; //用于创建游戏成功后查游戏相关信息

public:
	~AutoMatchServer();

	//来自用户
	void MatchRequest(NetLib_ServerSession_ptr sessionptr, const boids::MatchRequest& user_req);
	void MatchCancel(NetLib_ServerSession_ptr sessionptr);

	//来自战斗服务
	void CreateGameResponseGot(const boids::CreateGameResponse& res);
	void ServerRegister(NetLib_ServerSession_ptr sessionptr, const boids::PvPServerRegister& reg);
	void ServerHeartBeat(NetLib_ServerSession_ptr sessionptr, const boids::PvPServerHeartBeat& beat);
};

extern AutoMatchServer ams;

#endif
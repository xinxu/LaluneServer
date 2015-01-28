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
	std::shared_ptr<OneOffTimer> timer;
	int region;
	int priority;

	PvPServerInfo()
	{
	}
};

typedef std::vector<PvPServerInfo> ServerList;

class AutoMatchServer
{
protected:
	std::map<int, ServerList*> servers_by_region;

public:
	~AutoMatchServer();

	//来自用户
	void MatchRequest(NetLib_ServerSession_ptr sessionptr, const boids::MatchRequest& user_req);

	//来自战斗服务
	void CreateGameResponseGot(const boids::CreateGameResponse& res);
	void ServerRegister(NetLib_ServerSession_ptr sessionptr, const boids::PvPServerRegister& reg);
	void ServerHeartBeat(const boids::PvPServerHeartBeat& beat);
};

extern AutoMatchServer ams;

#endif
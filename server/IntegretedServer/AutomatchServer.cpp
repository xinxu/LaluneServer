#include "AutoMatchServer.h"
#include "Log/Log.h"
#include "ServerHeaderDef.h"
#include "ioservice_thread.h"
extern ioservice_thread thread;

#define PVPSERVER_TIMEOUT_SECS (90)

AutoMatchServer::~AutoMatchServer()
{
	for (auto& region_and_list : servers_by_region)
	{
		delete region_and_list.second;
	}
}

//来自用户
void AutoMatchServer::MatchRequest(NetLib_ServerSession_ptr sessionptr, const boids::MatchRequest& user_req)
{

}

//来自战斗服务
void AutoMatchServer::CreateGameResponseGot(const boids::CreateGameResponse& res)
{

}

void AutoMatchServer::ServerRegister(NetLib_ServerSession_ptr sessionptr, const boids::PvPServerRegister& reg)
{
	PvPServerInfo info;
	info.id = IpPort(reg.ip(), reg.port());
	info.region = reg.region();
	info.priority = reg.priority();
	info.timer = thread.create_one_off_timer(PVPSERVER_TIMEOUT_SECS, [=]() {
		LOGEVENTL("WARN", "a pvp server timeout. " << _ln("IP") << info.id.Ip << _ln("Port") << info.id.port);
		bool found = false;
		auto it_list = this->servers_by_region.find(info.region);
		if (it_list != this->servers_by_region.end())
		{
			auto& list = *it_list->second;
			for (auto it_server = list.begin(); it_server != list.end(); ++it_server)
			{
				if (it_server->id == info.id)
				{
					list.erase(it_server);
					found = true;
					break;
				}
			}
		}

		if (!found)
		{
			LOGEVENTL("ERROR", "a server timeout but not found in servers_by_region. " << _ln("IP") << info.id.Ip << _ln("Port") << info.id.port);
		}
	});

	auto it_list = servers_by_region.find(info.region);
	if (it_list == servers_by_region.end())
	{
		it_list = servers_by_region.emplace(info.id, new ServerList()).first;
	}
	auto& list = *it_list->second;
	list.push_back(info);
	std::sort(list.begin(), list.end(), [](const PvPServerInfo& server_a, const PvPServerInfo& server_b) {
		return server_a.priority < server_b.priority;
	});

	//TODO: 返回值
}

void AutoMatchServer::ServerHeartBeat(const boids::PvPServerHeartBeat& beat)
{
	IpPort id(beat.ip(), beat.port());
	for (auto& region_and_list : servers_by_region)
	{
		auto& list = *region_and_list.second;
		for (auto it_server = list.begin(); it_server != list.end(); ++it_server)
		{
			if (it_server->id == id)
			{
				it_server->timer->refresh();
				return;
			}
		}
	}
	LOGEVENTL("ERROR", "receive a PvPServerHeartBeat but not found that server in servers_by_region. " << _ln("IP") << id.Ip << _ln("Port") << id.port);
}

AutoMatchServer ams;
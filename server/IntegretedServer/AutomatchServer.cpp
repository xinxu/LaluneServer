#include "AutoMatchServer.h"
#include "Log/Log.h"
#include "ServerHeaderDef.h"
#include "ptime2.h"
#include "utility1.h"
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

void addForce(boids::GameInitData* init_data, const std::string& user_id) //这个方法具体也是临时方案，每个人上场哪些英雄在数据库里有了之后要改
{
	boids::ForceData* force_data = init_data->add_forces();
	force_data->set_user_id(user_id);
	force_data->set_force_id(init_data->forces_size());

	//从现有英雄里随机取HERO_PICK_COUNT(3)个
#define HERO_PICK_COUNT (3)

	std::vector<std::string> all_heros;
	all_heros.push_back("saber");
	all_heros.push_back("gandalf");
	all_heros.push_back("panda");
	all_heros.push_back("vanhelsing");
	all_heros.push_back("dracula");
	all_heros.push_back("zeus");

	for (unsigned i = 0; i < HERO_PICK_COUNT; ++i)
	{
		boids::UnitData* unit = force_data->add_units();
		unit->set_unit_name(all_heros[rand() % (all_heros.size() - HERO_PICK_COUNT) + i]);
		unit->set_unit_level(1);
	}
}

//来自用户
void AutoMatchServer::MatchRequest(NetLib_ServerSession_ptr sessionptr, const boids::MatchRequest& user_req)
{
	MatchKey key(0, user_req.map_name()); //先假设都是region0，以后再改
	auto it = waiting_users.find(key);
	if (it == waiting_users.end())
	{
		waiting_users.emplace(key, WaitingUser(sessionptr, user_req.user_id()));
	}
	else //已经有一个人了，那么就可以凑成一局了
	{
		const WaitingUser& opponent = it->second;
		std::string game_id = time_utility::ptime_to_string4(boost::posix_time::microsec_clock::local_time()); //直接拿精确到微秒的时间当gameid用；便于调试。以后可能要改成uuid，或者增加一段

		//产生一局的GameInitData，发给战斗服务
		boids::CreateGame cg;
		cg.set_game_id(game_id);
		boids::GameInitData* init_data = cg.mutable_game_init_data();
		addForce(init_data, opponent.second);
		addForce(init_data, user_req.user_id());

		//找一台可用的战斗服务发过去
		bool found = false;
		auto it_list = servers_by_region.find(key.first);
		if (it_list != servers_by_region.end())
		{
			auto& list = *it_list->second;
			for (auto it_server = list.begin(); it_server != list.end(); ++it_server)
			{
				if (it_server->session->IsConnected())
				{
					GameInfo game;
					game.server = it_server->id;
					game.sessions[0] = opponent.first;
					game.sessions[1] = sessionptr;
					games.emplace(game_id, game);
					ReplyMsg(it_server->session, boids::PVP_SERVER_CREATE_GAME_REQUEST, cg);
					found = true;

					//TODO: 最好还要一个timer，考虑时间到了没匹到的情况
				}
			}
		}

		if (!found)
		{
			boids::MatchResponse response4user;
			response4user.set_ret_value(boids::MatchResponse_Value_No_Server);
			response4user.set_ret_info("没有找到可用的服务器");

			ReplyMsg(opponent.first, boids::AUTO_MATCH_RESPONSE, response4user);
			ReplyMsg(sessionptr, boids::AUTO_MATCH_RESPONSE, response4user);
		}

		waiting_users.erase(it);
	}
}

//来自战斗服务
void AutoMatchServer::CreateGameResponseGot(const boids::CreateGameResponse& res)
{
	auto it_game = games.find(res.game_id());
	if (it_game != games.end())
	{
		auto& game = it_game->second;
		boids::MatchResponse res4user;
		if (res.ret_value() == 0)
		{
			res4user.set_ret_value(boids::MatchResponse_Value_Success);
			res4user.set_game_uuid(res.game_id());
			res4user.set_game_server_ip(game.server.Ip);
			res4user.set_game_server_port(game.server.port);
		}
		else
		{
			res4user.set_ret_value(boids::MatchResponse_Value_CreateFail);
			res4user.set_ret_info("创建错误。错误代码：" + utility1::int2str(res.ret_value()));
		}
		for (unsigned i = 0; i != 2; ++i)
		{
			ReplyMsg(game.sessions[i], boids::AUTO_MATCH_RESPONSE, res4user);
		}

		games.erase(it_game);
	}
	else
	{
		LOGEVENTL("ERROR", "Received CreateGameResponse but game not found. " << _ln("GameID") << res.game_id());
	}
}

void AutoMatchServer::ServerRegister(NetLib_ServerSession_ptr sessionptr, const boids::PvPServerRegister& reg)
{
	PvPServerInfo info;
	info.id = IpPort(reg.ip(), reg.port());
	info.session = sessionptr;
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
		it_list = servers_by_region.emplace(info.region, new ServerList()).first;
	}
	auto& list = *it_list->second;
	list.push_back(info);
	std::sort(list.begin(), list.end(), [](const PvPServerInfo& server_a, const PvPServerInfo& server_b) {
		return server_a.priority < server_b.priority;
	});

	boids::PvPServerRegisterResponse response;
	ReplyMsg(sessionptr, boids::PVP_SERVER_REGISTER_RESPONSE, response);
}

void AutoMatchServer::ServerHeartBeat(NetLib_ServerSession_ptr sessionptr, const boids::PvPServerHeartBeat& beat)
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
				if (it_server->session != sessionptr)
				{
					it_server->session = sessionptr;
				}
				return;
			}
		}
	}
	LOGEVENTL("ERROR", "receive a PvPServerHeartBeat but not found that server in servers_by_region. maybe already timeout. " << _ln("IP") << id.Ip << _ln("Port") << id.port);
}

AutoMatchServer ams;
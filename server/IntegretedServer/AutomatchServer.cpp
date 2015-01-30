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

	int r[HERO_PICK_COUNT];
	for (unsigned i = 0; i < HERO_PICK_COUNT; ++i)
	{
		r[i] = rand() % (all_heros.size() - HERO_PICK_COUNT + 1);
	}
	std::sort(r, r + HERO_PICK_COUNT);
	for (unsigned i = 0; i < HERO_PICK_COUNT; ++i)
	{
		boids::UnitData* unit = force_data->add_units();
		unit->set_unit_name(all_heros[r[i] + i]);
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
		session2matchkey.emplace(sessionptr, key);
		LOGEVENTL("INFO", user_req.user_id() << " start waiting. map_name: " << user_req.map_name());

		//TODO: 最好还要一个timer，考虑时间到了没匹到的情况
	}
	else //已经有一个人了，那么就可以凑成一局了
	{
		const WaitingUser& opponent = it->second;
		std::string game_id = time_utility::ptime_to_string4(boost::posix_time::microsec_clock::local_time()); //直接拿精确到微秒的时间当gameid用；便于调试。以后可能要改成uuid，或者增加一段

		if (opponent.second == user_req.user_id())
		{
			boids::MatchResponse response4user;
			response4user.set_ret_value(boids::MatchResponse_Value_IllegalRequest);

			LOGEVENTL("INFO", "illegal request. same user_id. " << _ln("user_id") << user_req.user_id() << _ln("game_id") << game_id);

			ReplyMsg(opponent.first, boids::AUTO_MATCH_RESPONSE, response4user);
			ReplyMsg(sessionptr, boids::AUTO_MATCH_RESPONSE, response4user);

			return;
		}

		//产生一局的GameInitData，发给战斗服务
		boids::CreateGame cg;
		cg.set_game_id(game_id);
		boids::GameInitData* init_data = cg.mutable_game_init_data();
		addForce(init_data, opponent.second);
		addForce(init_data, user_req.user_id());

		LOGEVENTL("INFO", "match almost success. " << _ln("game_id") << game_id << _ln("player1") << opponent.second << _ln("player2") << user_req.user_id());

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

					LOGEVENTL("INFO", "find a server. " << _ln("game_id") << game_id << _ln("server_ip") << it_server->id.Ip);
				}
			}
		}

		if (!found)
		{
			boids::MatchResponse response4user;
			response4user.set_ret_value(boids::MatchResponse_Value_No_Server);
			response4user.set_ret_info("没有找到可用的服务器");

			LOGEVENTL("INFO", "no server available. " << _ln("game_id") << game_id);

			ReplyMsg(opponent.first, boids::AUTO_MATCH_RESPONSE, response4user);
			ReplyMsg(sessionptr, boids::AUTO_MATCH_RESPONSE, response4user);
		}

		session2matchkey.erase(opponent.first);
		waiting_users.erase(it);
	}
}

void AutoMatchServer::MatchCancel(NetLib_ServerSession_ptr sessionptr)
{
	//只处理了匹一半掉线的，没有处理已经在询问战斗服务才掉线的情况(太少见了)
	auto it_matchkey = session2matchkey.find(sessionptr);
	if (it_matchkey != session2matchkey.end())
	{
		LOGEVENTL("INFO", "some user disconnected, so cancel match.");
		waiting_users.erase(it_matchkey->second);		
	}
	else
	{
		//找不到很正常，因为用户还没有匹配就断线了
		//LOGEVENTL("ERROR", "some user disconnected, but not found corresponding session in AutoMatchServer");
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

			LOGEVENTL("INFO", "match success. " << _ln("game_id") << res.game_id() << _ln("server_ip") << game.server.Ip);
		}
		else
		{
			res4user.set_ret_value(boids::MatchResponse_Value_CreateFail);
			res4user.set_ret_info("创建错误。错误代码：" + utility1::int2str(res.ret_value()));

			LOGEVENTL("INFO", "create game failed. " << _ln("game_id") << res.game_id() << _ln("ret_value") << res.ret_value());
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

	//检查有没有重复的，有重复的话更新
	bool duplicate = false;
	for (auto& server_info : list)
	{
		if (server_info.id == info.id)
		{
			server_info = info;
			duplicate = true;
			break;
		}
	}
	if (!duplicate)
	{
		list.push_back(info);
	}
	std::sort(list.begin(), list.end(), [](const PvPServerInfo& server_a, const PvPServerInfo& server_b) {
		return server_a.priority < server_b.priority;
	});

	LOGEVENTL("INFO", "a pvp_server registered. " << _ln("IP") << info.id.Ip << _ln("Port") << info.id.port);

	boids::PvPServerRegisterResponse response;
	response.set_ret_value(0); //表示成功
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
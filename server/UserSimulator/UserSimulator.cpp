#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "NetLib/NetLib.h"
#include <memory.h>
#include <google/protobuf/stubs/common.h>
#include <boost/thread.hpp>
#include "include/interactive_input.h"
#include "include/utility1.h"
#include "Log/Log.h"
#include "UserSimulator.h"
#include "Account.pb.h"
#include "ioservice_thread.h"
#include "Version.pb.h"
#include <iostream>
#include "Battle.pb.h"
#include "include/ptime2.h"
#include <vector>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
using namespace boost::uuids;
ioservice_thread thread;
 int ping[PING_RANGE_COUNT + 1];
 std::string section[PING_RANGE_COUNT + 1];
 uint64_t time_begin, time_now;
 /*UserSimulator::UserSimulator():timer1(thread.get_ioservice())
{
	 
}*/

void UserSimulator::Connect(const std::string& ip, int port)
{
	_client = NetLib_NewClient(shared_from_this(), &thread);
	_client->ConnectAsync(ip.c_str(), port, NETLIB_CLIENT_ENABLE_RECONNECT_ON_FIRST_CONNECT | NETLIB_CLIENT_FLAG_KEEP_ALIVE | NETLIB_FLAG_TCP_NODELAY);
	_client->SetKeepAliveIntervalSeconds(30);
}

void UserSimulator::Register()
{
	//lalune::AutoRegisterRequest auto_register;
	//auto_register.set_nick(utility1::generateRandomString(6));
	//SendMsg(MSG_TYPE_AUTOREGISTER_REQUEST, auto_register);
}
void UserSimulator::Version(const std::string& version_name)
{
	//lalune::CheckVersion now_version;
	//now_version.set_version_name(version_name);
	////now_version.set_nick(utility1::generateRandomString(6));
	//SendMsg(MSG_CHECK_VERSION, now_version);
}
void UserSimulator::Match()
{
	boids::MatchRequest match_req;
	match_req.set_user_id(utility1::generateRandomString(6));
	match_req.set_map_name("map01");
	SendMsg(boids::AUTO_MATCH_REQUEST, match_req);
}
void UserSimulator::Combat(int id)
{
	//lalune::ConnectToGame connect;
	//p_id = id;
	//connect.set_player_uid(p_id); 
	//connect.set_access_token("111");
	////now_version.set_nick(utility1::generateRandomString(6));
	//SendMsg(MSG_TYPE_SYNC_BATTLE_CONNECT_TO_GAME, connect);
}

void UserSimulator::ConnectedHandler(NetLib_Client_ptr clientptr)
{

}
//void UserSimulator::StartupTimer(const boost::system::error_code& error)
//{
//	if (!error)
//	{
//
//		lalune::GameAction action;
//		timer1.expires_from_now(boost::posix_time::milliseconds(400));
//		timer1.async_wait(boost::bind(&UserSimulator::StartupTimer, this, boost::asio::placeholders::error));
//		uint64_t time;
//		time = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();
//		action.set_action_data(std::string((char*)&time, 8));
//		action.set_player_uid(p_id);
//		SendMsg(MSG_TYPE_SYNC_BATTLE_GAME_ACTION, action);
//	}
//}
//int tm = 0;
void UserSimulator::ReconnectedHandler(NetLib_Client_ptr clientptr)
{
	//lalune::ConnectToGame connect;
	//connect.set_player_uid(p_id);
	//connect.set_access_token("111");
	////now_version.set_nick(utility1::generateRandomString(6));
	//SendMsg(MSG_TYPE_SYNC_BATTLE_CONNECT_TO_GAME, connect);

}
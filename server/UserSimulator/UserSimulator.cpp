﻿#include <boost/asio.hpp>
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
#include "MessageTypeDef.h"
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
/*boost::asio::deadline_timer timer1(thread.get_ioservice());
int p_id;
std::vector<uint64_t> ping[10];
std::string section[10];
uint64_t time_begin, time_now;
int tm2;*/
 UserSimulator::UserSimulator():timer1(thread.get_ioservice())
{
	 memset(ping, 0, sizeof(ping));
	 for (int i = 0; i < PING_RANGE_COUNT; ++i)
	 {
		 section[i] = "[" + utility1::int2str(i * 10) + "ms-" + utility1::int2str((i + 1) * 10) + "ms]";
	 }
	 section[PING_RANGE_COUNT] = "[" + utility1::int2str(PING_RANGE_COUNT * 10) + "ms+]";
}

void UserSimulator::Connect(const std::string& ip, int port)
{
	_client = NetLib_NewClient(shared_from_this(), &thread);
	_client->ConnectAsync(ip.c_str(), port, NETLIB_CLIENT_ENABLE_RECONNECT_ON_FIRST_CONNECT | NETLIB_CLIENT_FLAG_KEEP_ALIVE | NETLIB_FLAG_TCP_NODELAY);
	_client->SetKeepAliveIntervalSeconds(30);
}

void UserSimulator::Register()
{
	lalune::AutoRegisterRequest auto_register;
	auto_register.set_nick(utility1::generateRandomString(6));
	SendMsg(MSG_TYPE_AUTOREGISTER_REQUEST, auto_register);
}
void UserSimulator::Version(const std::string& version_name)
{
	lalune::CheckVersion now_version;
	now_version.set_version_name(version_name);
	//now_version.set_nick(utility1::generateRandomString(6));
	SendMsg(MSG_CHECK_VERSION, now_version);
}

void UserSimulator::Combat(int id)
{
	lalune::ConnectToGame connect;
	random_generator rgen;//随机生成器
	uuid u = rgen();
	p_id = unsigned int(*(char *)&u);
	std::cout << p_id << std::endl;
	connect.set_player_uid(p_id);
	connect.set_access_token("111");
	//now_version.set_nick(utility1::generateRandomString(6));
	SendMsg(MSG_TYPE_SYNC_BATTLE_CONNECT_TO_GAME, connect);
}

void UserSimulator::ConnectedHandler(NetLib_Client_ptr clientptr)
{

}
void UserSimulator::StartupTimer(const boost::system::error_code& error)
{
	if (!error)
	{

		lalune::GameAction action;
		timer1.expires_from_now(boost::posix_time::milliseconds(400));
		timer1.async_wait(boost::bind(&UserSimulator::StartupTimer, this, boost::asio::placeholders::error));
		uint64_t time;
		time = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();
		action.set_action_data(std::string((char*)&time, 8));
		action.set_player_uid(p_id);
		SendMsg(MSG_TYPE_SYNC_BATTLE_GAME_ACTION, action);
	}
}
//int tm = 0;
void UserSimulator::ReconnectedHandler(NetLib_Client_ptr clientptr)
{
	lalune::ConnectToGame connect;
	connect.set_player_uid(p_id);
	connect.set_access_token("111");
	//now_version.set_nick(utility1::generateRandomString(6));
	SendMsg(MSG_TYPE_SYNC_BATTLE_CONNECT_TO_GAME, connect);

}
void UserSimulator::RecvFinishHandler(NetLib_Client_ptr clientptr, char* data)
{
	if (MSG_LENGTH(data) >= MSG_HEADER_BASE_SIZE)
	{
		switch (MSG_TYPE(data))
		{
		case MSG_TYPE_AUTOREGISTER_RESPONSE:
		{
			lalune::AutoRegisterResponce response;
			if (ParseMsg(data, response))
			{
				LOGEVENTL("INFO", "AutoRegisterResponse. " << _ln("code") << response.code()
					<< _ln("uid") << response.uid() << _ln("pwd") << response.pwd()
					<< _ln("errStr") << response.errstr());
			}
		}break;
		case MSG_CHECK_VERSION_RESULT:
		{
										 lalune::CheckVersionResult  test_result;
										 test_result.ParseFromArray(MSG_DATA(data), MSG_DATA_LEN(data));
										 std::cout<<test_result.now_version()<<std::endl;
										 for (int i = 0; i < test_result.file_size(); i++)
										 {
											 std::cout << test_result.file(i).file_path() << std::endl;
											 std::cout << test_result.file(i).url_prefix() << std::endl;
										 }
										 
		}break;
		case MSG_TYPE_SYNC_BATTLE_GAME_START:
		{
						std::cout << "start" << std::endl;
						time_begin = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();
						
						timer1.expires_from_now(boost::posix_time::milliseconds(400));
						timer1.async_wait(boost::bind(&UserSimulator::StartupTimer,this, boost::asio::placeholders::error));
						
		}break;
		case MSG_TYPE_SYNC_BATTLE_GAME_ACTION:
		{
												 uint32_t empty = 4;
												 _client->SendCopyAsync((const char*)(&empty));
												 
						 lalune::GameActions actions;
						 lalune::GameAction action;
						 int i;
						 actions.ParseFromArray(MSG_DATA(data), MSG_DATA_LEN(data));
						 for (i = 0; i < actions.actions_size(); i++)
						 {

							 if ((int)(actions.actions(i).player_uid()) == p_id)
							 {

								 uint64_t time, time_ping;
								 time = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();
								// std::cout << *(uint64_t*)(actions.actions(i).action_data().c_str()) << std::endl;
								// std::cout <<(time- *(uint64_t*)(actions.actions(i).action_data().c_str())) << std:: endl;
								 time_ping = (time - *(uint64_t*)(actions.actions(i).action_data().c_str()));

								 int range = time_ping / 1000 / 10;

								 if (range >= PING_RANGE_COUNT)
								 {
									 ping[PING_RANGE_COUNT] ++;
								 }
								 else
								 {
									 ping[range] ++;
								 }
							 }
						 }
						 time_now = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();

						 if ((time_now - time_begin) / 1000000 >= 30 )
						 {
							 std::cout << p_id << std::endl;
							 LOGEVENTL("info","p_id"<<p_id);
							 time_begin = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();
							 for (i = 0; i <= PING_RANGE_COUNT; i++)
							 {
								 //std::cout << section[i] << ping[i] << std::endl;
								 LOGEVENTL("PING", _ln(section[i]) << ping[i]);
							 }
						 }
			 
		}
		default:
			break;
		}
	}
}
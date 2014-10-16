#pragma once
#include "CombateCommon.h"
#include "NetLib/NetLib.h"
#include "include/ioservice_thread.h"
#include "MessageTypeDef.h"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "Log/Log.h"
#include <google/protobuf/stubs/common.h>
#include "Battle.pb.h"
#include "include/ptime2.h"
#include <map>
#include <vector>
#include <boost/bind.hpp>
using namespace std; 
class OneGame
{
public:
	OneGame() 
	{
		timer = new boost::asio::deadline_timer(_thread.get_ioservice());
	}
	~OneGame()
	{
		delete timer;
	}
	void ConnectToGame(NetLib_ServerSession_ptr sessionptr, char *data);
	void BattleGameAction(NetLib_ServerSession_ptr sessionptr, char *data);
	void StartupTimer(const boost::system::error_code& error);
	void DelConnect(NetLib_ServerSession_ptr sessionptr)
	{
		client_connect.erase(sessionptr);
	}
	
private:
	map<NetLib_ServerSession_ptr, int> client_connect;
	uint64_t start_time, now_time;
	vector<lalune::GameAction> actions;
	boost::asio::deadline_timer *timer;

};


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
class OneGame : public std::enable_shared_from_this<OneGame>
{
public:
	OneGame() 
	{
		timer = std::make_shared<boost::asio::deadline_timer>(_thread.get_ioservice());
	}
	~OneGame()
	{
	}
	void ConnectToGame(NetLib_ServerSession_ptr sessionptr, char *data);
	void BattleGameAction(NetLib_ServerSession_ptr sessionptr, char *data);
	void ActionsReturn(shared_ptr<OneGame>, const boost::system::error_code& error);
	void DelConnect(NetLib_ServerSession_ptr sessionptr)
	{
		client_connect.erase(sessionptr);
	}
	map<NetLib_ServerSession_ptr, unsigned int> client_connect;
	uint64_t start_time, now_time;
	vector<lalune::GameAction> actions;
	std::shared_ptr<boost::asio::deadline_timer> timer;
	
private:

};


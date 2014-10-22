#pragma once
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
#include <boost/asio.hpp>
#include <google/protobuf/stubs/common.h>
#include <boost/thread.hpp>
using namespace std; 
using boost::asio::ip::udp;
class OneGameUdp
{
public:
	void ConnectToGame(udp::socket &socket_server, udp::endpoint client_endpoint, char *data);
	void BattleGameAction(udp::socket &socket_server, udp::endpoint client_endpoint, char *data);
	/*void DelConnect(NetLib_ServerSession_ptr sessionptr)
	{
		client_connect.erase(sessionptr);
	}*/
	void SendFinished(char *data, const boost::system::error_code &error, std::size_t)
	{
		delete []data;
	}
	void ReceivedFinishend(const boost::system::error_code &error, std::size_t)
	{
	}
	map<udp::endpoint, unsigned int> client_connect;
	uint64_t start_time, now_time;
	vector<lalune::GameAction> actions;
private:

};


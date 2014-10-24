#include <boost/asio.hpp>
#include <google/protobuf/stubs/common.h>
#include <boost/thread.hpp>
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
#include <string>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "OneGameUdp.h"
#include "ServerCommonLib/ServerCommon.h"
using boost::asio::ip::udp;
using namespace boost::uuids;
using namespace std;
map<string, shared_ptr<OneGameUdp>> gameid_to_onegame;//游戏id到具体游戏映射
map<udp::endpoint, string> ptr_to_gameid;//连接到游戏id 的映射
ioservice_thread _thread;
uuid now_game_id;
NetLib_Server_ptr server;
#define COMBAT_MATCH_PORT 5710
#define COMBAT_USER_PORT 5350
class CombatServer
{
public:
	CombatServer(boost::asio::io_service &io_service, short port) :socket_server(io_service,udp::endpoint(udp::v4(),port))
	{	
		Receive();
	}
	//~CombatServer()
	//{
	//}
	void Receive()
	{
		char *receive_buff = new char[200];
		socket_server.async_receive_from(
			boost::asio::buffer(receive_buff, 200), 
			client_endpoint,
			boost::bind(&CombatServer::ReceivedFinishend, this, receive_buff, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
			);
	}
	void SendFinished(char *data, const boost::system::error_code &error, std::size_t)
	{
		delete []data;
	}
	void ReceivedFinishend( char *data, const boost::system::error_code &error, std::size_t)
	{
		//std::string str = (char *)&client_endpoint.address();
		//std::cout << client_endpoint_one.address() << std::endl;
		//std::cout << client_endpoint_one.port() << std::endl;
		if (!error || error == boost::asio::error::message_size)
		
		{
			switch (MSG_TYPE(data))
			{
			case MSG_TYPE_SYNC_BATTLE_CONNECT_TO_GAME:
			{
				//if (gameid_to_onegame.size() == 0 || (gameid_to_onegame[now_game_id]->client_connect.size() == 6))
				//{
				//	random_generator rgen;//随机生成器
				//	uuid u = rgen();//生成一个随机的UUID
				//	shared_ptr<OneGameUdp> one_game_temp = std::make_shared<OneGameUdp>();
				//	gameid_to_onegame[u] = one_game_temp;
				//	now_game_id = u;
				//}
				lalune::ConnectToGame proto_connect;
				proto_connect.ParseFromArray(MSG_DATA(data), MSG_DATA_LEN(data));
				cout << proto_connect.access_token() << endl;
				gameid_to_onegame[proto_connect.access_token()]->ConnectToGame(socket_server, client_endpoint, data);
				ptr_to_gameid[client_endpoint] = proto_connect.access_token();
				cout << client_endpoint << endl;

			}break;
			case MSG_TYPE_SYNC_BATTLE_GAME_ACTION:
			{
				 if (gameid_to_onegame.count(ptr_to_gameid[client_endpoint]) == 1)
				 {
					 gameid_to_onegame[ptr_to_gameid[client_endpoint]]->BattleGameAction(socket_server, client_endpoint, data);
				 }

			}break;
			default:
				break;
			}
			delete[]data;
			Receive();
		}
	}
private:
	udp::socket socket_server;
	udp::endpoint client_endpoint;
	
};
class CombatMatchCommunicate : public NetLib_ServerSession_Delegate
{
public:
	void RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
	{
		if (SERVER_MSG_LENGTH(data) >= SERVER_MSG_HEADER_BASE_SIZE)
		{
			switch (SERVER_MSG_TYPE(data))
			{
			case MSG_TYPE_SYNC_BATTLE_CREATE_GAME:
			{
				lalune::SendGameId proto_game_id;
				proto_game_id.ParseFromArray(SERVER_MSG_DATA(data), SERVER_MSG_DATA_LEN(data));
				std::string str = proto_game_id.game_id();
				shared_ptr<OneGameUdp> one_game_temp = std::make_shared<OneGameUdp>();
				gameid_to_onegame[str] = one_game_temp;
				std::cout << str << endl;
			}break;
			}
		}
	}

};
class CombatServerTcp:public CommonLibDelegate
{
public:
	void onConfigInitialized()
	{
		server = NetLib_NewServer<CombatMatchCommunicate>(&_thread);
		if (!server->StartTCP(COMBAT_MATCH_PORT, 1, 120)) //端口，线程数，超时时间
		{
			LOGEVENTL("Error", "Server Start Failed !");
			exit(0);
		}
		ServerStarted(COMBAT_MATCH_PORT);
		LOGEVENTL("INFO", "CombatServer Start Success");
	}
	

};
int main(int argc, char* argv[])
{
	_thread.start();
	//boost::asio::io_service io_service;
	LogInitializeLocalOptions(true, true, "match_server");
	CombatServerTcp* cl_delegate = new CombatServerTcp();
	InitializeCommonLib(_thread, cl_delegate, SERVER_TYPE_SYNC_BATTLE_SERVER, argc, argv);
	CombatServer server(_thread.get_ioservice(), COMBAT_USER_PORT);
#if WIN32
	Sleep(-1);
#else
	sleep(-1);
#endif
	return 0;
}

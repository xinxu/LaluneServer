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
using boost::asio::ip::udp;
using namespace boost::uuids;
using namespace std;
map<uuid, shared_ptr<OneGameUdp>> gameid_to_onegame;//游戏id到具体游戏映射
map<udp::endpoint, uuid> ptr_to_gameid;//连接到游戏id 的映射
uuid now_game_id;
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
			boost::bind(&CombatServer::ReceivedFinishend, this, client_endpoint, receive_buff, boost::asio::placeholders::error)
			);
	}
	void SendFinished(char *data,const boost::system::error_code &error)
	{
		delete []data;
	}
	void ReceivedFinishend(udp::endpoint client_endpoint_one,char *data, const boost::system::error_code &error)
	{
		//std::string str = (char *)&client_endpoint.address();
		//std::cout << client_endpoint_one.address() << std::endl;
		//std::cout << client_endpoint_one.port() << std::endl;
		switch (MSG_TYPE(data))
		{
		case MSG_TYPE_SYNC_BATTLE_CONNECT_TO_GAME:
		{
			if (gameid_to_onegame.size() == 0 || (gameid_to_onegame[now_game_id]->client_connect.size() == 6))
			{
				random_generator rgen;//随机生成器
				uuid u = rgen();//生成一个随机的UUID
				shared_ptr<OneGameUdp> one_game_temp = std::make_shared<OneGameUdp>();
				gameid_to_onegame[u] = one_game_temp;
			    now_game_id = u;
			}
			gameid_to_onegame[now_game_id]->ConnectToGame(socket_server, client_endpoint_one, data);
			ptr_to_gameid[client_endpoint_one] = now_game_id;

		}break;
		case MSG_TYPE_SYNC_BATTLE_GAME_ACTION:
		{
			if (gameid_to_onegame.count(ptr_to_gameid[client_endpoint_one]) == 1)
			{
				gameid_to_onegame[ptr_to_gameid[client_endpoint_one]]->BattleGameAction(socket_server, client_endpoint_one, data);
			}

		}break;
		default:
			break;
		}
		delete []data;
		Receive();
	}
private:
	udp::socket socket_server;
	udp::endpoint client_endpoint;
	//char receive_buff[200];
	//char send_buff[200];
};
int main()
{
	boost::asio::io_service io_service;
	CombatServer server(io_service, 5350);
	io_service.run();
#if WIN32
	Sleep(-1);
#else
	sleep(-1);
#endif
	return 0;
}
/** @file UdpEchoServer.cpp
*  @note Hangzhou Hikvision System Technology Co., Ltd. All Rights Reserved.
*  @brief an udp server, echo what the client say.
*
*  @author Zou Tuoyu
*  @date 2012/11/28
*
*  @note 历史记录：
*  @note V1.0.0.0 创建
*/

////boost
//#include <boost/asio.hpp>
//
////stl
//#include <string>
//#include <iostream>
//
//using namespace std;
//using boost::asio::ip::udp;
//int main()
//{
//	boost::asio::io_service io_service;
//	boost::asio::ip::udp::socket udp_socket(io_service, udp::endpoint(udp::v4(), 7474));
//	//boost::asio::ip::udp::endpoint local_add(boost::asio::ip::address::from_string("127.0.0.1"), 7474);
//
//	//udp_socket.open(local_add.protocol());
//	//udp_socket.bind(local_add);
//
//	char receive_buffer[1024] = { 0 };
//	while (true)
//	{
//		boost::asio::ip::udp::endpoint send_point;
//		udp_socket.receive_from(boost::asio::buffer(receive_buffer, 1024), send_point);
//		cout << "recv:" << receive_buffer << endl;
//		udp_socket.send_to(boost::asio::buffer(receive_buffer), send_point);
//
//		memset(receive_buffer, 0, 1024);
//	}
//
//	return 1;
//}
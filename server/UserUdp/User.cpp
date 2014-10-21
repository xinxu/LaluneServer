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
#include "include/utility1.h"
using boost::asio::ip::udp;
#define PING_RANGE_COUNT (30)
ioservice_thread _thread;
int ping[PING_RANGE_COUNT + 1];
std::string section[PING_RANGE_COUNT + 1];
class User
{
public:
	User(boost::asio::io_service &io_service,int id) :socket_client(io_service), server_endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 5350),
		timer1(io_service)
	{
		p_id = id;
		socket_client.open(server_endpoint.protocol());
		Init();
		Receive();
	}
	void Init()
	{
		lalune::ConnectToGame connect;
		connect.set_player_uid(p_id);
		connect.set_access_token("111");
		int proto_size = connect.ByteSize();
		char* send_buff = new char[MSG_HEADER_BASE_SIZE + proto_size];
		MSG_LENGTH(send_buff) = MSG_HEADER_BASE_SIZE + proto_size;		// 数据包的字节数（含msghead）
		MSG_TYPE(send_buff) = MSG_TYPE_SYNC_BATTLE_CONNECT_TO_GAME;
		memset(send_buff + MSG_AFTER_TYPE_POS, 0, MSG_HEADER_BASE_SIZE - MSG_AFTER_TYPE_POS);
		connect.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)MSG_DATA(send_buff));
		socket_client.async_send_to(boost::asio::buffer(send_buff, MSG_LENGTH(send_buff)+1),
			server_endpoint,
			boost::bind(&User::SendFinished, this,send_buff, boost::asio::placeholders::error)
			);
	}
	void Receive()
	{
		char *receive_buff = new char[200];
		socket_client.async_receive_from(
			boost::asio::buffer(receive_buff, 200),
			server_endpoint,
			boost::bind(&User::ReceiveFinished, this, receive_buff, boost::asio::placeholders::error)
			);
	}
	void User::PlayAction(const boost::system::error_code& error)
	{
		if (!error)
		{

			lalune::GameAction action;
			timer1.expires_from_now(boost::posix_time::milliseconds(400));
			timer1.async_wait(boost::bind(&User::PlayAction, this, boost::asio::placeholders::error));
			uint64_t time;
			time = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();
			action.set_action_data(std::string((char*)&time, 8));
			action.set_player_uid(p_id);
			int proto_size = action.ByteSize();
			char* send_buff = new char[MSG_HEADER_BASE_SIZE + proto_size];
			MSG_LENGTH(send_buff) = MSG_HEADER_BASE_SIZE + proto_size;		// 数据包的字节数（含msghead）
			MSG_TYPE(send_buff) = MSG_TYPE_SYNC_BATTLE_GAME_ACTION;
			memset(send_buff + MSG_AFTER_TYPE_POS, 0, MSG_HEADER_BASE_SIZE - MSG_AFTER_TYPE_POS);
			action.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)MSG_DATA(send_buff));
			socket_client.async_send_to(boost::asio::buffer(send_buff, MSG_LENGTH(send_buff) + 1),
				server_endpoint,
				boost::bind(&User::SendFinished, this, send_buff, boost::asio::placeholders::error)
				);
			//SendMsg(MSG_TYPE_SYNC_BATTLE_GAME_ACTION, action);
		}
	}
	void ReceiveFinished(char *data, const boost::system::error_code &error)
	{
		
		switch (MSG_TYPE(data))
		{
		case MSG_TYPE_SYNC_BATTLE_GAME_START:
		{
			std::cout << "start" << std::endl;
            timer1.expires_from_now(boost::posix_time::milliseconds(400));
			timer1.async_wait(boost::bind(&User::PlayAction, this, boost::asio::placeholders::error));

		}break;
		case MSG_TYPE_SYNC_BATTLE_GAME_ACTION:
		{
												// cout << "xx" << endl;
			 lalune::GameAction action;
			 int i;
			 action.ParseFromArray(MSG_DATA(data), MSG_DATA_LEN(data));
			 if ((int)(action.player_uid()) == p_id)
			 {

				 uint64_t time, time_ping;
				 time = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();
				 // std::cout << *(uint64_t*)(actions.actions(i).action_data().c_str()) << std::endl;
				 // std::cout <<(time- *(uint64_t*)(actions.actions(i).action_data().c_str())) << std:: endl;
				 time_ping = (time - *(uint64_t*)(action.action_data().c_str()));

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
		
		}break;
		default:
			break;
		}

		delete []data;
		Receive();
	}
	void SendFinished(char *data, const boost::system::error_code &error)
	{
		//std::cout << "xxzx " << std::endl;
		delete []data;
	}
private:
	udp::socket socket_client;
	udp::endpoint server_endpoint;
	boost::asio::deadline_timer timer1;
	int p_id;

};
int main()
{
	LogInitializeLocalOptions(true, true, "combat_udp");
	memset(ping, 0, sizeof(ping));
	for (int i = 0; i < PING_RANGE_COUNT; ++i)
	{
		section[i] = "[" + utility1::int2str(i * 10) + "ms-" + utility1::int2str((i + 1) * 10) + "ms]";
	}
	section[PING_RANGE_COUNT] = "[" + utility1::int2str(PING_RANGE_COUNT * 10) + "ms+]";
	boost::asio::ip::address addr = boost::asio::ip::address::from_string("127.0.0.1");
	udp::endpoint server_endpoint(addr, 5350);
	int i;
	//boost::asio::io_service io_service;
	_thread.start();
	for (i = 0; i < 6; i++)
	{
		User *server=new User(_thread.get_ioservice(),i);
		//io_service[i].run();
	}
	//_thread.get_ioservice().run();
	//io_service.run();
	uint64_t time_begin, time_now;
	time_begin = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();
	while (1)
	{
		time_now = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();

		if ((time_now - time_begin) / 1000000 >= 31)
		{
			int i;
			time_begin = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();
			for (i = 0; i <= PING_RANGE_COUNT; i++)
			{
				//std::cout << section[i] << ping[i] << std::endl;
				LOGEVENTL("PING", _ln(section[i]) << ping[i]);
			}
		}
	}
	Sleep(-1);
	return 0;

}

//#include "boost/asio.hpp"
//
////stl
//#include <iostream>
//
//using namespace std;
//
//int main()
//{
//	boost::asio::io_service io_service;
//	boost::asio::ip::udp::socket socket(io_service);
//	boost::asio::ip::udp::endpoint end_point(boost::asio::ip::address::from_string("127.0.0.1"), 5350);
//	socket.open(end_point.protocol());
//	char receive_buffer[1024] = { 0 };
//	while (true)
//	{
//		cout << "input:";
//		string input_data;
//		cin >> input_data;
//		cout << endl;
//
//		try
//		{
//			socket.send_to(boost::asio::buffer(input_data.c_str(), 500), end_point);
//
//		
//		}
//		catch (boost::system::system_error &e)
//		{
//			cout << "process failed:" << e.what() << endl;
//		}
//	}
//}
#include "OneGameUdp.h"
void OneGameUdp::ConnectToGame(udp::socket &socket_server,udp::endpoint client_endpoint, char *data)
{
	lalune::ConnectToGame proto_connect;
	proto_connect.ParseFromArray(MSG_DATA(data), MSG_DATA_LEN(data));
	client_connect[client_endpoint] = proto_connect.player_uid();
	cout << proto_connect.player_uid() << endl;

	//返回客户端确认
	lalune::ConnectToGameResponse proto_connect_response;
	proto_connect_response.set_team(client_connect.size() % 2);
	int proto_size = proto_connect_response.ByteSize();
	char* send_buf = new char[MSG_HEADER_BASE_SIZE + proto_size];
	MSG_LENGTH(send_buf) = MSG_HEADER_BASE_SIZE + proto_size;		// 数据包的字节数（含msghead）
	MSG_TYPE(send_buf) = MSG_TYPE_SYNC_BATTLE_CONNECT_TO_GAME_RESPONSE;
	MSG_ERROR(send_buf) = 0;
	MSG_ENCRYPTION_TYPE(send_buf) = 0;
	MSG_RESERVED(send_buf) = 0;
	proto_connect_response.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)MSG_DATA(send_buf));
	socket_server.async_send_to(boost::asio::buffer(send_buf, MSG_LENGTH(send_buf) + 1),
		client_endpoint,
		boost::bind(&OneGameUdp::SendFinished, this, send_buf, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
		);

	//开始游戏
	const int player_num = 6;
	cout << client_connect.size() << endl;
	if (client_connect.size() == player_num)
	{
		//char* send_buf = new char[MSG_HEADER_BASE_SIZE + proto_size];
		//MSG_LENGTH(send_buf) = MSG_HEADER_BASE_SIZE + proto_size;		// 数据包的字节数（含msghead）
		//MSG_TYPE(send_buf) = MSG_TYPE_SYNC_BATTLE_GAME_START;
		//MSG_ERROR(send_buf) = 0;
		//MSG_ENCRYPTION_TYPE(send_buf) = 0;
		//MSG_RESERVED(send_buf) = 0;
		//proto_game_start.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)MSG_DATA(send_buf));
		start_time = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();
		auto player = client_connect.begin();
		for (; player != client_connect.end(); player++)
		{
			lalune::GameStart proto_game_start;
			proto_game_start.set_rand_seed(0);
			proto_game_start.set_time_per_frame(1000 / 33);
			int proto_size = proto_game_start.ByteSize();
			char* send_buf = new char[MSG_HEADER_BASE_SIZE + proto_size];
			MSG_LENGTH(send_buf) = MSG_HEADER_BASE_SIZE + proto_size;		// 数据包的字节数（含msghead）
			MSG_TYPE(send_buf) = MSG_TYPE_SYNC_BATTLE_GAME_START;
			MSG_ERROR(send_buf) = 0;
			MSG_ENCRYPTION_TYPE(send_buf) = 0;
			MSG_RESERVED(send_buf) = 0;
			proto_game_start.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)MSG_DATA(send_buf));
			//start_time = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();
			socket_server.async_send_to(boost::asio::buffer(send_buf, MSG_LENGTH(send_buf) + 1),
				player->first,
				boost::bind(&OneGameUdp::SendFinished, this, send_buf, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
				);
			//cout << player->first.port() << endl;
		}
	}

}
void OneGameUdp::BattleGameAction(udp::socket &socket_server, udp::endpoint client_endpoint, char *data)
{

	//lalune::GameAction proto_game_action;
	//proto_game_action.ParseFromArray(MSG_DATA(data), MSG_DATA_LEN(data));
	//uint64_t now_time = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();
	//proto_game_action.set_time_from_game_start(now_time - start_time);
	//int proto_size = proto_game_action.ByteSize();
	//char* send_buf = new char[MSG_HEADER_BASE_SIZE + proto_size];
	//MSG_LENGTH(send_buf) = MSG_HEADER_BASE_SIZE + proto_size;		// 数据包的字节数（含msghead）
	//MSG_TYPE(send_buf) = MSG_TYPE_SYNC_BATTLE_GAME_ACTION;
	//MSG_ERROR(send_buf) = 0;
	//MSG_ENCRYPTION_TYPE(send_buf) = 0;
	//MSG_RESERVED(send_buf) = 0;
	//proto_game_action.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)MSG_DATA(send_buf));
	auto player = client_connect.begin();
	for (; player != client_connect.end(); player++)
	{
		lalune::GameAction proto_game_action;
		proto_game_action.ParseFromArray(MSG_DATA(data), MSG_DATA_LEN(data));
		uint64_t now_time = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();
		proto_game_action.set_time_from_game_start(now_time - start_time);
		int proto_size = proto_game_action.ByteSize();
		char* send_buf = new char[MSG_HEADER_BASE_SIZE + proto_size];
		MSG_LENGTH(send_buf) = MSG_HEADER_BASE_SIZE + proto_size;		// 数据包的字节数（含msghead）
		MSG_TYPE(send_buf) = MSG_TYPE_SYNC_BATTLE_GAME_ACTION;
		MSG_ERROR(send_buf) = 0;
		MSG_ENCRYPTION_TYPE(send_buf) = 0;
		MSG_RESERVED(send_buf) = 0;
		proto_game_action.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)MSG_DATA(send_buf));
		socket_server.async_send_to(boost::asio::buffer(send_buf, MSG_LENGTH(send_buf) + 1),
			player->first,
			boost::bind(&OneGameUdp::SendFinished, this, send_buf, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred)
			);
	}

}
//void OneGame::ActionsReturn(shared_ptr<OneGame> keep_alive, const boost::system::error_code& error)
//{
//	if (!error)
//	{
//
//		const int time_space = 33;
//		//boost::asio::deadline_timer timer(_thread.get_ioservice());
//		timer->expires_from_now(boost::posix_time::milliseconds(time_space));
//		timer->async_wait(boost::bind(&OneGame::ActionsReturn, this, shared_from_this(),  boost::asio::placeholders::error));
//
//		lalune::GameActions proto_actions;
//		auto action_temp = actions.begin();
//		for (; action_temp != actions.end(); action_temp++)
//		{
//			lalune::GameAction *proto_action;
//			proto_action = proto_actions.add_actions();
//			*proto_action = *action_temp;
//		}
//		actions.clear();
//		int proto_size = proto_actions.ByteSize();
//		char* send_buf = new char[MSG_HEADER_BASE_SIZE + proto_size];
//		MSG_LENGTH(send_buf) = MSG_HEADER_BASE_SIZE + proto_size;		// 数据包的字节数（含msghead）
//		MSG_TYPE(send_buf) = MSG_TYPE_SYNC_BATTLE_GAME_ACTION;
//		MSG_ERROR(send_buf) = 0;
//		MSG_ENCRYPTION_TYPE(send_buf) = 0;
//		MSG_RESERVED(send_buf) = 0;
//		proto_actions.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)MSG_DATA(send_buf));
//		auto player = client_connect.begin();
//		for (; player != client_connect.end(); player++)
//		{
//			player->first->SendCopyAsync(send_buf);
//		}
//	}
//}
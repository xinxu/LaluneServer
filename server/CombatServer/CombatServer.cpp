#include "NetLib\NetLib.h"
#include "include/ioservice_thread.h"
#include "MessageTypeDef.h"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "Log\Log.h"
#include <google/protobuf/stubs/common.h>
#include "Battle.pb.h"
#include "include/ptime2.h"
#include <map>
using namespace std;
class CombatServerSessionDelegate : public NetLib_ServerSession_Delegate
{
private:
	map<NetLib_ServerSession_ptr,int> client_connect;
	uint64_t start_time, now_time;
public:
	//void ConnectedHandler(NetLib_ServerSession_ptr sessionptr);

	void DisconnectedHandler(NetLib_ServerSession_ptr sessionptr);
	void ConnectToGame(NetLib_ServerSession_ptr sessionptr,char *data)
	{
		lalune::ConnectToGame proto_connect;
		proto_connect.ParseFromArray(MSG_DATA(data), MSG_DATA_LEN(data));
		client_connect[sessionptr] = proto_connect.player_uid();

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
		sessionptr->SendCopyAsync(send_buf);

		//开始游戏
		const int player_num = 2;
		if (client_connect.size() == player_num)
		{
			lalune::GameStart proto_game_start;
			proto_game_start.set_rand_seed(0);
			proto_game_start.set_time_per_frame( 1000 / 33);
			int proto_size = proto_game_start.ByteSize();
			char* send_buf = new char[MSG_HEADER_BASE_SIZE + proto_size];
			MSG_LENGTH(send_buf) = MSG_HEADER_BASE_SIZE + proto_size;		// 数据包的字节数（含msghead）
			MSG_TYPE(send_buf) = MSG_TYPE_SYNC_BATTLE_GAME_START;
			MSG_ERROR(send_buf) = 0;
			MSG_ENCRYPTION_TYPE(send_buf) = 0;
			MSG_RESERVED(send_buf) = 0;
			proto_game_start.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)MSG_DATA(send_buf));
			uint64_t start_time = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();
			sessionptr->SendCopyAsync(send_buf);
		}
	}
	void BattleGameAction(NetLib_ServerSession_ptr sessionptr,char *data)//战斗包反馈
	{
		lalune::GameAction proto_game_action;
		proto_game_action.ParseFromArray(MSG_DATA(data), MSG_DATA_LEN(data));
		uint64_t now_time = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();
		proto_game_action.set_time_from_game_start(now_time - start_time);
		auto map_temp = client_connect.begin();
		for (; map_temp != client_connect.end(); map_temp++)
		{
			proto_game_action.set_player_uid(map_temp->second);
			proto_game_action.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)MSG_DATA(data));
			sessionptr->SendCopyAsync(data);
		}
	}

	void RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
	{
		//....
		switch (MSG_TYPE(data))
		{
		case MSG_TYPE_SYNC_BATTLE_CONNECT_TO_GAME:
		{
													 ConnectToGame(sessionptr, data);

		}break;
		case MSG_TYPE_SYNC_BATTLE_GAME_ACTION:
		{
												 BattleGameAction(sessionptr, data);

		}break;
		default:
			break;
		}

	}
};
int main()
{
	ioservice_thread _thread;
	auto server4user = NetLib_NewServer<CombatServerSessionDelegate>(&_thread);

	//超时时间得可以中途重设 TODO
	if (!server4user->StartTCP(5000, 1, 25)) //端口，线程数，超时时间  （客户端现在是15秒发个心跳包）
	{
		LOGEVENTL("Error", "Server4User Start Failed !");


	//	NetLib_Servers_WaitForStop();

	//	LogUnInitialize();

	//	google_lalune::protobuf::ShutdownProtobufLibrary();

		exit(0);
	}

}
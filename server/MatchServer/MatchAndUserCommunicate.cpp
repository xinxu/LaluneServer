#include "MatchAndUserCommunicate.h"
#include "Battle.pb.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <queue>
using namespace boost::uuids;
std::queue<NetLib_ServerSession_ptr> user;
std::queue<std::string> game_id;
const int player_num = 1;
std::shared_ptr<NetLibPlus_Client> client_ptr;
void MatchAndUserCommunicate::RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
{
	if (SERVER_MSG_LENGTH(data) >= SERVER_MSG_HEADER_BASE_SIZE)
	{
		switch (SERVER_MSG_TYPE(data))
		{
		case MSG_TYPE_AUTOMATCH_MATCH_REQUEST:
		{
			/*lalune::MatchRequest match_request;
			match_request.ParseFromArray(SERVER_MSG_DATA(data), MSG_DATA_LEN(data));*/
			user.push(sessionptr);
			if (user.size() == player_num)
			{
				random_generator rgen;//随机生成器
				uuid u = rgen();//生成一个随机的UUID
				std::stringstream ss;
				ss << u;
				std::string game_id_one;
				ss >> game_id_one;
				game_id.push(game_id_one);
				std::cout << game_id_one << std::endl;
				//发送给战斗服务，创建一个房间
				lalune::SendGameId proto_gameid;
				proto_gameid.set_game_id(game_id_one.c_str());
				int proto_size = proto_gameid.ByteSize();
				char* send_buf = new char[SERVER_MSG_HEADER_BASE_SIZE + proto_size];
				SERVER_MSG_LENGTH(send_buf) = SERVER_MSG_HEADER_BASE_SIZE + proto_size;		// 数据包的字节数（含msghead）
				SERVER_MSG_TYPE(send_buf) = MSG_TYPE_SYNC_BATTLE_CREATE_GAME;
				memset(send_buf + SERVER_MSG_AFTER_TYPE_POS, 0, SERVER_MSG_HEADER_BASE_SIZE - SERVER_MSG_AFTER_TYPE_POS);
				proto_gameid.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)SERVER_MSG_DATA(send_buf));
				client_ptr->SendCopyAsync(send_buf);
				delete send_buf;

			}

		}break;
		default:
			break;
		}
	}
}
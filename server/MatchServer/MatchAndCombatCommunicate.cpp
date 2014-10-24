#include "MatchAndCombatCommunicate.h"
#include <queue>
extern std::queue<NetLib_ServerSession_ptr> user;
extern const int player_num = 2;
extern std::shared_ptr<NetLibPlus_Client> client_ptr;
extern std::queue<std::string> game_id;
template<typename P>
void SendMsg(NetLib_ServerSession_ptr &sessionptr, uint32_t msg_type, P& proto)
{
	int proto_size = proto.ByteSize();

	char* send_buf = new char[SERVER_MSG_HEADER_BASE_SIZE + proto_size];
	SERVER_MSG_LENGTH(send_buf) = SERVER_MSG_HEADER_BASE_SIZE + proto_size;		// 数据包的字节数（含msghead）
	SERVER_MSG_TYPE(send_buf) = msg_type;
	memset(send_buf + SERVER_MSG_AFTER_TYPE_POS, 0, SERVER_MSG_HEADER_BASE_SIZE - SERVER_MSG_AFTER_TYPE_POS);

	proto.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)SERVER_MSG_DATA(send_buf));

	sessionptr->SendAsync(send_buf);
}
void MatchAndCombatCommunicate::ConnectedHandler(std::shared_ptr<NetLibPlus_Client> clientptr)
{
	client_ptr = clientptr;

}
void MatchAndCombatCommunicate::ReconnectedHandler(std::shared_ptr<NetLibPlus_Client> clientptr)
{
	client_ptr = clientptr;
}
void MatchAndCombatCommunicate::RecvFinishHandler(std::shared_ptr<NetLibPlus_Client> clientptr, char* data)
{
	if (SERVER_MSG_LENGTH(data) >= SERVER_MSG_HEADER_BASE_SIZE)
	{
		switch (SERVER_MSG_TYPE(data))
		{
		case MSG_TYPE_SYNC_BATTLE_CREATE_GAME_RESPONSE:
		{
			int i;
		    lalune::MatchResponse proto_response;
			std::string game_id_one = game_id.front();
			game_id.pop();
			for (i = 0; i < player_num; i++)
			{
				NetLib_ServerSession_ptr user_one;
				user_one = user.front();
				user.pop();
				SendMsg(user_one, MSG_TYPE_AUTOMATCH_MATCH_REQUEST, proto_response);
			}
		}break;
		}
	}
}
#include "CombateCommon.h"
#include "OneGame.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <list>
using namespace boost::uuids;
ioservice_thread _thread;
//OneGame game0;
map<uuid, shared_ptr<OneGame>> gameid_to_onegame;//游戏id到具体游戏映射
map<NetLib_ServerSession_ptr, uuid> ptr_to_gameid;//连接到游戏id 的映射
uuid now_game_id;
//list<OneGame*> game_all;
//int game_num = 0;
class CombatServerSessionDelegate : public NetLib_ServerSession_Delegate
{
private:
public:
	//void ConnectedHandler(NetLib_ServerSession_ptr sessionptr);
	void DisconnectedHandler(NetLib_ServerSession_ptr sessionptr, NetLib_Error error, int inner_error_code)
	{
		cout << "end" << endl;
		gameid_to_onegame[ptr_to_gameid[sessionptr]]->DelConnect(sessionptr);
		if (gameid_to_onegame.size() != 0&&gameid_to_onegame[ptr_to_gameid[sessionptr]]->client_connect.size() == 0)
		{
			//OneGame *one_game_temp;
			//one_game_temp = gameid_to_onegame[ptr_to_gameid[sessionptr]];
			//delete gameid_to_onegame[ptr_to_gameid[sessionptr]];
			gameid_to_onegame.erase(ptr_to_gameid[sessionptr]);
			//delete one_game_temp;
		}
		ptr_to_gameid.erase(sessionptr);
	}
	
	void RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
	{
		//....
		switch (MSG_TYPE(data))
		{
		case MSG_TYPE_SYNC_BATTLE_CONNECT_TO_GAME:
		{						
													 
				 if (gameid_to_onegame.size() == 0 || (gameid_to_onegame[now_game_id]->client_connect.size() == 6))
				 {
					 random_generator rgen;//随机生成器
					 uuid u = rgen();//生成一个随机的UUID
				//	 assert(u.version() == uuid::version_random_number_based);
				//	 std::cout << u << endl;
					 shared_ptr<OneGame> one_game_temp = std::make_shared<OneGame>();
					 gameid_to_onegame[u] = one_game_temp;
					 now_game_id = u;


				 }
				 gameid_to_onegame[now_game_id]->ConnectToGame(sessionptr, data);
				 ptr_to_gameid[sessionptr] = now_game_id;

		}break;
		case MSG_TYPE_SYNC_BATTLE_GAME_ACTION:
		{
				if (gameid_to_onegame.count(ptr_to_gameid[sessionptr]) == 1)
				{
					 gameid_to_onegame[ptr_to_gameid[sessionptr]]->BattleGameAction(sessionptr, data);
				}

		}break;
		default:
			break;
		}

	}
};
int main()
{
	shared_ptr <NetLib_Server_Interface> server4user;
	_thread.start();
	server4user = NetLib_NewServer<CombatServerSessionDelegate>(&_thread);
	//超时时间得可以中途重设 TODO
	if (!server4user->StartTCP(5000, 1, 25, NETLIB_FLAG_TCP_NODELAY)) //端口，线程数，超时时间  （客户端现在是15秒发个心跳包）
	{
		LOGEVENTL("Error", "Server4User Start Failed !");
		exit(0);
	}
#ifdef WIN32
	Sleep(-1);
#else
	sleep(-1);
#endif
	exit(0);
}
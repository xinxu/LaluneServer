#include "CombateCommon.h"
#include "OneGame.h"
ioservice_thread _thread;
OneGame game0;
class CombatServerSessionDelegate : public NetLib_ServerSession_Delegate
{
private:
	map<int,OneGame> all_game;//游戏id到具体游戏映射
	map<NetLib_ServerSession_ptr, int> ptr_to_gameid;//连接到游戏id 的映射
public:
	//void ConnectedHandler(NetLib_ServerSession_ptr sessionptr);
	void DisconnectedHandler(NetLib_ServerSession_ptr sessionptr, NetLib_Error error, int inner_error_code)
	{
		//cout << "asf" << endl;
		game0.DelConnect(sessionptr);
	}
	
	void RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
	{
		//....
		switch (MSG_TYPE(data))
		{
		case MSG_TYPE_SYNC_BATTLE_CONNECT_TO_GAME:
		{
				 game0.ConnectToGame(sessionptr, data);

		}break;
		case MSG_TYPE_SYNC_BATTLE_GAME_ACTION:
		{
				game0.BattleGameAction(sessionptr, data);

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
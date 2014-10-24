#include "MatchServer.h"
#include "MatchAndUserCommunicate.h"
#include "MatchAndCombatCommunicate.h"
ioservice_thread _thread;
NetLib_Server_ptr server;
#define MATCH_USER_PORT (5700)
class MatchAndUserConnect :public CommonLibDelegate
{
public:
	void onConfigInitialized()
	{
		server = NetLib_NewServer<MatchAndUserCommunicate>(&_thread);
		if (!server->StartTCP(MATCH_USER_PORT, 1, 120)) //端口，线程数，超时时间
		{
			LOGEVENTL("Error", "Server Start Failed !");
			exit(0);
		}

		ServerStarted(MATCH_USER_PORT);
	}
private:

};

int main(int argc, char* argv[])
{
	_thread.start();
	LogInitializeLocalOptions(true, true, "match_server");
	MatchAndUserConnect* cl_delegate = new MatchAndUserConnect();
	InitializeCommonLib(_thread, cl_delegate, SERVER_TYPE_AUTO_MATCH_SERVER, argc, argv);
	NetLibPlus_InitializeClients<MatchAndCombatCommunicate>(SERVER_TYPE_SYNC_BATTLE_SERVER);
#if WIN32
	Sleep(-1);
#else
	sleep(-1);
#endif
	return 0;

}
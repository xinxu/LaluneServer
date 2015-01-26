#include <boost/asio.hpp>
#include "../include/ioservice_thread.h"
#include "../Log/Log.h"
#include "../NetLib/NetLib.h"
#include "memory.h"
#include "string.h"
#include <google/protobuf/stubs/common.h>
#include "../../../LaluneCommon/include/MessageTypeDef.h"
#include "../ServerCommonLib/ServerCommon.h"
#include "AccountServerSessionDelegate.h"
#include "mongo/client/dbclient.h"
#include "AccountServerConfig.h"

#define ACCOUNT_SERVER_PORT (6834)

ioservice_thread thread;

boost::mutex user_id_mutex;

long long nextUserId = 0;

long long getNextUserId() {
    user_id_mutex.lock();
    long long ret = nextUserId++;
    user_id_mutex.unlock();
    return ret;
}

void setNextUserId( long long value ) {
    user_id_mutex.lock();
    nextUserId = value;
    user_id_mutex.unlock();
}

class AccountServerCommonLibDelegate : public CommonLibDelegate
{
public:
	void onConfigRefresh(const std::string& content)
	{

	}
};

int main(int argc, char* argv[])
{		
	//Check Memory Leaks
#if WIN32
	// Get the current bits
	int tmp = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);

	tmp |= _CRTDBG_LEAK_CHECK_DF;

	// Set the new bits
	_CrtSetDbgFlag(tmp);
#endif

    mongo::Status status = mongo::client::initialize();
    if( !status.isOK() ) {
        LOGEVENT( "error", "init mongodb driver failed. code: " << status.code() << " reason: " << status.codeString() );
        return 0;
    }
    
    //load user id
    try {
        mongo::ScopedDbConnection conn( DB_HOST );
        if( !conn.ok() ) {
            LOGEVENT( "error", "connect to db failed" );
            return 0;
        }
        
        std::string errmsg;
        if( !conn->auth( DB_NAME, DB_USR, DB_PWD, errmsg ) ) {
            LOGEVENT( "error", "db auth failed" << errmsg );
            return 0;
        }

        mongo::BSONObj obj = conn->findOne( "Boids.Account", mongo::Query().sort("id", -1 ) );
        if( !obj.isEmpty() && obj.hasField( "id" ) ) {
            setNextUserId( obj.getField( "id" ).Long() + 1 );
        }
        else {
            LOGEVENT( "error", "load next user id failed. "; );
            return 0;
        }
        
        conn.done();
    }
    catch( mongo::AssertionException e ) {
        LOGEVENT( "error", "load next user id failed. info: " << e.toString() );
        return 0;
    }
    
	NETLIB_CHECK_VERSION;
    LogInitializeLocalOptions(true, true, "accountservice");

	thread.start();

	NetLib_Server_ptr server = NetLib_NewServer<AccountServerSession>(&thread);

	//可以不指定端口 TODO (主要是内部端口)
	if (!server->StartTCP(ACCOUNT_SERVER_PORT, 1, 120)) //端口，线程数，超时时间
	{
		LOGEVENTL("Error", "Server Start Failed !");

		NetLib_Servers_WaitForStop();

		LogUnInitialize();

		google_lalune::protobuf::ShutdownProtobufLibrary();

		return 0;
	}
	
	LOGEVENTL("Info", "Server Start Success. " << _ln("Port") << ACCOUNT_SERVER_PORT);

	AccountServerCommonLibDelegate* cl_delegate = new AccountServerCommonLibDelegate();
	InitializeCommonLib(thread, cl_delegate, SERVER_TYPE_ACCOUNT_SERVER, argc, argv);
	ServerStarted(ACCOUNT_SERVER_PORT);
	
	for (;;)
	{
		char tmp[200];
		if (scanf("%s", tmp) <= 0)
		{
			boost::this_thread::sleep(boost::posix_time::hours(1));
			continue;
		}

		if (strcmp(tmp, "stop") == 0)
		{
			if (server)
			{
				server->Stop();
				server.reset();
			}
		}
		else if (strcmp(tmp, "exit") == 0)
		{			
			break;
		}
	}

	NetLib_Servers_WaitForStop();

	LogUnInitialize();
	
	google_lalune::protobuf::ShutdownProtobufLibrary();
    mongo::ScopedDbConnection::clearPool();
    
    status = mongo::client::shutdown();
    if( !status.isOK() ) {
        LOGEVENT( "error", "shutdown mongodb driver failed. code: " << status.code() << " reason: " << status.codeString() );
        return 0;
    }

	return 0;
}


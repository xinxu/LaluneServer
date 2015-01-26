#include "AccountServerSessionDelegate.h"
#include "../ServerCommonLib/ServerCommon.h"
#include "../../../LaluneCommon/include/MessageTypeDef.h"
#include "req_resp.pb.h"
#include "../Log/Log.h"
#include "../include/utility1.h"
#include "AccountServerConfig.h"
#include "AccountServer.h"
#include <boost/random.hpp>

void AccountServerSession::RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
{
	if (SERVER_MSG_LENGTH(data) >= SERVER_MSG_HEADER_BASE_SIZE)
	{
		switch (SERVER_MSG_TYPE(data))
		{
		case MSG_TYPE_AUTOREGISTER_REQUEST:
		{
			uint32_t op_id = 0;
			lalune::AutoRegisterRequest auto_register;
			if (ParseMsgOpId(data, op_id, auto_register))
			{
                lalune::AutoRegisterResponce response;
                try {
                    mongo::ScopedDbConnection conn( DB_HOST );
                    do {
                        if( conn.ok() ) {
                            std::string errmsg;
                            if( !conn->auth( DB_NAME, DB_USR, DB_PWD, errmsg ) ) {
                                LOGEVENT( "error", "db auth failed" );
                                response.mutable_header()->set_code( BOID_ERROR_SERVER_FAILED );
                                response.mutable_header()->set_err_str( "server failed" );
                                break;
                            }
                            long long uid = getNextUserId();
                            char usr[64] = { 0 };
                            sprintf( usr, "guest%lld", uid );
                            char pwd[16] = { 0 };
                            boost::random::mt19937 rng;
                            boost::random::uniform_int_distribution<> num(100000,999999);
                            int x = num( rng );
                            sprintf( pwd, "%d", x );
                            std::string strusr = std::string( usr );
                            std::string strpwd = std::string( pwd );
                            
                            conn->insert( "Boids.Account", BSON( "id" << uid++ << "usr" << strusr  << "pwd" << strpwd << "date" << mongo::DATENOW ) );
                            errmsg = conn->getLastError( "Boid" );
                            if( errmsg.size() > 0 ) {
                                if( errmsg.find( "dup key" ) != std::string::npos ) {
                                    response.mutable_header()->set_code( BOID_ERROR_DUPLICATE_USERNAME );
                                    response.mutable_header()->set_err_str( "duplicate username" );
                                    LOGEVENT( "warn", "db insert failed. err: " << errmsg );
                                }
                                else {
                                    response.mutable_header()->set_code( BOID_ERROR_SERVER_FAILED );
                                    response.mutable_header()->set_err_str( "server failed" );
                                    LOGEVENT( "error", "db insert failed. err: " << errmsg );
                                }
                                break;
                            }
                            
                            response.mutable_header()->set_code( BOID_ERROR_NONE );
                            response.set_uid( uid );
                            response.set_usr( strusr );
                            response.set_pwd( strpwd );
                        }
                        else {
                            response.mutable_header()->set_code( BOID_ERROR_SERVER_FAILED );
                            response.mutable_header()->set_err_str( "server failed" );
                            break;
                        }
                    }while( 0 );
                    conn.done();
                }
                catch( mongo::DBException e ) {
                    response.mutable_header()->set_code( BOID_ERROR_SERVER_FAILED );
                    response.mutable_header()->set_err_str( "server failed" );
                    LOGEVENT( "error", "db insert failed. err: " << e.toString() );
                }
                ReplyMsgOpId(sessionptr, MSG_TYPE_AUTOREGISTER_RESPONSE, op_id, response);
			}
			break;
		}
	
		default:
			LOGEVENTL("WARN", "unrecognized message type. " << SERVER_MSG_TYPE(data));
			break;
		}
	}
	else
	{
		LOGEVENTL("WARN", "message length not enough: " << SERVER_MSG_LENGTH(data));
	}
}

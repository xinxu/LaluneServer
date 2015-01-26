//
//  MongodbService.cpp
//  MongodbService
//
//  Created by Yanjie Chen on 11/13/14.
//  Copyright (c) 2014 Shanghai Ue2game Network Technology Co. Ltd. All rights reserved.
//

#include "MongodbService.h"
#include "../../Log/Log.h"

#define MONGODB_ACCOUNT_DB_NAME     "Boids"
#define MONGODB_ACCOUNT_USER_NAME   "BoidsAdmin"
#define MONGODB_ACCOUNT_PWD         "ImBoidsFan"

MongodbService::MongodbService( const std::string dbhost, int dbport ) :
_dbhost( dbhost ),
_dbport( dbport ),
_dbconn( true ){
    
}

MongodbService::~MongodbService() {
    
}

bool MongodbService::init() {
    do {
        LogInitializeLocalOptions( true, true, "mongodb_service" );
        
        mongo::Status status = mongo::client::initialize();
        if( !status.isOK() ) {
            LOGEVENT( "error", "init mongodb driver failed. code: " << status.code() << " reason: " << status.codeString() );
            break;
        }
        
        if( !this->connect() ) {
            this->destroy();
            break;
        }
        
        return true;
    }while( false );
    
    return false;
}

bool MongodbService::destroy() {
    do {
        mongo::Status status = mongo::client::shutdown();
        if( !status.isOK() ) {
            LOGEVENT( "error", "shutdown mongodb driver failed. code: " << status.code() << " reason: " << status.codeString() );
            break;
        }
        LogUnInitialize();
        return true;
    }while( false );
    return false;
}

bool MongodbService::connect() {
    do {
        std::string str_err;
        if( !_dbconn.connect( mongo::HostAndPort( _dbhost, _dbport ), str_err ) ) {
            LOGEVENT( "err", "connect to db failed: " << str_err );
            break;
        }
        return true;
    }while( false );
    return false;
}

bool MongodbService::_auth( const std::string& dbname, const std::string& usr, const std::string &pwd ) {
    std::string errmsg;
    if( !_dbconn.auth( dbname, usr, pwd, errmsg ) )  {
        LOGEVENT( "error", "auth failed by mongodb. reason: " << errmsg );
        return false;
    }
    return true;
}

bool MongodbService::_logout( const std::string& dbname ) {
    mongo::BSONObj obj;
    _dbconn.logout( dbname, obj );
    return true;
}

int MongodbService::createNewUser( long long uid, const std::string& usr, const std::string& pwd ) {
    if( _dbconn.isFailed() ) {
        return NotConnected;
    }
    if( !this->_auth( MONGODB_ACCOUNT_DB_NAME, MONGODB_ACCOUNT_USER_NAME, MONGODB_ACCOUNT_PWD ) ) {
        return AutoFailed;
    }
    
    std::string err;
    
    _dbconn.insert( "Boids.Account", BSON( "$set" << BSON( "id" << uid << "pwd" << pwd << "date" << mongo::DATENOW ) ) );
    _dbconn.getLastError( MONGODB_ACCOUNT_DB_NAME );
    if( err.size() > 0 ) {
        LOGEVENT( "warn", "update user failed. info: " << err );
        return InsertNewUserFailed;
    }
    
    return NoErr;
}
//
//  MongodbService.h
//  MongodbService
//
//  Created by Yanjie Chen on 11/13/14.
//  Copyright (c) 2014 Shanghai Ue2game Network Technology Co. Ltd. All rights reserved.
//

#ifndef __MongodbService__MongodbService__
#define __MongodbService__MongodbService__

#include <string>
#include "../mongodb_driver/include/mongo/client/dbclient.h"

class MongodbService {
public:
    enum MongodbError {
        NoErr = 0,
        NotConnected,
        AutoFailed,
        InsertNewUserFailed,
    };
    
    MongodbService( const std::string dbhost, int dbport );
    ~MongodbService();
    
    bool init();
    bool destroy();
    
    bool connect();
    
    int createNewUser( long long uid, const std::string& usr, const std::string& pwd );
    
private:
    bool _auth( const std::string& dbname, const std::string& usr, const std::string &pwd );
    bool _logout( const std::string& dbname );
    
    mongo::DBClientConnection _dbconn;
    
    std::string _dbhost;
    int _dbport;
};

#endif /* defined(__MongodbService__MongodbService__) */

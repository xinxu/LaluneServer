//
//  PvpGameServer.h
//  BoidsPvpServer
//
//  Created by Yanjie Chen on 1/12/15.
//  Copyright (c) 2015 Shanghai Yuyu Network Technology Co. Ltd. All rights reserved.
//

#ifndef __BoidsPvpServer__PvpGameServer__
#define __BoidsPvpServer__PvpGameServer__

#include <list>
#include "PvpTerminal.h"
#include "boost/array.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/bind.hpp"

#define DEFAULT_FRAME_RATE 30.0

namespace boids {
    enum GameState {
        WAITING = 1,
        RUNNING = 2
    };
    
    class PvpServer;
    
    class PvpGameServer : public boost::enable_shared_from_this<PvpGameServer> {
    public:
        PvpGameServer( boost::asio::io_service& io_service, const boost::shared_ptr<PvpServer>& pvp_server );
        ~PvpGameServer();
        
        inline std::string getGameId() { return _game_id; }
        void setGameId( const std::string& game_id );
        
        void start();
        void stop();
        void triggerUpdate( const boost::system::error_code& error );
        void update( int millisec );
    
        void handleMessage( PvpMessagePtr message, PvpTerminalPtr terminal );
        
        void addTerminal( PvpTerminalPtr terminal );
        void deleteTerminal( PvpTerminalPtr terminal );
        
        bool containsTerminal( PvpTerminalPtr terminal );
        
        bool containsUser( const std::string& user_id );
        
        void setGameInitData( const boids::GameInitData& init_data );
        
    private:
        boids::GameInitData _game_init_data;
        UserOperationPackagePtr _wrapped_operations;
        
        GameState _state;
        
        std::string _game_id;
        
        boost::weak_ptr<PvpServer> _pvp_server;
        
        std::list<PvpTerminalPtr> _terminals;
        
        boost::asio::deadline_timer _timer;
        
        float _frame_rate;
        
        int _game_time;
    };
    
    typedef boost::shared_ptr<PvpGameServer> PvpGameServerPtr;
}

#endif /* defined(__BoidsPvpServer__PvpGameServer__) */

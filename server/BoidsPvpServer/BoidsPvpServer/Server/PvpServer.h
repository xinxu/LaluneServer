//
//  PvpServer.h
//  BoidsPvpServer
//
//  Created by Yanjie Chen on 1/12/15.
//  Copyright (c) 2015 Shanghai Yuyu Network Technology Co. Ltd. All rights reserved.
//

#ifndef __BoidsPvpServer__PvpServer__
#define __BoidsPvpServer__PvpServer__

#include "PvpGameServer.h"
#include <unordered_map>

#define GAME_PORT 20437
#define CTRL_PORT 20438

namespace boids {

    typedef boost::array<char, 1024> MessageBuffer;
    
    class PvpServerContext {
    public:
        PvpServerContext();
        ~PvpServerContext();
        
        unsigned short game_port;
        unsigned short ctrl_port;
    };
    
    typedef boost::shared_ptr<PvpServerContext> PvpServerContextPtr;
    
    class PvpServer : public boost::enable_shared_from_this<PvpServer> {
    public:
        PvpServer( PvpServerContextPtr context );
        ~PvpServer();
        
        bool start();
        void stop();
        
        void sendBoidsMessage( BoidsMessagePtr message );
        void sendBoidsMessages( MessageQueue<BoidsMessagePtr> message_queue );
        
        void receive();
        
        void sendHandler( const boost::system::error_code& error, std::size_t size );
        void receiveHandler( const boost::system::error_code& error, std::size_t size );
        
        void dispatchMessage( const boost::asio::ip::udp::endpoint& endpoint, const MessageBuffer& buffer, std::size_t size );
        
        bool addGame( int game_id );
        void deleteGame( int game_id );
        
        void addTerminal( const std::string& key, PvpTerminalPtr terminal );
    
        //temp use
        PvpGameServerPtr getTestGameServer() { return _test_game_server; }
    private:
        void sendto( const boost::asio::ip::udp::endpoint& endpoint, boost::shared_ptr<PvpMessage> msg );
        void sendBoidsMessageInQueue();
        
        std::string endpointToString( const boost::asio::ip::udp::endpoint& endpoint );
        
        PvpServerContextPtr _context;
        
        boost::asio::io_service _io_service;
        boost::asio::ip::udp::socket _game_sock;    //receive and send game messages
        boost::asio::ip::udp::endpoint _remote_endpoint;
        MessageBuffer _message_buffer;
        
        boost::asio::ip::tcp::socket _ctrl_sock;     //receive and send control messages such as "add game"
        
        std::unordered_map<int, PvpGameServerPtr> _games;
        std::unordered_map<std::string, PvpTerminalPtr> _terminals;
        
        MessageQueue<BoidsMessagePtr> _tosend_queue;
        
        bool _is_sending;
        //temp use
        PvpGameServerPtr _test_game_server;
    };
    
    typedef boost::shared_ptr<PvpServer> PvpServerPtr;
}

#endif /* defined(__BoidsPvpServer__PvpServer__) */

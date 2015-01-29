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
#include "Config.h"
#include "../Data/boids.pb.h"
#include "../Data/Battle.pb.h"
#include <unordered_map>
#include <array>

namespace boids {

    typedef boost::array<char, 1024> MessageBuffer;
    typedef boost::shared_ptr<char> MessageDataPtr;
    
    class PvpServerContext {
    public:
        PvpServerContext();
        ~PvpServerContext();
        
        unsigned short game_port;
        std::string ctrl_server_ip;
        unsigned short ctrl_server_port;
    };
    
    typedef boost::shared_ptr<PvpServerContext> PvpServerContextPtr;
    
    class PvpServer : public boost::enable_shared_from_this<PvpServer> {
    public:
        PvpServer( PvpServerContextPtr context );
        ~PvpServer();
        
        bool init();
        
        bool start();
        void stop();
        
        //control service
        void connect();
        void reconnect();
        void sendWithoutSize( const std::string& message );
        void send( const std::string& message );
        void receive();
        
        void connectHandler( const boost::system::error_code& error );
        void sendHandler( const boost::system::error_code& error, std::size_t size );
        void receiveHandler( const boost::system::error_code& error, std::size_t size );
        
        void reconnectTrigger( const boost::system::error_code& error );
        
        void parseControlMessage( const std::string& data );
        
        void sendHeartbeat();
        void heartBeatTrigger( const boost::system::error_code& error );
        
        //game service
        void sendBoidsMessage( BoidsMessagePtr message );
        void sendBoidsMessages( MessageQueue<BoidsMessagePtr> message_queue );
        
        void receivefrom();
        
        void sendtoHandler( const boost::system::error_code& error, std::size_t size );
        void receivefromHandler( const boost::system::error_code& error, std::size_t size );
        
        void dispatchMessage( const boost::asio::ip::udp::endpoint& endpoint, const MessageBuffer& buffer, std::size_t size );
        
        PvpGameServerPtr addGame( const std::string& game_id );
        void deleteGame( const std::string& game_id );
        PvpGameServerPtr findGameServer( const std::string& game_id );
        PvpGameServerPtr findGameServerByUserId( const std::string& user_id );
        
        void addTerminal( const std::string& key, PvpTerminalPtr terminal );
    private:
        void sendto( const boost::asio::ip::udp::endpoint& endpoint, boost::shared_ptr<PvpMessage> msg );
        void sendBoidsMessageInQueue();
        
        std::string endpointToString( const boost::asio::ip::udp::endpoint& endpoint );
        
        void handleCreateGameRequest( const std::string& data );
        void handleRegisterResponse( const std::string& data );
        
        void registerToControlServer();
        
    private:
        PvpServerContextPtr _context;
        
        boost::asio::io_service _io_service;
        boost::asio::ip::udp::socket _game_sock;    //receive and send game messages
        boost::asio::ip::udp::endpoint _remote_endpoint;
        MessageBuffer _message_buffer;
        
        //heart beat with control server( match server )
        std::string _heart_beat_data;
        boost::posix_time::seconds _heart_beat_interval;
        boost::asio::deadline_timer _heart_beat_timer;
        
        boost::posix_time::microseconds _reconnect_interval;
        boost::asio::deadline_timer _reconnect_timer;
        boost::asio::ip::tcp::socket _ctrl_sock;     //receive and send control messages such as "add game"
        
        std::unordered_map<std::string, PvpGameServerPtr> _games;
        std::unordered_map<std::string, PvpTerminalPtr> _terminals;
        
        MessageQueue<BoidsMessagePtr> _tosend_queue;
        
        bool _is_sending;
        
        bool _is_reading_size;
        std::array<char, 4> _size_bytes;
        std::vector<char> _data_bytes;
    };
    
    typedef boost::shared_ptr<PvpServer> PvpServerPtr;
}

#endif /* defined(__BoidsPvpServer__PvpServer__) */

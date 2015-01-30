//
//  PvpServer.cpp
//  BoidsPvpServer
//
//  Created by Yanjie Chen on 1/12/15.
//  Copyright (c) 2015 Shanghai Yuyu Network Technology Co. Ltd. All rights reserved.
//

#include "PvpServer.h"
#include <iostream>

using namespace boids;

PvpServerContext::PvpServerContext() :
game_port( GAME_SERVICE_PORT ),
ctrl_server_ip( CTRL_SERVER_IP ),
ctrl_server_port( CTRL_SERVER_PORT )
{
}

PvpServerContext::~PvpServerContext() {

}

PvpServer::PvpServer( PvpServerContextPtr context ) :
_context( context ),
_game_sock( _io_service, boost::asio::ip::udp::endpoint( boost::asio::ip::udp::v4(), context->game_port ) ),
_ctrl_sock( _io_service ),
_reconnect_timer( _io_service ),
_reconnect_interval( 5000000.0 ),
_heart_beat_interval( 60.0 ),
_heart_beat_timer( _io_service )
{
}

PvpServer::~PvpServer() {
    
}

bool PvpServer::init() {
    this->connect();
    _io_service.run();
    return true;
}

bool PvpServer::start() {
    std::cout << "server start" << std::endl;
    
    //init heart beat message
    std::string heart_beat_data;
    PvPServerHeartBeat heart_beat;
    heart_beat.set_ip( GAME_SERVICE_IP );
    heart_beat.set_port( GAME_SERVICE_PORT );
    heart_beat.SerializeToString( &heart_beat_data );
    BoidsMessageHeader header;
    header.set_type( PVP_SERVER_HEART_BEAT );
    header.set_data( heart_beat_data.c_str(), heart_beat_data.size() );
    header.SerializeToString( &_heart_beat_data );
    
    this->registerToControlServer();
    this->sendHeartbeat();
    
    _is_reading_size = true;
    _is_sending = false;
    
    this->receivefrom();
    this->receive();
    return true;
}

void PvpServer::stop() {
    
}

//control service
void PvpServer::connect() {
    std::cout << "connecting to control server..." << std::endl;
    _ctrl_sock.close();
    boost::asio::ip::address addr = boost::asio::ip::address::from_string( _context->ctrl_server_ip );
    boost::asio::ip::tcp::endpoint ctrl_server( addr, _context->ctrl_server_port );
    _ctrl_sock.async_connect( ctrl_server, boost::bind( &PvpServer::connectHandler , this, boost::asio::placeholders::error ) );
}

void PvpServer::reconnect() {
    std::cout << "reconnecting to control server..." << std::endl;
    _reconnect_timer.expires_from_now( _reconnect_interval );
    _reconnect_timer.async_wait( boost::bind( &PvpServer::reconnectTrigger, this, boost::asio::placeholders::error ) );
}

void PvpServer::sendWithoutSize( const std::string& message ) {
    int size = (int)message.size() + 4;
    char* num = (char*)&size;
    std::string send_string = std::string( num, 4 );
    send_string.append( message.c_str(), size - 4 );
//    std::cout << std::hex << send_string.size() << " " << std::hex << size << std::endl;
//    std::cout << "send message:" << std::endl;
//    for( int i = 0; i < send_string.size(); i++ ) {
//        std::cout << std::hex << (int)send_string[i] << " ";
//    }
//    std::cout << std::endl;
    this->send( send_string );
}

void PvpServer::send( const std::string& message ) {
    _ctrl_sock.async_send( boost::asio::buffer( message.c_str(), message.size() ), boost::bind( &PvpServer::sendHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred ) );
}

void PvpServer::receive() {
    if( _is_reading_size ) {
        _ctrl_sock.async_receive( boost::asio::buffer( _size_bytes ), boost::bind( &PvpServer::receiveHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred ) );
    }
    else {
        _ctrl_sock.async_receive( boost::asio::buffer( _data_bytes, _data_bytes.size() ), boost::bind( &PvpServer::receiveHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred ) );
    }
}

void PvpServer::connectHandler( const boost::system::error_code& error ) {
    if( !error ) {
        std::cout << "connected to control server!" << std::endl;
        this->start();
    }
    else {
        std::cout << "connect to control server failed: " << error << std::endl;
        this->reconnect();
    }
}

void PvpServer::sendHandler( const boost::system::error_code& error, std::size_t size ) {
    if( error ) {
        std::cout << "send message error:" << error << std::endl;
    }
}

void PvpServer::receiveHandler( const boost::system::error_code& error, std::size_t size ) {
    if( !error ) {
        if( _is_reading_size ) {
            int size = *(int*)&_size_bytes[0] - 4;
            _data_bytes.resize( size );
        }
        else {
            std::string data_string;
            data_string.assign( _data_bytes.begin(), _data_bytes.end() );
            this->parseControlMessage( data_string );
        }
        _is_reading_size = !_is_reading_size;
        this->receive();
    }
    else {
        std::cout << "receive from ctrl server failed.. error: " << error << std::endl;
        this->reconnect();
    }
}

void PvpServer::sendHeartbeat() {
    this->sendWithoutSize( _heart_beat_data );
    _heart_beat_timer.expires_from_now( _heart_beat_interval );
    _heart_beat_timer.async_wait( boost::bind( &PvpServer::heartBeatTrigger, this, boost::asio::placeholders::error ) );
}

void PvpServer::reconnectTrigger( const boost::system::error_code& error ) {
    if( !error ) {
        this->connect();
    }
}

void PvpServer::parseControlMessage( const std::string& data ) {
    boids::BoidsMessageHeader message;
    message.ParseFromString( data );
    switch( message.type() ) {
        case PVP_SERVER_CREATE_GAME_REQUEST:
            this->handleCreateGameRequest( message.data() );
            break;
        case PVP_SERVER_REGISTER_RESPONSE:
            this->handleRegisterResponse( message.data() );
            break;
        default:
            break;
    }
}

void PvpServer::handleCreateGameRequest( const std::string& data ) {
    boids::CreateGame request;
    request.ParseFromString( data );
    
    PvpGameServerPtr server = this->addGame( request.game_id() );
    if( server != nullptr ) {
        server->setGameId( request.game_id() );
        server->setGameInitData( request.game_init_data() );
    }
    int ret_value = server == nullptr ? 1 : 0;
    //send response back
    boids::CreateGameResponse response;
    response.set_game_id( request.game_id() );
    response.set_ret_value( ret_value );
    std::string resp_string;
    response.SerializeToString( &resp_string );
    boids::BoidsMessageHeader header;
    std::string header_string;
    header.set_type( PVP_SERVER_CREATE_GAME_RESPONSE );
    header.set_data( resp_string.c_str(), resp_string.size() );
    header.SerializeToString( &header_string );
    this->sendWithoutSize( header_string );
}

void PvpServer::handleRegisterResponse( const std::string& data ) {
    boids::PvPServerRegisterResponse response;
    response.ParseFromString( data );
    if( response.ret_value() != 0 ) {
        std::cout << "register to control server failed!" << std::endl;
    }
}

void PvpServer::registerToControlServer() {
    boids::BoidsMessageHeader header;
    header.set_type( boids::PVP_SERVER_REGISTER_REQUEST );
    boids::PvPServerRegister request;
    request.set_ip( GAME_SERVICE_IP );
    request.set_port( GAME_SERVICE_PORT );
    std::string request_string;
    request.SerializeToString( &request_string );
    header.set_data( request_string.c_str(), request_string.size() );
    std::string data_string;
    header.SerializeToString( &data_string );
    this->sendWithoutSize( data_string );
}

void PvpServer::heartBeatTrigger( const boost::system::error_code& error ) {
    if( !error ) {
        this->sendHeartbeat();
    }
}

//game services

void PvpServer::sendBoidsMessage( BoidsMessagePtr message ) {
//    _tosend_queue.merge( message );
//    if( !_is_sending ) {
//        _is_sending = true;
//        this->sendBoidsMessageInQueue();
//    }
    this->sendto( message->endpoint, message->message );
}

void PvpServer::sendBoidsMessages( MessageQueue<BoidsMessagePtr> message_queue ) {
    _tosend_queue.merge( message_queue );
    if( !_is_sending ) {
        _is_sending = true;
        this->sendBoidsMessageInQueue();
    }
}

void PvpServer::sendto( const boost::asio::ip::udp::endpoint& endpoint, PvpMessagePtr msg ) {
    std::string message_string;
    msg->SerializeToString( &message_string );
//    _game_sock.async_send_to( boost::asio::buffer( message_string ), endpoint, boost::bind( &PvpServer::sendtoHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred ) );
    _game_sock.async_send_to( boost::asio::buffer( message_string.c_str(), message_string.size() ), endpoint, boost::bind( &PvpServer::sendtoHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred ) );
}

void PvpServer::sendBoidsMessageInQueue() {
    if( !_tosend_queue.isEmpty() ) {
        BoidsMessagePtr boids_message = _tosend_queue.dequeue();
        this->sendto( boids_message->endpoint, boids_message->message );
    }
    else {
        _is_sending = false;
    }
}

void PvpServer::receivefrom() {
    _game_sock.async_receive_from( boost::asio::buffer( _message_buffer ), _remote_endpoint, boost::bind( &PvpServer::receivefromHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred ) );
}

void PvpServer::sendtoHandler( const boost::system::error_code& error, std::size_t size ) {
    if( error != 0 ) {
        std::cout << "send message error: " << error << std::endl;
    }
}

void PvpServer::receivefromHandler( const boost::system::error_code& error, std::size_t size ) {
    if( !error || error == boost::asio::error::message_size ) {
        this->dispatchMessage( _remote_endpoint, _message_buffer, size );
    }
    else {
        std::cout << "receive error" << error << std::endl;
    }
    this->receivefrom();
}

void PvpServer::dispatchMessage( const boost::asio::ip::udp::endpoint& endpoint, const MessageBuffer& buffer, std::size_t size ) {
    //parse message
    PvpMessagePtr message = PvpMessagePtr( new PvpMessage() );
    message->ParseFromArray( &buffer, (int)size );
    
    std::string key = this->endpointToString( endpoint );
    auto itr = _terminals.find( key );
    if( itr != _terminals.end() ) {
        itr->second->receiveMessage( message );
    }
    else {
        //new endpoint
        PvpTerminalPtr term = PvpTerminalPtr( new PvpTerminal( _io_service, shared_from_this(), endpoint ) );
        this->addTerminal( key, term );
        term->receiveMessage( message );
        std::cout << "new terminal:" << this->endpointToString( endpoint ) << std::endl;
    }
}

PvpGameServerPtr PvpServer::addGame( const std::string& game_id ) {
    if( _games.find( game_id ) == _games.end() ) {
        PvpGameServerPtr game_server = PvpGameServerPtr( new PvpGameServer( _io_service, shared_from_this() ) );
        _games.insert( std::make_pair( game_id, game_server ) );
        return game_server;
    }
    
    return nullptr;
}

void PvpServer::deleteGame( const std::string& game_id ) {
    auto itr = _games.find( game_id );
    if( itr != _games.end() ) {
        _games.erase( itr );
    }
}

PvpGameServerPtr PvpServer::findGameServer( const std::string& game_id ) {
    auto itr = _games.find( game_id );
    if( itr != _games.end() ) {
        return itr->second;
    }
    return nullptr;
}

PvpGameServerPtr PvpServer::findGameServerByUserId( const std::string& user_id ) {
    for( auto itr = _games.begin(); itr != _games.end(); ++itr ) {
        PvpGameServerPtr game_server = itr->second;
        if( game_server->containsUser( user_id ) ) {
            return game_server;
        }
    }
    return nullptr;
}

void PvpServer::addTerminal( const std::string& key, PvpTerminalPtr terminal ) {
    if( _terminals.find( key ) == _terminals.end() ) {
        _terminals.insert( std::make_pair( key, terminal ) );
    }
}

void PvpServer::deleteTerminal( PvpTerminalPtr terminal ) {
    for( auto itr = _terminals.begin(); itr != _terminals.end(); ++itr ) {
        if( itr->second == terminal ) {
            _terminals.erase( itr );
            break;
        }
    }
}

std::string PvpServer::endpointToString( const boost::asio::ip::udp::endpoint& endpoint ) {
    std::stringstream ss;
    ss << endpoint.address().to_string() << ";" << endpoint.port() << ";" << endpoint.protocol().family() << ";" << endpoint.protocol().protocol() << ";" << endpoint.protocol().type();
    std::cout << "endpoint str:" << ss.str() << std::endl;
    return ss.str();
}

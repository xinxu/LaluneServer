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
game_port( GAME_PORT ),
ctrl_port( CTRL_PORT )
{
}

PvpServerContext::~PvpServerContext() {

}

PvpServer::PvpServer( PvpServerContextPtr context ) :
_context( context ),
_game_sock( _io_service, boost::asio::ip::udp::endpoint( boost::asio::ip::udp::v4(), context->game_port ) ),
_ctrl_sock( _io_service, boost::asio::ip::tcp::endpoint( boost::asio::ip::tcp::v4(), context->ctrl_port ) )

{
}

PvpServer::~PvpServer() {
    
}

bool PvpServer::start() {
    std::cout << "server start" << std::endl;
    //temp use
    _test_game_server = PvpGameServerPtr( new PvpGameServer( _io_service, shared_from_this() ) );
    
    _is_sending = false;
    this->receive();
    _io_service.run();
    return true;
}

void PvpServer::stop() {
    
}

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
    _game_sock.async_send_to( boost::asio::buffer( message_string.c_str(), message_string.size() ), endpoint, boost::bind( &PvpServer::sendHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred ) );
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

void PvpServer::receive() {
    _game_sock.async_receive_from( boost::asio::buffer( _message_buffer ), _remote_endpoint, boost::bind( &PvpServer::receiveHandler, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred ) );
}

void PvpServer::sendHandler( const boost::system::error_code& error, std::size_t size ) {
    if( error != 0 ) {
        std::cout << "send message error: " << error << std::endl;
    }
}

void PvpServer::receiveHandler( const boost::system::error_code& error, std::size_t size ) {
    if( !error || error == boost::asio::error::message_size ) {
        this->dispatchMessage( _remote_endpoint, _message_buffer, size );
    }
    else {
        std::cout << "recive error" << error << std::endl;
    }
    this->receive();
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

bool PvpServer::addGame( int game_id ) {
    return true;
}

void PvpServer::deleteGame( int game_id ) {
    
}

void PvpServer::addTerminal( const std::string& key, PvpTerminalPtr terminal ) {
    if( _terminals.find( key ) == _terminals.end() ) {
        _terminals.insert( std::make_pair( key, terminal ) );
    }
}

std::string PvpServer::endpointToString( const boost::asio::ip::udp::endpoint& endpoint ) {
    std::stringstream ss;
    ss << endpoint.address().to_string() << ";" << endpoint.port() << ";" << endpoint.protocol().family() << ";" << endpoint.protocol().protocol() << ";" << endpoint.protocol().type();
    std::cout << "endpoint str:" << ss.str() << std::endl;
    return ss.str();
}

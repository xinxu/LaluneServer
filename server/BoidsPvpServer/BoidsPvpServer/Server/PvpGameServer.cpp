//
//  PvpGameServer.cpp
//  BoidsPvpServer
//
//  Created by Yanjie Chen on 1/12/15.
//  Copyright (c) 2015 Shanghai Yuyu Network Technology Co. Ltd. All rights reserved.
//

#include "PvpGameServer.h"
#include <iostream>

using namespace  boids;

GameInitData init_data;

void init_force_data() {
    boids::ForceData* force1 = init_data.add_forces();
    force1->set_user_id( "junge" );
    force1->set_force_id( 1 );
    boids::UnitData* u11 = force1->add_units();
    u11->set_unit_name( "panda" );
    u11->set_unit_level( 1 );
    boids::UnitData* u12 = force1->add_units();
    u12->set_unit_name( "zeus" );
    u12->set_unit_level( 1 );
    boids::UnitData* u13 = force1->add_units();
    u13->set_unit_name( "gandalf" );
    u13->set_unit_level( 1 );
    
    boids::ForceData* force2 = init_data.add_forces();
    force2->set_user_id( "tassar" );
    force2->set_force_id( 2 );
    boids::UnitData* u21 = force2->add_units();
    u21->set_unit_name( "saber" );
    u21->set_unit_level( 1 );
    boids::UnitData* u22 = force2->add_units();
    u22->set_unit_name( "dracula" );
    u22->set_unit_level( 1 );
    boids::UnitData* u23 = force2->add_units();
    u23->set_unit_name( "vanhelsing" );
    u23->set_unit_level( 1 );
}

PvpGameServer::PvpGameServer( boost::asio::io_service& io_service, const boost::shared_ptr<PvpServer>& pvp_server ) :
_pvp_server( pvp_server ),
_frame_rate( DEFAULT_FRAME_RATE ),
_timer( io_service ),
_state( GameState::WAITING ) {
    _wrapped_operations = UserOperationPackagePtr( new UserOperationPackage() );
    init_force_data();
}

PvpGameServer::~PvpGameServer() {
}

void PvpGameServer::start() {
    _state = GameState::RUNNING;
    _game_time = 0;
    _timer.expires_from_now( boost::posix_time::microseconds( 1000000.0 / _frame_rate ) );
    _timer.async_wait( boost::bind( &PvpGameServer::triggerUpdate, this, boost::asio::placeholders::error ) );
}

void PvpGameServer::stop() {
    _timer.cancel();
    _state = GameState::WAITING;
}

void PvpGameServer::triggerUpdate( const boost::system::error_code& error ) {
    if( !error ) {
        this->start();
        this->update( int( 1000.0 / _frame_rate ) );
    }
}

void PvpGameServer::update( int millisec ) {
    _game_time += millisec;
    
    std::string operations_string;
    GameMessage game_message;
    game_message.set_type( boids::GameMessage_MessageType_UserOperationPackage );
    game_message.mutable_user_op_package()->CopyFrom( *_wrapped_operations );
    game_message.SerializeToString( &operations_string );
    
    for( auto term : _terminals ) {
        PvpMessagePtr wrapped_message = PvpMessagePtr( new PvpMessage() );
        wrapped_message->set_data( operations_string );
        term->sendMessage( wrapped_message );
    }
    _wrapped_operations->clear_operations();
}

void PvpGameServer::handleMessage( PvpMessagePtr message, PvpTerminalPtr terminal ) {
    std::cout << "received seq no:" << message->seq_no() << " message: " << message->data() << std::endl;
    GameMessage game_message;
    if( game_message.ParseFromString( message->data() ) ) {
        if( game_message.type() == GameMessage_MessageType_UserOperation ) {
            UserOperation user_op = game_message.user_op();
            switch( user_op.op_type() ) {
                case UserOperation_OperationType_EnterGame:
                {
                    if( _state == GameState::WAITING ) {
                        std::string data_string;
                        GameMessage game_message;
                        game_message.set_type( GameMessage_MessageType_GameInitData );
                        game_message.mutable_game_init_data()->CopyFrom( init_data );
                        game_message.SerializeToString( &data_string );
                        
                        PvpMessagePtr wrapped_message = PvpMessagePtr( new PvpMessage() );
                        wrapped_message->set_data( data_string );
                        
                        terminal->sendMessage( wrapped_message );
                        
                        if( _terminals.size() == 2 ) {
                            this->start();
                        }
                        break;
                    }
                }
                case UserOperation_OperationType_QuitGame:
                    
                    break;
                default:
                    user_op.set_timestamp( _game_time );
                    _wrapped_operations->add_operations()->CopyFrom( user_op );
                    break;
            }
        }
    }
}

void PvpGameServer::addTerminal( PvpTerminalPtr terminal ) {
    if( !this->containsTerminal( terminal ) ) {
        _terminals.push_back( terminal );
    }
}

void PvpGameServer::deleteTerminal( PvpTerminalPtr terminal ) {
    for( auto itr = _terminals.begin(); itr != _terminals.end(); ++itr ) {
        if( (*itr) == terminal ) {
            _terminals.erase( itr );
            return;
        }
    }
}

bool PvpGameServer::containsTerminal( PvpTerminalPtr terminal ) {
    for( auto itr = _terminals.begin(); itr != _terminals.end(); ++itr ) {
        if( (*itr) == terminal ) {
            return true;
        }
    }
    return false;
}
//
//  PvpTerminal.cpp
//  BoidsPvpServer
//
//  Created by Yanjie Chen on 1/12/15.
//  Copyright (c) 2015 Shanghai Yuyu Network Technology Co. Ltd. All rights reserved.
//

#include "PvpTerminal.h"
#include "../Server/PvpServer.h"

using namespace boids;

#define PROTO_VERSION 1

#define HEADER_SIZE 12

#define SEQ_NO_BITS 16

#define DEFAULT_RESEND_INTERVAL 200000 //mirco sec

PvpTerminal::PvpTerminal( boost::asio::io_service& io_service, boost::shared_ptr<PvpServer> server, const boost::asio::ip::udp::endpoint& ep ) :
_server( server ),
_endpoint( ep ),
_timer( io_service ),
_resend_interval( DEFAULT_RESEND_INTERVAL ),
_state( PvpTerminalState::Disconnected )
{
    _max_resent_bit = 32;
    _seq_no = 0;
    _remote_ack_bits = 0xffffffff;
    
    _ack_no = 0;
    _ack_bits = 0;
    this->startTimer( _resend_interval );
}

PvpTerminal::~PvpTerminal() {
    
}

void PvpTerminal::enterGame( boost::shared_ptr<PvpGameServer> game ) {
    _game_server = game;
    _game_server.lock()->addTerminal( shared_from_this() );
}

void PvpTerminal::quitGame() {
    _game_server.lock()->deleteTerminal( shared_from_this() );
    _game_server.reset();
}

bool PvpTerminal::receiveMessage( PvpMessagePtr message ) {
   do {
      unsigned short proto_id = message->proto_id();
      unsigned short ack_no = (unsigned short)message->ack_no();
      unsigned short seq_no = (unsigned short)message->seq_no();
      unsigned int ack_bits = message->ack_bits();
      
      if( proto_id == SYNC_PROTO_ID ) {
         //connect request
         if( _state != PvpTerminalState::Disconnected ) {
            break;
         }
         _ack_no = seq_no;
         _ack_bits = 0x1;
         _remote_ack_bits = 0xffffffff;
         _max_resent_bit = 32;
         _sent_message.clear();
         
         PvpMessagePtr resp_message = PvpMessagePtr( new PvpMessage() );
         _state = PvpTerminalState::Sync;
         this->sendMessage( resp_message, SYNC_PROTO_ID );
      }
      else {
         if( _state == PvpTerminalState::Sync ) {
            _state = PvpTerminalState::Connected;
         }
         if( _state == PvpTerminalState::Connected ) {
            int shift = this->diffBetweenSequenceNumbers( seq_no, _ack_no );
            //update ack info
            if( shift == 0 ) {
               break;  //duplicated packet
            }
            else if( shift < 0 ) { //received ealier packet
               unsigned int bit = 0x1 << (-shift);
               if( ( _ack_bits & bit ) == 0 ) {
                  _ack_bits |= bit;
               }
               else {
                  break;  //duplicated packet
               }
            }
            else {  //latest packet
               _ack_no = seq_no;
               _ack_bits = ( _ack_bits << shift ) | 0x1;
            }
            
            //udpate remote_ack_bits
            int diff = this->diffBetweenSequenceNumbers( _seq_no, ack_no );
            if( diff >= 0 ) {
               for( int i = 0; i < 32 - diff; i++ ) {
                  unsigned int in_local_ack_bit = 1 << ( i + diff );
                  unsigned int in_msg_ack_bit = 1 << i;
                  if( ( _remote_ack_bits & in_local_ack_bit ) == 0 && ( ack_bits & in_msg_ack_bit ) != 0 ) {
                     _remote_ack_bits |= in_local_ack_bit;
                     this->dropMessage( this->prevSeqNo( ack_no, i ) );
                  }
               }
            }
            
            this->parseMessage( message );
         }
      }
   }while( 0 );
   
   return true;
}

void PvpTerminal::parseMessage( PvpMessagePtr message ) {
   if( _game_server.lock() == nullptr ) {
      //temp use
      this->enterGame( _server.lock()->getTestGameServer() );
   }
     //accept message
   PvpGameServerPtr game_server = _game_server.lock();
   if( game_server != nullptr ) {
      game_server->handleMessage( message, shared_from_this() );
   }
}

bool PvpTerminal::sendMessage( PvpMessagePtr message, unsigned short proto_id ) {
   if( ( _remote_ack_bits & 0x80000000 ) == 0 )
      return false;
   
   _seq_no = this->nextSeqNo( _seq_no );
   _remote_ack_bits <<= 1;
   
   message->set_version( PROTO_VERSION );
   message->set_proto_id( proto_id );
   message->set_ack_no( _ack_no );
   message->set_ack_bits( _ack_bits );
   message->set_seq_no( _seq_no );

   std::cout << "send message: " << message->seq_no() << std::endl;
   
   if( !this->directSendMessage( message ) ) {
      return false;
   }
   _sent_message.enqueue( message );
   
    return true;
}

bool PvpTerminal::sendMessages( std::list<PvpMessagePtr> message_list ) {
    //todo
    return true;
}

bool PvpTerminal::directSendMessage( PvpMessagePtr message ) {
   if( _state == PvpTerminalState::Disconnected ) {
      return false;
   }
   if( _state == PvpTerminalState::Sync && message->proto_id() != SYNC_PROTO_ID ) {
      return false;
   }

   PvpServerPtr server = _server.lock();
   BoidsMessagePtr boids_message = BoidsMessagePtr( new BoidsMessage( message, _endpoint ) );
   server->sendBoidsMessage( boids_message );
   if( _max_resent_bit > 0 ) {
      _max_resent_bit--;
   }
   return true;
}

void PvpTerminal::resendMessages() {
   if( _remote_ack_bits != 0xffffffff ) {
      unsigned int bit_mask = 0x80000000;
      for( int i = 1; i <= _max_resent_bit; i++ ) {
         if( ( _remote_ack_bits & bit_mask ) == 0 ) {
            //packet is thought to be lost, resend it
            unsigned short no =  this->prevSeqNo( _seq_no, 32 - i );
            PvpMessagePtr message = this->getSentMessage( no );
            if( message ) {
               std::cout << "resend message:" << message->seq_no() << std::endl;
               message->set_ack_no( _ack_no );
               message->set_ack_bits( _ack_bits );
               this->directSendMessage( message );
            }
            break;
         }
         bit_mask >>= 1;
      }
   }
   _max_resent_bit = 32;
}

void PvpTerminal::startTimer( long interval ) {
    _timer.expires_from_now( boost::posix_time::microseconds( interval ) );
    _timer.async_wait( boost::bind( &PvpTerminal::triggerUpdate, this, boost::asio::placeholders::error ) );
}

void PvpTerminal::triggerUpdate( const boost::system::error_code& error ) {
   if( !error ) {
      this->startTimer( _resend_interval );
      this->resendMessages();
   }
}

//private methods

PvpMessagePtr PvpTerminal::getSentMessage( unsigned short seq_no ) {
   std::list<PvpMessagePtr>* message_list = _sent_message.getMutableMessages();
   for( auto itr = message_list->begin(); itr != message_list->end(); ++itr ) {
      unsigned short m_seq_no = (unsigned short)(*itr)->seq_no();
      if( m_seq_no == seq_no ) {
         return *itr;
      }
   }
   return nullptr;
}

void PvpTerminal::dropMessage( unsigned short seq_no ) {
   std::list<PvpMessagePtr>* message_list = _sent_message.getMutableMessages();
   for( auto itr = message_list->begin(); itr != message_list->end(); ++itr ) {
      unsigned short m_seq_no = (unsigned short)(*itr)->seq_no();
      if( m_seq_no == seq_no ) {
         (*itr).reset();
         message_list->erase( itr );
         return;
      }
   }
}

int PvpTerminal::diffBetweenSequenceNumbers( unsigned short seq_one, unsigned seq_two ) {
    int diff = seq_one - seq_two;
    if( diff < -0x8000 ) {
        diff += 0x10000;
    }
    else if( diff > 0x8000 ) {
        diff -= 0x10000;
    }
    return diff;
}

unsigned short PvpTerminal::nextSeqNo( unsigned short seq_no ) {
    return seq_no == 0xffff ? 1 : seq_no + 1;
}

unsigned short PvpTerminal::prevSeqNo( unsigned short seq_no, unsigned short bias ) {
    return seq_no > bias ? seq_no - bias : seq_no + 0xffff - bias;
}
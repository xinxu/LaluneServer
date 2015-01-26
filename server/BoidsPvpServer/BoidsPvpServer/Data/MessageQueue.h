//
//  MessageQueue.h
//  BoidsPvpServer
//
//  Created by Yanjie Chen on 1/12/15.
//  Copyright (c) 2015 Shanghai Yuyu Network Technology Co. Ltd. All rights reserved.
//

#ifndef __BoidsPvpServer__MessageQueue__
#define __BoidsPvpServer__MessageQueue__

#include "pvp.pb.h"
#include <list>
#include "boost/asio.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/weak_ptr.hpp"

namespace boids {
    
    typedef boost::shared_ptr<UserOperation> UserOperationPtr;
    typedef boost::shared_ptr<UserOperationPackage> UserOperationPackagePtr;
    typedef boost::shared_ptr<PvpMessage> PvpMessagePtr;
    typedef boost::shared_ptr<GameInitData> GameInitDataPtr;
    typedef boost::shared_ptr<GameMessage> GameMessagePtr;
    
    class BoidsMessage {
    public:
        BoidsMessage( PvpMessagePtr msg, const boost::asio::ip::udp::endpoint& ep ) : message( msg ), endpoint( ep ) {}
        ~BoidsMessage() {}
        
        PvpMessagePtr message;
        boost::asio::ip::udp::endpoint endpoint;
    };
    
    typedef boost::shared_ptr<BoidsMessage> BoidsMessagePtr;
    
    template <typename T>
    class MessageQueue {
    public:
        MessageQueue() {}
        ~MessageQueue() {}
        
        void enqueue( T msg ) {
            _messages.push_back( msg );
        }
        
        T dequeue() {
            if( _messages.size() > 0 ) {
                T ret = _messages.front();
                _messages.pop_front();
                return ret;
            }
            return nullptr;
        }
        
        void clear() {
            _messages.clear();
        }
        
        bool isEmpty() {
            return _messages.size() == 0;
        }
        
        bool contains( T msg ) {
            for( auto itr = _messages.begin(); itr != _messages.end(); ++itr ) {
                if( *itr == msg ) {
                    return true;
                }
            }
            return false;
        }
        
        void merge( T msg ) {
            if( !this->contains( msg ) ) {
                _messages.push_back( msg );
            }
        }
        
        void merge( MessageQueue& queue ) {
            std::list<T>* messages = queue.getMutableMessages();
            for( auto itr = messages->begin(); itr != messages->end(); ++itr ) {
                if( !this->contains( *itr) ) {
                    _messages.push_back( *itr );
                }
            }
        }
        
        inline const std::list<T>& getMessages() { return _messages; }
        inline std::list<T>* getMutableMessages() { return &_messages; }
        
    private:
        std::list<T> _messages;
    };
}

#endif /* defined(__BoidsPvpServer__MessageQueue__) */

//
//  PvpTerminal.h
//  BoidsPvpServer
//
//  Created by Yanjie Chen on 1/12/15.
//  Copyright (c) 2015 Shanghai Yuyu Network Technology Co. Ltd. All rights reserved.
//

#ifndef __BoidsPvpServer__PvpTerminal__
#define __BoidsPvpServer__PvpTerminal__

#include "../Data/MessageQueue.h"
#include "boost/enable_shared_from_this.hpp"

#define PROTO_ID 0x1
#define SYNC_PROTO_ID 0x2

namespace boids {
    
    class PvpServer;
    class PvpGameServer;
    
    enum PvpTerminalState {
        Disconnected = 0,
        Connected = 1,
        Sync = 2
    };
    
    class PvpTerminal : public boost::enable_shared_from_this<PvpTerminal> {
    public:
        PvpTerminal( boost::asio::io_service& io_service, boost::shared_ptr<PvpServer> server, const boost::asio::ip::udp::endpoint& ep );
        ~PvpTerminal();
        
        inline const boost::asio::ip::udp::endpoint& getEndpoint() { return _endpoint; }
        inline void setEndpoint( const boost::asio::ip::udp::endpoint& endpoint ) { _endpoint = endpoint; }
        
        inline PvpTerminalState getPvpTerminalState() { return _state; }
        inline void setPvpTerminalState( PvpTerminalState state ) { _state = state; }
        
        inline unsigned short getAckNo() { return _ack_no; }
        inline unsigned int getAckBits() { return _ack_bits; }
        
        inline unsigned short getSeqNo() { return _seq_no; }
        inline unsigned int getRemoteAckBits() { return _remote_ack_bits; }
        
        void enterGame( boost::shared_ptr<PvpGameServer> game );
        void quitGame();
        
        bool receiveMessage( PvpMessagePtr message );
        bool sendMessage( PvpMessagePtr message, unsigned short proto_id = PROTO_ID );
        bool sendMessages( std::list<PvpMessagePtr> message_list );
        void resendMessages();
        
        void startTimer( long interval );
        void triggerUpdate( const boost::system::error_code& error );
        
    private:
        bool directSendMessage( PvpMessagePtr message );
        
        PvpMessagePtr getSentMessage( unsigned short seq_no );
        void dropMessage( unsigned short seq_no );
        
        void parseMessage( PvpMessagePtr message );
        
        int diffBetweenSequenceNumbers( unsigned short seq_one, unsigned seq_two );
        unsigned short nextSeqNo( unsigned short seq_no );
        unsigned short prevSeqNo( unsigned short seq_no, unsigned short bias );
        
    private:
        //resend timer
        long _resend_interval;
        boost::asio::deadline_timer _timer;
        
        boost::weak_ptr<PvpServer> _server;
        boost::weak_ptr<PvpGameServer> _game_server;
        
        boost::asio::ip::udp::endpoint _endpoint;
        PvpTerminalState _state;
        
        unsigned short _ack_no;
        unsigned int _ack_bits;
        
        unsigned short _seq_no;
        unsigned int _remote_ack_bits;
        
        unsigned int _max_resent_bit;
        
        MessageQueue<PvpMessagePtr> _sent_message;
    };
    
    typedef boost::shared_ptr<PvpTerminal> PvpTerminalPtr;
}

#endif /* defined(__BoidsPvpServer__PvpTerminal__) */

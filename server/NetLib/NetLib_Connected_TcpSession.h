#ifndef _NETLIB_CONNECTED_TCPSESSION_H_
#define _NETLIB_CONNECTED_TCPSESSION_H_

#include "NetLib.h"
#include "NetLib_Session_Owner.h"
#include "NetLib_Packet.h"
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/locks.hpp>
#include <memory>
#include <queue>
#include "../Log/Log.h"

using namespace boost::asio::ip;

//这个类总是由Owner来释放，Owner会等到Connected_TcpSession没有任何pending操作之后才释放他。有明确的释放时机，所以外部其实不需要用shared_ptr
class NetLib_Connected_TcpSession
{
protected:
	std::shared_ptr<NetLib_Session_Owner> owner; //session不会去释放owner. 只有owner会来手动释放session
	boost::asio::io_service& boostioservice;
	bool m_isconnected;	

	tcp::socket* tcpsocket;	
	std::queue<netlib_packet*> async_send_queue;
	uint32_t receiving_data_size;
	std::shared_ptr<char> receiving_data;

	void tcp_connect_async();
	void tcp_connected_handler(boost::system::error_code& error);

	void _send_async_from_queue(); //内部方法，不加锁

	void send_packet_finish(char* data, void* pHint);
	void send_packet_copy_finish(char* data, void* pHint);
	void tcp_send_async_finished(const boost::system::error_code& error, std::size_t bytes_transferred);
	
	void tcp_receive_async();	
	void tcp_header_recv(const boost::system::error_code& error);
	void tcp_recv_async_finished(const boost::system::error_code& error);
	
	void set_socket(tcp::socket* tcpsocket);

public:
	void close_session_and_handle_error(NetLib_Error error, const boost::system::error_code& boost_error);

	//should be called after construction
	void init(std::shared_ptr<NetLib_Session_Owner> _owner) 
	{
		owner = _owner;
	}

	uint32_t get_remote_ip();
	bool get_remote_address(char* ip, uint16_t& port);
	bool get_remote_address(std::string& ip, uint16_t& port);
	std::string get_local_address();
	void start();
	void send_async(const char* data, bool copy = false, void* pHint = nullptr);

	void process_failed_packets(); //同步方法
	void process_failed_packet(netlib_packet* packet);

	bool is_connected() //在这个方法里加锁没有意义，要有同步意义的话，在is_connected外面加锁
	{		
		return m_isconnected;
	}
	
	void disconnect(NetLib_Error error = close_by_local);

	boost::asio::io_service& GetIoService()
	{
		return boostioservice;
	}

public:	
	virtual ~NetLib_Connected_TcpSession();
	NetLib_Connected_TcpSession(boost::asio::io_service & ioservice); //服务端使用，还会有个类来继承一下
	NetLib_Connected_TcpSession(std::shared_ptr<NetLib_Session_Owner> _owner, boost::asio::io_service & ioservice, tcp::socket& tcpsocket); //客户端使用
};


#endif
#include "NetLib_Connected_TcpSession.h"
#include "NetLib_Params.h"
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include "Log/Log.h"

NetLib_Connected_TcpSession::NetLib_Connected_TcpSession(boost::asio::io_service & ioservice) : boostioservice(ioservice), m_isconnected(true)
{
}

NetLib_Connected_TcpSession::NetLib_Connected_TcpSession(std::shared_ptr<NetLib_Session_Owner> _owner, boost::asio::io_service & ioservice, tcp::socket& _socket) : 
	boostioservice(ioservice), m_isconnected(true), tcpsocket(&_socket)
{
	init(_owner);
}

NetLib_Connected_TcpSession::~NetLib_Connected_TcpSession()
{	
	//LOGEVENTL("NetLib_Trace", "NetLib_Connected_TcpSession deconstruction. " << _ln("owner_ptr") << hex(owner.get()) << _ln("self_ptr") << hex((std::size_t)this));
	LOGEVENTL("~ConnectedSession", _ln("owner") << hex((std::size_t)owner.get()) << _ln("self") << hex((std::size_t)this));
}

void NetLib_Connected_TcpSession::set_socket(tcp::socket* _socket)
{
	tcpsocket = _socket;
}

void NetLib_Connected_TcpSession::start()
{	
	boost::lock_guard<boost::recursive_mutex> lock(owner->get_mutex());
	owner->increase_pending_ops_count(); //在这儿加了，就不用在tcp_receive_async里面每次加了
	tcp_receive_async();
/*
	boost::asio::socket_base::receive_buffer_size option;
	boost::system::error_code error;
	tcpsocket->get_option(option, error);
	LOGEVENTL("TCPRecvBuffer", option.value());*/
}

void NetLib_Connected_TcpSession::tcp_receive_async()
{
	boost::lock_guard<boost::recursive_mutex> lock(owner->get_mutex());
	boost::asio::async_read(*tcpsocket,
		boost::asio::buffer((char*)(&receiving_data_size), 4),
		boost::asio::transfer_at_least(4),
		boost::bind(&NetLib_Connected_TcpSession::tcp_header_recv, this, boost::asio::placeholders::error));
}

void NetLib_Connected_TcpSession::tcp_header_recv(const boost::system::error_code& error)
{
	boost::lock_guard<boost::recursive_mutex> lock(owner->get_mutex());
	if (error || !m_isconnected)
	{
		if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset || error == boost::asio::error::connection_refused)
		{
			close_session_and_handle_error(tcp_disconnect_by_remote, error);
		}
		else
		{
			close_session_and_handle_error(client_recv_error, error); 
		}
		owner->decrease_pending_ops_count(); //只有error的时候需要decrease，因为其它时候都会继续收
	}
	else
	{
		if (receiving_data_size > MAX_PACKET_SIZE)
		{
			close_session_and_handle_error(packet_too_big, error);
			owner->decrease_pending_ops_count(); //只有error的时候需要decrease，因为其它时候都会继续收。add @ 11.28，以前估计忘了
			return;
		}

		if (receiving_data_size < 4) 
		{
			close_session_and_handle_error(packet_too_small, error); 
			owner->decrease_pending_ops_count(); //只有error的时候需要decrease，因为其它时候都会继续收。add @ 11.28，以前估计忘了
			return;
		}

		//其实这里不用shared_ptr也可以。用shared_ptr的话就要注意别被提前释放
		receiving_data = std::shared_ptr<char>(new char[receiving_data_size], [](char* d){delete[] d;} );
		memcpy(receiving_data.get(), &receiving_data_size, 4);

		boost::asio::async_read(*tcpsocket,
			boost::asio::buffer(receiving_data.get() + 4, receiving_data_size - 4),
			boost::asio::transfer_at_least(receiving_data_size - 4),
			boost::bind(&NetLib_Connected_TcpSession::tcp_recv_async_finished, this, boost::asio::placeholders::error));
	}
}

void NetLib_Connected_TcpSession::tcp_recv_async_finished(const boost::system::error_code& error)
{	
	//无锁方法。所有recv方法是由内部依次调用的，只要该方法内的所有直接访问的变量都只和recv系列方法有关，就不会有人来修改。
	//而且参数里keep_alive了，也不用担心中途被释放
	//只有m_isconnected不是recv相关的，但m_isconnected无所谓

	if (error || !m_isconnected)
	{		
		if (error == boost::asio::error::eof || error == boost::asio::error::connection_reset || error == boost::asio::error::connection_refused)
		{
			close_session_and_handle_error(tcp_disconnect_by_remote, error);
		}
		else
		{
			close_session_and_handle_error(client_recv_error, error); 
		}
		owner->decrease_pending_ops_count(); //只有error的时候需要decrease，因为其它时候都会继续收
	}
	else
	{
		owner->RecvFinishHandler(receiving_data.get()); //这个Handler不会post，所以是处理完了再收下一个，shared_ptr不会被释放

		tcp_receive_async();
	}
}

void NetLib_Connected_TcpSession::send_packet_copy_finish(char* data, void* pHint)
{
	owner->SendCopyFinishHandler(data, pHint);
	delete[] data;

	owner->decrease_pending_ops_count();
}

void NetLib_Connected_TcpSession::send_packet_finish(char* data, void* pHint)
{
	owner->SendFinishHandler(data, pHint);

	owner->decrease_pending_ops_count();
}

void NetLib_Connected_TcpSession::tcp_send_async_finished(const boost::system::error_code& error, std::size_t bytes_transferred)
{	
	boost::lock_guard<boost::recursive_mutex> lock(owner->get_mutex());
	if (error || !m_isconnected)
	{
		close_session_and_handle_error(client_recv_error, error);		
		owner->decrease_pending_ops_count();
	}
	else
	{	
		netlib_packet* front_packet = async_send_queue.front();

		if (front_packet->copy)
		{
			boostioservice.post(boost::bind(&NetLib_Connected_TcpSession::send_packet_copy_finish, this, front_packet->data, front_packet->pHint));
		}
		else
		{
			boostioservice.post(boost::bind(&NetLib_Connected_TcpSession::send_packet_finish, this, front_packet->data, front_packet->pHint));
			//why post: 
			//1). we should let the async_send_queue.front() be released first. avoid SendFinishHandler calls something and affect async_send_queue
			//2). Handler都不应在锁内，避免死锁
		}

		delete front_packet;
		async_send_queue.pop();

		if (async_send_queue.size() > 0)
		{
			//里面会加pending_ops
			_send_async_from_queue();
		}
	}
}

void NetLib_Connected_TcpSession::disconnect(NetLib_Error error)
{
	boost::system::error_code boost_empty_error;
	close_session_and_handle_error(error, boost_empty_error);
}

//其实这个方法可以不加keep_alive参数，只要保证他的所有调用者都已经把connected_session给keep_alive了，并且对他都是直接调用而非post调用即可。
//但还是加上keep_alive了，避免逻辑复杂，这点性能损失不算什么
void NetLib_Connected_TcpSession::close_session_and_handle_error(NetLib_Error error, const boost::system::error_code& boost_error)
{		
	boost::lock_guard<boost::recursive_mutex> lock(owner->get_mutex());
	if (m_isconnected)
	{		
		m_isconnected = false;	//这个变量，从初始化为true之后，总共只会变化一次

		if (error != close_by_local && !tcpsocket->is_open())
		{
			LOGEVENTL("Debug", "close_session, but tcpsocket->is_open() already false.");
		}

		//不管现在是不是open，都shutdown和close一下
		boost::system::error_code ignored_error;
		tcpsocket->shutdown(boost::asio::socket_base::shutdown_both, ignored_error);
		tcpsocket->close(ignored_error);
			
		//server不是session_detail的owner, serversession才是session_detail的owner
			
		//这里不能用post，因为对于client来说，我们要保证error先被记录，然后再触发decrease_pending_ops，用post的话顺序就没法保证了
		owner->handle_error(error, boost_error.value()); //一次连接中，只有第一个error需要被记录，后面的error扔了就扔了
	}
}

//这个方法不用加锁，因为它只会在m_isconnected=false之后才会被调用，因此不可能被send_async所干扰
//费劲心思不加锁，是因为里面有回调要执行。。
void NetLib_Connected_TcpSession::process_failed_packets()
{	
	/*if (async_send_queue.size())
	{
		LOGEVENTL("NetLib_Debug", "TcpSession failed_data: " << async_send_queue.size());
	}*/

	while (async_send_queue.size())
	{
		process_failed_packet(async_send_queue.front());
		async_send_queue.pop();
	}
}

void NetLib_Connected_TcpSession::process_failed_packet(netlib_packet* packet)
{
	if (packet->copy)
	{
		if (!owner->SendCopyFailedHandler(packet->data, packet->pHint)) //返回true表示还要重发不用删，返回false表示可以删了
		{
			//LOGEVENTL("NetLib_Trace", "data_copy: 0x" << std::hex << (std::size_t)packet->data << " deleted.");
			delete[] packet->data;
		}
	}
	else
	{
		owner->SendFailedHandler(packet->data, packet->pHint);
	}
	delete packet;
}

void NetLib_Connected_TcpSession::_send_async_from_queue()
{	
	boost::lock_guard<boost::recursive_mutex> lock(owner->get_mutex());
	
	owner->increase_pending_ops_count();

	netlib_packet* front_packet = async_send_queue.front();

	boost::asio::async_write(*tcpsocket, boost::asio::buffer(front_packet->data, front_packet->data_size),
		boost::bind(&NetLib_Connected_TcpSession::tcp_send_async_finished, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

//这个方法体执行的时候，因为锁保护，可以假设一直是m_isconnected状态(虽然底层有可能断了)，即不用担心会和m_isconnected=false之后发生的操作如process_failed_packets互相干扰。
void NetLib_Connected_TcpSession::send_async(const char* data, bool copy, void* pHint) //该方法肯定是通过shared_ptr调用的，不必keep_alive
{		
	boost::lock_guard<boost::recursive_mutex> lock(owner->get_mutex());

	netlib_packet* packet = new netlib_packet(data, copy, pHint);				
	async_send_queue.push(packet);
	if (async_send_queue.size() == 1)
	{
		//async_send can only be called after the previous one has been finished
		//if size() > 1, means there are packet(s) still be sending, then we needn't do any thing, just push the packet into queue.

		_send_async_from_queue();

		//boostioservice.post(boost::bind(&NetLib_Connected_TcpSession::send_async_from_queue, this, shared_from_this())); 
		//why post: avoid RecvFinishHandler() (or other handler) indirectly calls send_async_from_queue() before the handler finish
	}
}

bool NetLib_Connected_TcpSession::get_remote_address(char* ip, uint16_t& port)
{	
	boost::lock_guard<boost::recursive_mutex> lock(owner->get_mutex());
	boost::system::error_code error;
	const tcp::endpoint& ep = tcpsocket->remote_endpoint(error);
	if (error)
	{
		return false;
	}
	strcpy(ip, ep.address().to_string(error).c_str());
	if (error)
	{
		return false;
	}
	port = ep.port();
	return true;
}

bool NetLib_Connected_TcpSession::get_remote_address(std::string& ip, uint16_t& port)
{	
	boost::lock_guard<boost::recursive_mutex> lock(owner->get_mutex());
	boost::system::error_code error;
	const tcp::endpoint& ep = tcpsocket->remote_endpoint(error);
	if (error)
	{
		return false;
	}
	ip = ep.address().to_string(error);
	if (error)
	{
		return false;
	}
	port = ep.port();
	return true;
}

uint32_t NetLib_Connected_TcpSession::get_remote_ip()
{
	boost::lock_guard<boost::recursive_mutex> lock(owner->get_mutex());
	boost::system::error_code error;
	const tcp::endpoint& ep = tcpsocket->remote_endpoint(error);
	if (error)
	{
		return 0;
	}
	return ep.address().to_v4().to_ulong();
}

std::string NetLib_Connected_TcpSession::get_local_address()
{
	boost::system::error_code error;
	const tcp::endpoint& ep = tcpsocket->local_endpoint(error);
	if (error)
	{
		return "";
	}
	else
	{
		return ep.address().to_string(error);
	}
}
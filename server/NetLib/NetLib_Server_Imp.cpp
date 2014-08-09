#include "NetLib_Server_Imp.h"
#include "NetLib_Server_Connected_TcpSession.h"
#include "NetLib_ServerSession_Imp.h"
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/thread/locks.hpp>
#include <boost/array.hpp>
#include "../include/ioservice_thread.h"
#include "NetLib_Params.h"
#include "Log/Log.h"

NetLib_Server_Imp::NetLib_Server_Imp(std::shared_ptr<NetLib_Server_Delegate> d, ioservice_thread& _thread) :
thread(_thread), m_ioservice(_thread.get_ioservice()),
	m_theDelegate(d), m_tcpacceptor(m_ioservice), 
	m_started(false), m_closed(false)
{
	srand(20120206);
	LOGEVENTL("NetLib_Info", "NetLib_Server Construct. " << log_::h((std::size_t)this));
}

NetLib_Server_Imp::~NetLib_Server_Imp() //现在Server的析构函数是在另一个线程里被调用的。没有任何锁
{
	LOGEVENTL("NetLib_Debug", "NetLib_Server Released. " << log_::h((std::size_t)this));
}

void NetLib_Server_Imp::connected_handler(std::shared_ptr<NetLib_ServerSession_Imp>& sessionptr)
{	
	int sessions_count;
	{		
		boost::lock_guard<boost::recursive_mutex> lock(mutex);
		sessions_set.insert(sessionptr);
		sessions_count = sessions_set.size();
	}
	LOGEVENTL("NetLib_SessionsCount", "increase to " << sessions_count);

	sessionptr->server = shared_from_this();
	sessionptr->refresh_timeout_timer();
	sessionptr->theDelegate->ConnectedHandler(sessionptr);
}

void NetLib_Server_Imp::tcp_accept_async()
{	
	if (m_closed) return;
	boost::lock_guard<boost::recursive_mutex> lock(tcp_mutex);
	NetLib_Server_Connected_TcpSession* _session = new NetLib_Server_Connected_TcpSession(m_ioservice);
	std::shared_ptr<NetLib_Connected_TcpSession> session = std::shared_ptr<NetLib_Connected_TcpSession>(_session);
	m_tcpacceptor.async_accept(_session->get_socket(),
			boost::bind(&NetLib_Server_Imp::tcp_accept_handler, this, shared_from_this(), session, boost::asio::placeholders::error));
}

void NetLib_Server_Imp::tcp_accept_handler(std::shared_ptr<NetLib_Server_Imp> keep_alive, std::shared_ptr<NetLib_Connected_TcpSession> session_detail, const boost::system::error_code& error)
{
	if (error)
	{
		m_theDelegate->ErrorHandler(server_tcp_accept_error, error.value());
	}
	else
	{
		if (m_closed) return;

		tcp_accept_async();

		NetLib_ServerSession_Imp* session = new NetLib_ServerSession_Imp(m_theDelegate->New_SessionDelegate(), m_theDelegate, m_ioservice, m_timeout_seconds);
		session->session_detail = session_detail;
		std::shared_ptr<NetLib_ServerSession_Imp> session_ptr = std::shared_ptr<NetLib_ServerSession_Imp>(session);
		session_detail->init(session_ptr);
		
		connected_handler(session_ptr);

		//多线程的情况下，要先调connected_handler再调start，否则可能会先触发断线的系列行为，再触发connected的相关行为，
		//导致handle_error_nolock的时候server都还没被赋值，也导致DisconnectedHandler可能先于ConnectedHandler被调用
		session_detail->start();
	}
}

void NetLib_Server_Imp::erase_session(NetLib_ServerSession_ptr sessionptr)
{
	int sessions_count;
	{
		boost::lock_guard<boost::recursive_mutex> lock(mutex);
		sessions_set.erase(sessionptr);
		sessions_count = sessions_set.size();
	}	
	LOGEVENTL("NetLib_SessionsCount", "decrease to " << sessions_count << " " << _ln("sessionptr") << hex((std::size_t)sessionptr.get()));
}

bool NetLib_Server_Imp::tcp_start_listen(int listen_port)
{
	mutex.lock();	
	mutex.unlock();

	//接下来这里不用进锁，线程都还没启动呢

	boost::system::error_code error, error4close;
	m_tcpacceptor.open(tcp::v4(), error);
	if (error)
	{
		LOGEVENTL("Error", "tcp open error: " << error.value());
		m_theDelegate->ErrorHandler(server_tcp_open_fail, error.value());
		return false;
	}

	boost::system::error_code ignored_error;
	m_tcpacceptor.set_option(tcp::acceptor::reuse_address(true), ignored_error);

	if (ignored_error)
	{
		LOGEVENTL("Warn", "set_option(reuse_address) error: " << ignored_error.value());
	}

	m_tcpacceptor.bind(tcp::endpoint(tcp::v4(), listen_port), error);
	if (error)
	{
		LOGEVENTL("Error", "tcp bind error: " << error.value());
		m_tcpacceptor.close(error4close);
		m_theDelegate->ErrorHandler(server_tcp_bind_fail, error.value());
		return false;
	}
	m_tcpacceptor.listen(boost::asio::socket_base::max_connections, error);
	if (error)
	{
		LOGEVENTL("Error", "tcp listen error: " << error.value());
		m_tcpacceptor.close(error4close);
		m_theDelegate->ErrorHandler(server_tcp_listen_fail, error.value());
		return false;
	}

	if (m_flags & NETLIB_FLAG_TCP_NODELAY)
	{
		boost::system::error_code ignored_error;
		m_tcpacceptor.set_option(boost::asio::ip::tcp::no_delay(true), ignored_error); //TODO: is this enough? need test
		if (ignored_error)
		{
			LOGEVENTL("Warn", "set_option(no_delay) error: " << ignored_error.value());
		}
	}

	boost::asio::socket_base::receive_buffer_size option(SERVER_RECV_BUFFER_SIZE);
	boost::system::error_code error4recv_buf;
	m_tcpacceptor.set_option(option, error4recv_buf);
	if (error4recv_buf)
	{
		LOGEVENTL("Warn", "(tcp) set_option(receive_buffer_size) error: " << error4recv_buf.value());
	}

	tcp_accept_async();

	return true;
}

bool NetLib_Server_Imp::initialize(int timeout_seconds, uint64_t flags)
{
	mutex.lock();
	if (m_started || m_closed)
	{
		mutex.unlock();
		LOGEVENTL("Warn", "You cannot start a Server twice. Stop the Server and Start again is no use. Nothing happened.");
		return false;
	}

	m_timeout_seconds = timeout_seconds;
	m_flags = flags;
	m_started = true;

	mutex.unlock();

	return true;
}

bool NetLib_Server_Imp::StartTCP(int listen_port, int work_thread_num, int timeout_seconds, uint64_t flags)
{
	if (!initialize(timeout_seconds, flags))
	{
		return false;
	}

	if (!tcp_start_listen(listen_port))
	{
		return false;
	}

	thread.start(work_thread_num, true); //如果这个ioservice_thread对象已经启动，那么这行就会失效。关系不大，只要使用者清楚这一点就行

	return true;
}

bool NetLib_Server_Imp::Start(int tcp_listen_port, int udp_listen_port, int work_thread_num, int timeout_seconds, uint64_t flags)
{	
	return StartTCP(tcp_listen_port, work_thread_num, timeout_seconds, flags);
}

void NetLib_Server_Imp::Stop()
{
	m_closed = true;

	boost::lock_guard<boost::recursive_mutex> lock(mutex);
	tcp_mutex.lock();
	if (m_tcpacceptor.is_open())
	{
		boost::system::error_code ignored_error;
		m_tcpacceptor.close(ignored_error);
	}
	tcp_mutex.unlock();

	mutex.lock();
	for (std::set<NetLib_ServerSession_ptr>::iterator it = sessions_set.begin(); it != sessions_set.end(); ++it)
	{
		if (*it)
		{
			(*it)->Disconnect();
		}
	}
	mutex.unlock();
}

boost::asio::io_service* NetLib_Server_Imp::GetWorkIoService()
{
	return &m_ioservice;
}

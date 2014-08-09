#ifndef _NETLIB_SERVER_IMP_H_
#define _NETLIB_SERVER_IMP_H_

#include "NetLib.h"
#include "NetLib_Server_Connected_TcpSession.h"
#include "NetLib_Session_Owner.h"
#include <boost/asio.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <set>
#include <map>

using namespace boost::asio::ip;

#define MAX_UINT32		(0x11111111)

class NetLib_Server_Imp : public NetLib_Server_Interface, public std::enable_shared_from_this<NetLib_Server_Imp>
{
	friend class NetLib_ServerSession_Imp;
protected:
	std::shared_ptr<NetLib_Server_Delegate> m_theDelegate;
	class ioservice_thread& thread;
	boost::asio::io_service& m_ioservice;
	uint64_t m_flags;
	tcp::acceptor m_tcpacceptor;
	bool m_started, m_closed;
	boost::recursive_mutex mutex;
	boost::recursive_mutex tcp_mutex;
	int m_timeout_seconds;
	std::set<NetLib_ServerSession_ptr> sessions_set;

protected:
	void connected_handler(std::shared_ptr<class NetLib_ServerSession_Imp>& sessionptr);

	void tcp_accept_async();
	void tcp_accept_handler(std::shared_ptr<NetLib_Server_Imp> keep_alive, std::shared_ptr<NetLib_Connected_TcpSession> session_detail, const boost::system::error_code& error);

	void erase_session(NetLib_ServerSession_ptr session_ptr);

	bool initialize(int timeout_seconds = 300, uint64_t flags = 0);
	bool tcp_start_listen(int listen_port);

public:
	NetLib_Server_Imp(std::shared_ptr<NetLib_Server_Delegate> d, ioservice_thread& thread);
	virtual ~NetLib_Server_Imp();

	bool Start(int tcp_listen_port, int udp_listen_port, int work_thread_num = 1, int timeout_seconds = 300, uint64_t flags = 0);
	bool StartTCP(int listen_port, int work_thread_num = 1, int timeout_seconds = 300, uint64_t flags = 0);
	bool StartUDP(int listen_port, int work_thread_num = 1, int timeout_seconds = 300, uint64_t flags = 0) { return false; }
	void Stop();
	boost::asio::io_service* GetWorkIoService();
};

#endif

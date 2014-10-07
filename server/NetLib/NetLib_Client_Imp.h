#ifndef _NETLIB_CLIENT_IMP_H_
#define _NETLIB_CLIENT_IMP_H_

#include "NetLib.h"
#include "NetLib_Connected_TcpSession.h"
#include "NetLib_Session_Owner.h"
#include "NetLib_Packet.h"
#include <boost/asio.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <queue>

using namespace boost::asio::ip;

class NetLib_Client_Imp : public NetLib_Client_Interface, public NetLib_Session_Owner, public std::enable_shared_from_this<NetLib_Client_Imp>
{
protected:
	std::shared_ptr<NetLib_Client_Delegate> theDelegate;
	boost::asio::io_service & boostioservice;
	tcp::socket tcpsocket;
	std::string m_dest_ip;
	uint32_t m_dest_ip_u;
	uint16_t m_tcp_port;
	uint64_t m_flags;
	bool m_in_disconnect_process;
	NetLib_Error m_last_error;
	int m_last_internal_error;
	bool m_manually_disconnect;
	bool m_enable_reconnect, m_will_reconnect_if_disconnected;
	bool m_first_connect; //第一次连。主要是避免第一次连没连上就立即重连
	int m_reconnect_interval_ms, m_max_continuous_retries, m_currently_retries;
	boost::asio::deadline_timer reconnect_retry_timer;
	boost::asio::deadline_timer keep_alive_timer;
	int m_keepalive_interval_seconds;
	std::queue< netlib_packet > failed_data_queue;
	std::shared_ptr<class NetLib_Connected_TcpSession> connected_session; //是否已连接只用connected_session是否有内容来判断，免得多个变量含义不清
	boost::recursive_mutex client_mutex;

	boost::recursive_mutex& get_mutex();

	void send_keep_alive(std::shared_ptr<NetLib_Client_Imp> keep_alive, const boost::system::error_code& error);
	void send_keep_alive_in_future();

	void decrease_pending_ops_count();
	void disconnected(std::shared_ptr<NetLib_Client_Imp> keep_alive);
	
	void connected_handler();
	void connect_async();
	void reconnect_timer_pulse(std::shared_ptr<NetLib_Client_Imp> keep_alive, const boost::system::error_code& error);

	void tcp_connect_async();
	void tcp_connected_handler(std::shared_ptr<NetLib_Client_Imp> keep_alive, const boost::system::error_code& error); //async handlers need to keep this() alive

	void _connect_async(const char* ip_s, uint32_t ip_u, uint16_t port, uint64_t flags = 0);
	
public:
	void SendFinishHandler(char* data, void* pHint) //as long as connected_sessions is connected, the session_container is kept alive
	{
		theDelegate->SendFinishHandler(shared_from_this(), data, pHint);		
	}

	void SendCopyFinishHandler(char* data, void* pHint)
	{
		theDelegate->SendCopyFinishHandler(shared_from_this(), data, pHint);
	}

	void RecvFinishHandler(char* data)  //as long as connected_sessions is connected, the session_container is kept alive
	{
		theDelegate->RecvFinishHandler(shared_from_this(), data);
	}

	void SendFailedHandler(const char* data, void* pHint);

	bool SendCopyFailedHandler(const char* data_copy, void* pHint);

	void SendAsyncFailed(std::shared_ptr<NetLib_Client_Imp> keep_alive, const char* data, void* pHint);

	//SendCopyAsync直接失败了(connected_session已经断了)，就进这个方法
	void SendCopyAsyncFailed(std::shared_ptr<NetLib_Client_Imp> keep_alive, const char* data_copy, void* pHint);

	void ResetFailedData();

	void handle_error(NetLib_Error error, int error_code);

public:
	NetLib_Client_Imp(std::shared_ptr<NetLib_Client_Delegate> d, boost::asio::io_service & ioservice);
	virtual ~NetLib_Client_Imp();

	void Disconnect();
	bool IsConnected();
	void ConnectAsync(const char* ip, uint16_t tcp_port, uint64_t _flags);
	void ConnectAsync(uint32_t ip, uint16_t tcp_port, uint64_t _flags);

	void SendAsync(const char* data, void* pHint = nullptr);
	void SendCopyAsync(const char* data, void* pHint = nullptr);

	void DisableReconnect();
	void EnableReconnect(int reconnect_interval_ms = 3000, int max_continuous_retries = -1); 
		
	boost::asio::io_service* GetWorkIoService();

	void SetKeepAliveIntervalSeconds(int keepalive_interval_seconds = 240);
};

#endif
#ifndef _NETLIB_SERVERSESSION_INTERFACE_H_
#define _NETLIB_SERVERSESSION_INTERFACE_H_

#include "NetLib.h"
#include "NetLib_Connected_TcpSession.h"
#include <boost/thread/recursive_mutex.hpp>
#include <string>

class NetLib_ServerSession_Imp : public NetLib_ServerSession_Interface, public NetLib_Session_Owner, public std::enable_shared_from_this<NetLib_ServerSession_Imp>
{	
	friend class NetLib_Server_Imp;
protected:
	std::shared_ptr<NetLib_Connected_TcpSession> session_detail;
	boost::recursive_mutex session_mutex;
	NetLib_ServerSession_Delegate* theDelegate;
	std::shared_ptr<NetLib_Server_Delegate> serverDelegate;	
	boost::asio::io_service& boostioservice;
	boost::asio::deadline_timer m_keep_alive_timer;
	int m_timeout_seconds;
	boost::recursive_mutex& get_mutex();
	std::shared_ptr<class NetLib_Server_Imp> server;

	NetLib_ServerSession_Imp(NetLib_ServerSession_Delegate* d, std::shared_ptr<NetLib_Server_Delegate> sd, boost::asio::io_service& ioservice, int timeout_seconds);
	
	void decrease_pending_ops_count();
	
	void refresh_timeout_timer();
	void timeout_handler(std::shared_ptr<NetLib_ServerSession_Imp> keep_alive, const boost::system::error_code& error);

	void disconnected();
	void handle_error(NetLib_Error error, int error_code);

	void SendFinishHandler(char* data, void* pHint);
	void SendCopyFinishHandler(char* data, void* pHint);

	//'data' will be released just after RecvFinishHandler returns
	void RecvFinishHandler(char* data);

	void SendFailedHandler(const char* data, void* pHint);
	bool SendCopyFailedHandler(const char* data_copy, void* pHint);

	//SendCopyAsync直接失败(session已经断了)，就进这个方法
	void SendCopyAsyncFailed(std::shared_ptr<NetLib_ServerSession_Imp> keep_alive, const char* data_copy, void* pHint);

	int m_attached_data;

public:	
	virtual ~NetLib_ServerSession_Imp(); //don't directly delete

	//'data' must be retained until SendFinishHandler has been called
	void SendAsync(const char* data, void* pHint = nullptr);

	void SendCopyAsync(const char* data, void* pHint = nullptr);

	bool GetRemoteAddress(char* ip, uint16_t& port);
	bool GetRemoteAddress(std::string& ip, uint16_t& port);
	std::string GetRemoteIP();
	uint32_t GetRemoteIPu();

	std::string GetLocalAddress();

	int GetAttachedData(); 
	void SetAttachedData(int d);

	void Disconnect();
};

#endif
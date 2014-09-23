#include "NetLib_ServerSession_Imp.h"
#include "NetLib_Server_Imp.h"
#include "Log/Log.h"
#include <boost/bind.hpp>

NetLib_ServerSession_Imp::NetLib_ServerSession_Imp(NetLib_ServerSession_Delegate* d, std::shared_ptr<NetLib_Server_Delegate> sd, boost::asio::io_service& ioservice, int timeout_seconds): 
	theDelegate(d), serverDelegate(sd), boostioservice(ioservice), m_keep_alive_timer(boostioservice), m_timeout_seconds(timeout_seconds)
{
}

NetLib_ServerSession_Imp::~NetLib_ServerSession_Imp()
{
	serverDelegate->Release_SessionDelegate(theDelegate);
	
	//LOGEVENTL("NetLib_Trace", "NetLib_ServerSession_Imp deconstruction " << _ln("ptr") << hex((std::size_t)this));
}

boost::recursive_mutex& NetLib_ServerSession_Imp::get_mutex()
{
	return session_mutex;
}

void NetLib_ServerSession_Imp::refresh_timeout_timer() //session建立后，该方法至少会被server调用一次，然后每次收到包即刷新超时时间
{
	m_keep_alive_timer.expires_from_now(boost::posix_time::seconds(m_timeout_seconds));
	//LOGEVENTL("NetLib_Trace", "ServerSession timeout_handler refreshed");
	m_keep_alive_timer.async_wait(boost::bind(&NetLib_ServerSession_Imp::timeout_handler, this, shared_from_this(), boost::asio::placeholders::error));

	//这个timer和session_detail没有关系，不用increasing_pending_ops_count。他是ServerSession类的pending_op
}

//session_detail要keep_alive, ServerSession_Imp本身也要keep_alive，因为这是个异步调用的方法
void NetLib_ServerSession_Imp::timeout_handler(std::shared_ptr<NetLib_ServerSession_Imp> keep_alive, const boost::system::error_code& error) 
{
	if (!error)
	{		
		boost::lock_guard<boost::recursive_mutex> lock(session_mutex);

		if (session_detail && session_detail->is_connected())
		{
			session_detail->disconnect(session_timeout);
		} //如果session_detail已经被释放了，那就说明连接已经因为别的原因断了，不必再触发超时。Session不是Client，一旦创建，在断开前肯定是有session_detail的
	}
	else
	{
		//LOGEVENTL("NetLib_Trace", "ServerSession timeout_handler canceled");
	}
}

//'data' must be retained until SendFinishHandler has been called
void NetLib_ServerSession_Imp::SendAsync(const char* data, void* pHint)
{	
	session_mutex.lock(); //必须保证m_isconnected为true之后，不再进send_async。进send_async会导致新增pending_ops，到时候pending_ops反复减到0，会导致多次释放

	if (session_detail && session_detail->is_connected())
	{
		session_detail->send_async(data, false, pHint);
		session_mutex.unlock();
	}
	else
	{
		session_mutex.unlock();
		boostioservice.post(boost::bind(&NetLib_ServerSession_Delegate::SendFailedHandler, theDelegate, shared_from_this(), data, pHint)); //Handler必须直接或间接的经过post才能调用，否则有可能导致死锁
	}
}

void NetLib_ServerSession_Imp::SendCopyAsyncFailed(std::shared_ptr<NetLib_ServerSession_Imp> keep_alive, const char* data_copy, void* pHint)
{
	theDelegate->SendCopyFailedHandler(keep_alive, data_copy, pHint);
	delete[] data_copy;
}

void NetLib_ServerSession_Imp::SendCopyAsync(const char* data, void* pHint)
{		
	char* data_copy = new char[*(uint32_t*)data];
	memcpy(data_copy, data, *(uint32_t*)data);

	session_mutex.lock(); //必须保证m_isconnected为true之后，不再进send_async。进send_async会导致新增pending_ops，到时候pending_ops反复减到0，会导致多次释放

	if (session_detail && session_detail->is_connected())
	{
		session_detail->send_async(data_copy, true, pHint);		
		session_mutex.unlock();
	}
	else
	{		
		session_mutex.unlock();
		boostioservice.post(boost::bind(&NetLib_ServerSession_Imp::SendCopyAsyncFailed, this, shared_from_this(), data_copy, pHint)); //Handler必须直接或间接的经过post才能调用，否则有可能导致死锁
	}
}

bool NetLib_ServerSession_Imp::GetRemoteAddress(char* ip, uint16_t& port)
{	
	//加锁也不能保证session_detail不被释放，因此keep_alive一份
	std::shared_ptr<NetLib_Connected_TcpSession> session_detail_keep_alive = session_detail;

	if (session_detail_keep_alive)
	{
		return session_detail_keep_alive->get_remote_address(ip, port);
	}
	else
	{
		return false;
	}
}

bool NetLib_ServerSession_Imp::GetRemoteAddress(std::string& ip, uint16_t& port)
{
	boost::lock_guard<boost::recursive_mutex> lock(session_mutex);

	if (session_detail)
	{
		return session_detail->get_remote_address(ip, port);
	}
	else
	{
		return false;
	}
}

std::string NetLib_ServerSession_Imp::GetRemoteIP()
{
	boost::lock_guard<boost::recursive_mutex> lock(session_mutex);

	if (session_detail)
	{
		std::string ip;
		uint16_t port;
		if (session_detail->get_remote_address(ip, port))
		{
			return ip;
		}
	}
	return "";
}

uint32_t NetLib_ServerSession_Imp::GetRemoteIPu()
{
	boost::lock_guard<boost::recursive_mutex> lock(session_mutex);

	if (session_detail)
	{
		return session_detail->get_remote_ip();
	}
	return 0;
}

std::string NetLib_ServerSession_Imp::GetLocalAddress()
{
	boost::lock_guard<boost::recursive_mutex> lock(session_mutex);

	if (session_detail)
	{
		return session_detail->get_local_address();
	}
	else
	{
		return "";
	} 
}

void NetLib_ServerSession_Imp::Disconnect()
{	
	boost::lock_guard<boost::recursive_mutex> lock(session_mutex);

	if (session_detail)
	{
		//LOGEVENTL("NetLib_Trace", "manually disconnect a ServerSession");
		session_detail->disconnect();
	}
	else
	{
		//LOGEVENTL("NetLib_Trace", "manually disconnect a ServerSession, but session_detail has been released already.");
	}
}

//在pending_ops减到0时执行
void NetLib_ServerSession_Imp::disconnected()
{	
	{
		boost::lock_guard<boost::recursive_mutex> lock(session_mutex);

		session_detail->process_failed_packets();
		session_detail.reset(); //这里虽然切断了session_detail和server_session的联系，但不会导致server_session释放。server_session在接下来的erase_session里面删掉最后一份引用后，才会被释放
	}
	
	server->erase_session(shared_from_this());
}

//只会进一次
void NetLib_ServerSession_Imp::handle_error(NetLib_Error error, int error_code) 
{
	boost::system::error_code ignored_error;
	m_keep_alive_timer.cancel(ignored_error); //虽然析构函数里也会cancel，但那会导致ServerSession的释放时间比session_detail迟好久，因为有一个ServerSession的keepalive在timer上

	boostioservice.post(boost::bind(&NetLib_ServerSession_Delegate::DisconnectedHandler, theDelegate, shared_from_this(), error, error_code));	
}

void NetLib_ServerSession_Imp::decrease_pending_ops_count() //必须在每个pending_op的最后调用
{	
	boost::lock_guard<boost::recursive_mutex> lock(session_mutex);

	m_pending_ops_count --;
	//LOGEVENTL("pending_ops_count", _ln("decrease_to") << m_pending_ops_count << _ln("ptr") << hex((std::size_t)this));
	if (m_pending_ops_count == 0)
	{
		//这时候这个session的pending_ops已经不可能再涨了		
		//post是为了出锁, 同时也使得Server Stop的时候，iterator不会close了一个就导致失效
		boostioservice.post(boost::bind(&NetLib_ServerSession_Imp::disconnected, this)); //post是为了彻底出锁，不管调用者外面还有没锁。
	}
}

void NetLib_ServerSession_Imp::SendFinishHandler(char* data, void* pHint)
{
	theDelegate->SendFinishHandler(shared_from_this(), data, pHint);
}

void NetLib_ServerSession_Imp::SendCopyFinishHandler(char* data, void* pHint)
{
	theDelegate->SendCopyFinishHandler(shared_from_this(), data, pHint);
}

//'data' will be released just after RecvFinishHandler returns
void NetLib_ServerSession_Imp::RecvFinishHandler(char* data) //这些方法的上层都保证了本方法不被中途释放
{
	refresh_timeout_timer();

	if (*(uint32_t*)data == 4)
	{
		/*if (m_flags & NETLIB_SERVER_LISTEN_KEEP_ALIVE_EVENT)
		{
			theDelegate->RecvKeepAliveHandler(shared_from_this());
		}*/
	}
	else
	{
		theDelegate->RecvFinishHandler(shared_from_this(), data);
	}
}

void NetLib_ServerSession_Imp::SendFailedHandler(const char* data, void* pHint)
{
	theDelegate->SendFailedHandler(shared_from_this(), data, pHint);
}

bool NetLib_ServerSession_Imp::SendCopyFailedHandler(const char* data_copy, void* pHint)
{
	theDelegate->SendCopyFailedHandler(shared_from_this(), data_copy, pHint);
	return false;
}
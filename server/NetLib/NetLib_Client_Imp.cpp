#include "NetLib_Client_Imp.h"
#include "NetLib_Connected_TcpSession.h"
#include <boost/bind.hpp>
#include <boost/thread/locks.hpp>
#include <boost/array.hpp>
#include "NetLib_Params.h"
#include "Log/Log.h"

NetLib_Client_Imp::NetLib_Client_Imp(std::shared_ptr<NetLib_Client_Delegate> d, boost::asio::io_service & ioservice) : theDelegate(d), boostioservice(ioservice), tcpsocket(ioservice), 
	m_last_error(no_error), m_last_internal_error(0), 
	m_enable_reconnect(true), m_manually_disconnect(false), m_will_reconnect_if_disconnected(false), m_in_disconnect_process(false),
	m_reconnect_interval_ms(5000), m_max_continuous_retries(-1), m_currently_retries(0), reconnect_retry_timer(ioservice), keep_alive_timer(ioservice), m_keepalive_interval_seconds(240)
{
}

NetLib_Client_Imp::~NetLib_Client_Imp() //由外部保证在锁外调用。(post调用)
{										//需要锁外调用的原因是里面有回调FailedDataReleaseHandler。
	//之所以不直接调用ResetFailedData()，是为了避免进锁和post。
	while (! failed_data_queue.empty())
	{
		if (failed_data_queue.front().copy)
		{
			delete[] failed_data_queue.front().data;
		}
		else
		{
			theDelegate->FailedDataReleaseHandler(failed_data_queue.front().data, failed_data_queue.front().pHint);
		}
		failed_data_queue.pop();
	}
}

void NetLib_Client_Imp::ResetFailedData()
{		
	boost::lock_guard<boost::recursive_mutex> lock(client_mutex);

	while (! failed_data_queue.empty())
	{
		if (failed_data_queue.front().copy)
		{
			delete[] failed_data_queue.front().data;
		}
		else
		{
			boostioservice.post(boost::bind(&NetLib_Client_Delegate::FailedDataReleaseHandler, theDelegate.get(), shared_from_this(), failed_data_queue.front().data, failed_data_queue.front().pHint));
			//这时候可能已经在NetLib_Client_Imp的析构函数里了，不用也不能shared_from_this()。
			//使用者在这个Handler里面基本上也就是做一下delete[] data了
		}
		failed_data_queue.pop();
	}
}

boost::recursive_mutex& NetLib_Client_Imp::get_mutex()
{
	return client_mutex;
}

void NetLib_Client_Imp::Disconnect()
{	
	std::shared_ptr<NetLib_Connected_TcpSession> connected_session_keep_alive = connected_session;

	boost::lock_guard<boost::recursive_mutex> lock(client_mutex);
	
	m_manually_disconnect = true; //Disconnect可以不打断本次的重连过程，但至少不能再让他继续重连了。否则可能出现“不停失败不停重连停都停不掉”的情况
	m_will_reconnect_if_disconnected = false;

	bool perform_disconnect = false;
	if (!m_in_disconnect_process) //避免在自动重连的时候调用Disconnect()，引发各种复杂逻辑
	{
		if (connected_session_keep_alive && connected_session_keep_alive->is_connected() )
		{
			perform_disconnect = true;
			connected_session_keep_alive->disconnect();
		}
	}
	else
	{
		LOGEVENTL("NetLib_Info", "the client you call Disconnect() is already disconnecting.");
	}

	if (!perform_disconnect)
	{
		handle_error(client_cancel_connect_by_local, 0);

		boost::system::error_code ignored_error;

		tcpsocket.close(ignored_error);

		if (ignored_error)
		{
			LOGEVENTL("Warn", "when close a un-connected tcp client, a error occurs: " << ignored_error.value());
		}

		reconnect_retry_timer.cancel(ignored_error);
	}
}

bool NetLib_Client_Imp::IsConnected()
{	
	std::shared_ptr<NetLib_Connected_TcpSession> connected_session_keep_alive = connected_session;

	if (connected_session_keep_alive)
	{
		return connected_session_keep_alive->is_connected();
	}
	return false;
}

void NetLib_Client_Imp::reconnect_timer_pulse(std::shared_ptr<NetLib_Client_Imp> keep_alive, const boost::system::error_code& error)
{
	if (!error) //
	{		
		boost::lock_guard<boost::recursive_mutex> lock(client_mutex);
		m_currently_retries ++;
		connect_async();
	}
}

//同一时间，只会有一个线程进入disconnected的方法体。通过bool m_in_disconnect_process来保证。
void NetLib_Client_Imp::disconnected(std::shared_ptr<NetLib_Client_Imp> keep_alive)
{
	//LOGEVENTL("disconnected", log_::n("ptr") << hex((std::size_t)keep_alive.get()) << log_::n("delegate") << hex((std::size_t)theDelegate.get()));

	//无锁方法，由外部保证线程安全。外部必须保证同一时间本方法涉及的变量或对象都不被修改，或者修改了也不影响。也要保证本方法体做出的修改对外界不产生超出预期的影响。
	//由于是pending_op == 0了才进，因此那些异步方法肯定不会和本方法相互影响了。只要保证手动调用的方法和本方法体内的代码不冲突即可。另外也就是说，和本方法肯定不会相互影响的方法，可以不使用incre/decre pending_op那一套

	bool will_continue_reconnect = m_will_reconnect_if_disconnected && (m_max_continuous_retries == -1 || m_currently_retries < m_max_continuous_retries); //或者无限重连，或者尚未到达最大重连次数

	if (m_currently_retries == 0) //刚断线，还没有重连过
	{
		theDelegate->DisconnectedHandler(shared_from_this(), m_last_error, m_last_internal_error, will_continue_reconnect); 

		if (connected_session)
		{
			connected_session->process_failed_packets(); 
		}
		else
		{
			//这种情况其实很容易发生，ConnectAsync没成功就会进到这儿
		}
	}
	else //已经在有重连次数了，这次是重连失败
	{
		//LOGEVENTL("_reconn_fail", log_::n("ptr") << hex((std::size_t)keep_alive.get()) << _ln("error") << m_last_error << _ln("internal_error") << m_last_internal_error);

		theDelegate->ReconnectFailedHandler(shared_from_this(), will_continue_reconnect); //这个Handler可能会修改will_continue_reconnect的值
	}
	
	//LOGEVENTL("disconnected_half", log_::n("ptr") << hex((std::size_t)keep_alive.get()) << log_::n("connected_session") << hex((std::size_t)connected_session.get()));

	connected_session.reset(); //cut the relationship between the session and its owner. otherwise the shared_ptrs formed a cycle and can't release.
	
	//如果Client被手动Disconnect或Release，则m_will_reconnect_if_disconnected是false，不会再次重连
	//m_will_reconnect_if_disconnected是强条件，即使will_continue_reconnect被改了，如果m_will_reconnect_if_disconnected是false那也没有用
	if (m_will_reconnect_if_disconnected && will_continue_reconnect)
	{
		if (m_currently_retries || (m_currently_retries == 0 && (m_flags & NETLIB_CLIENT_ENABLE_RECONNECT_ON_FIRST_CONNECT)))
		{
			//LOGEVENTL("Info", "will reconnect later");
			//非首次断线，或者第一次连就失败然后需要重连(开了相关选项)，需要延时一段时间再重连
			reconnect_retry_timer.expires_from_now(boost::posix_time::milliseconds(m_reconnect_interval_ms));
			reconnect_retry_timer.async_wait(boost::bind(&NetLib_Client_Imp::reconnect_timer_pulse, this, shared_from_this(), boost::asio::placeholders::error));
		}
		else //刚断线，还没有重连过，且不是首次连接，因此立即重连
		{
			//LOGEVENTL("Info", "reconnect at once");
			m_currently_retries ++;
			connect_async();
			//connect_async有可能进到decrease_pending_ops_count里，有可能会post一个disconnected。但当ioservice只有一个线程在跑时，后post的这个disconnected方法必须等当前的方法体执行完才能进。
		}
	}
	else
	{
		m_in_disconnect_process = false;
		m_will_reconnect_if_disconnected = false; //不再重连
		m_currently_retries = 0; //重置重试次数，因为这个变量也被用来标识是否正在重连
	}

	//NetLib_Client_Imp的一个可能的释放点。只要出这个方法的时候引用计数为0。因为前面我们把connected_session给reset了

	//LOGEVENTL("disconnected_finish", log_::n("ptr") << hex((std::size_t)keep_alive.get()) << log_::n("delegate") << hex((std::size_t)theDelegate.get()));
}

void NetLib_Client_Imp::decrease_pending_ops_count() //必须在每个pending_op的最后调用，因为可能触发connected_session.reset()导致整个client被释放
{	
	boost::lock_guard<boost::recursive_mutex> lock(client_mutex);

	m_pending_ops_count --;
	//LOGEVENTL("pending_ops_count", "decrease to " << m_pending_ops_count);
	if (m_pending_ops_count == 0 && m_last_error != no_error) //有错误才触发disconnected；没有error但是m_pending_ops_count减为0的情况发生在如udp tcp连接的切换，中间可能有0的情况
	{
		m_in_disconnect_process = true; 

		//只有接下来的disconnected方法的末尾会将m_in_disconnect_process置为false. 避免中途被ConnectAsync打扰。SendAsync和Disconnect由于connected_session是断开状态，是不会真正执行的。
		//只要那几个方法不进，m_pending_ops_count就不会再度增加，这个地方也不会再次进来。除非是自动重连的失败可能会再进这儿
		//因此只要保证ioservice只有一个线程在跑，disconnected方法就同一时间只会进一次。
		boostioservice.post(boost::bind(&NetLib_Client_Imp::disconnected, this, shared_from_this())); //post是为了彻底出锁，不管调用者外面还有没锁。
	}
}

//所有的断线、连接失败都会进这儿，但最终触发DisconnectedHandler不在这儿而在decrease_pending_ops_count。需要保证每次handle_error()先于decrease_pending_ops_count调用。
void NetLib_Client_Imp::handle_error(NetLib_Error error, int error_code)  //as long as connected_sessions is connected, the session_container is kept alive
{	
	boost::lock_guard<boost::recursive_mutex> lock(client_mutex);

	//这个方法可能进多次
	//由于该方法的所有调用者都有直接或间接将NetLib_Client_Imp keep_alive，所以这个方法就不加keep_alive了
	if (m_last_error == no_error) //通过这个方法保证只进一次。而m_last_error必须至少等整个client都idle了(没有pending_ops了)才重置
	{
		m_last_error = error;
		m_last_internal_error = error_code;

		boost::system::error_code ignored_error;
		keep_alive_timer.cancel(ignored_error);
	}
}

void NetLib_Client_Imp::send_keep_alive(std::shared_ptr<NetLib_Client_Imp> keep_alive, const boost::system::error_code& error) //send_keep_alive和shared_ptr的keep_alive完全不是一回事
{
	if (!error)
	{
		char send_buf[4];
		*(uint32_t*)send_buf = 4;
		SendCopyAsync(send_buf);

		send_keep_alive_in_future();
	}
	else
	{
		decrease_pending_ops_count();
	}
}

void NetLib_Client_Imp::send_keep_alive_in_future()
{
	boost::system::error_code ignored_error;
	keep_alive_timer.expires_from_now(boost::posix_time::seconds(m_keepalive_interval_seconds), ignored_error);
	keep_alive_timer.async_wait(boost::bind(&NetLib_Client_Imp::send_keep_alive, this, shared_from_this(), boost::asio::placeholders::error));
}

void NetLib_Client_Imp::connected_handler() //该方法只能在锁内调用
{	
	if (m_enable_reconnect)
	{
		m_will_reconnect_if_disconnected = true; //主要的重置m_will_reconnect_if_disconnected的地方。
	}

	if (m_currently_retries == 0) //表示是外部调用ConnectAsync的连接成功
	{		
		boostioservice.post(boost::bind(&NetLib_Client_Delegate::ConnectedHandler, theDelegate.get(), shared_from_this())); //外面还有锁，所以只能通过post来出锁了
	}
	else //表示自动重连成功
	{		
		LOGEVENTL("NetLib_Info", "(" << hex((std::size_t)this) << ") NetLib_Client reconnected."); 

		m_in_disconnect_process = false;
		m_currently_retries = 0;
		boostioservice.post(boost::bind(&NetLib_Client_Delegate::ReconnectedHandler, theDelegate.get(), shared_from_this())); //外面还有锁，所以只能通过post来出锁了
	}

	if (connected_session)
	{
		if (! failed_data_queue.empty())
		{			
			//发送之前失败的数据
			LOGEVENTL("NL_ResendFailedData", log_::n("ptr") << hex((std::size_t)this) << log_::n("queue_size") << failed_data_queue.size());

			do
			{
				connected_session->send_async(failed_data_queue.front().data, failed_data_queue.front().copy, failed_data_queue.front().pHint); //SendFinish或SendFailed会比Connected迟触发，只要ioservice是依次执行队列中的事情的
				failed_data_queue.pop();
			} while (! failed_data_queue.empty());
		}

		//如果需要keep_alive，则启动定时器
		if (m_flags & NETLIB_CLIENT_FLAG_KEEP_ALIVE)
		{
			increase_pending_ops_count();
			send_keep_alive_in_future();
		}
	}
	else
	{
		LOGEVENTL("Error", "ConnectedSession is empty in connected_handler. Client ptr: " << log_::h((std::size_t)this));
	}
}

void NetLib_Client_Imp::tcp_connect_async()
{	
	//LOGEVENTL("NetLib_Trace", "tcp_connect_async");
	boost::lock_guard<boost::recursive_mutex> lock(client_mutex);
	try
	{
		boost::asio::ip::tcp::endpoint endpoint;
		if (m_dest_ip_u == 0)
		{
			endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(m_dest_ip), m_tcp_port);
		}
		else
		{
			endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4(m_dest_ip_u), m_tcp_port);
		}

		increase_pending_ops_count();

		tcpsocket.async_connect(endpoint, 
			boost::bind(&NetLib_Client_Imp::tcp_connected_handler, this, shared_from_this(), boost::asio::placeholders::error));
	}
	catch (std::exception e)
	{		
		LOGEVENTL("Debug", "tcpsocket.async_connect throws exception: " << e.what());

		std::shared_ptr<NetLib_Client_Imp> keep_alive = shared_from_this(); //why this? 11.28
		handle_error(client_connect_error_std_ex, 0);

		decrease_pending_ops_count();
	}
}

void NetLib_Client_Imp::tcp_connected_handler(std::shared_ptr<NetLib_Client_Imp> keep_alive, const boost::system::error_code& error)
{	
	boost::lock_guard<boost::recursive_mutex> lock(client_mutex);
	if (error)
	{
		handle_error(client_connect_error, error.value());
	}
	else
	{		
		if (m_manually_disconnect) //刚连上，却发现已经被手动断开了，就当作没发生过。错误代码应该已经被赋成client_cancel_connect_by_local了
		{
			if (m_last_error != client_cancel_connect_by_local)
			{
				LOGEVENTL("Error", "UNEXPECTED. m_last_error got: " << m_last_error << ", expected client_cancel_connect_by_local(" << client_cancel_connect_by_local << ")");
			}
		}
		else
		{
			if (m_flags & NETLIB_FLAG_TCP_NODELAY)
			{
				boost::asio::ip::tcp::no_delay option(true); 
				boost::system::error_code ignored_error;
				tcpsocket.set_option(option, ignored_error); 
			}

			boost::asio::socket_base::receive_buffer_size option(CLIENT_RECV_BUFFER_SIZE);
			boost::system::error_code error4recv_buf;
			tcpsocket.set_option(option, error4recv_buf);
			if (error4recv_buf)
			{
				LOGEVENTL("Warn", "(tcp client) set_option(receive_buffer_size) error: " << error4recv_buf.value());
			}

			if (connected_session && connected_session->is_connected())
			{
				LOGEVENTL("Fatal", "tcp_connected_handler was triggered when member class connected_session are still connected.");
			}
			else //尚未连接，或上一个connected_session已断开
			{
				connected_session = std::shared_ptr<NetLib_Connected_TcpSession> (new NetLib_Connected_TcpSession(shared_from_this(), boostioservice, tcpsocket));
				connected_session->start();

				connected_handler();
			}
		}
	}
	decrease_pending_ops_count();
}

void NetLib_Client_Imp::connect_async()
{
	//这里不改变量m_will_reconnect_if_disconnected的值，因为首次连和重连都可能进这儿

	boost::lock_guard<boost::recursive_mutex> lock(client_mutex);

	if (m_manually_disconnect)
	{
		LOGEVENTL("NetLib_Debug", "will connect_async, but m_manually_disconnect is true. nothing will be done.");
		//这种情况发生在自动重连的间隔中，外部把连接Disconnect了。因此也不用重连了。m_last_error应是上一次的error，而不是client_cancel_connect_by_local
	}
	else
	{
		m_last_error = no_error;
		m_last_internal_error = 0;

		tcp_connect_async();
	}
}

void NetLib_Client_Imp::_connect_async(const char* ip_s, uint32_t ip_u, uint16_t port, uint64_t flags)
{
	std::shared_ptr<NetLib_Client_Imp> keep_alive = shared_from_this(); //避免在锁外的时候this还健在，等能进锁了this已经释放了。
	boost::lock_guard<boost::recursive_mutex> lock(client_mutex);
	if (m_pending_ops_count != 0 || m_in_disconnect_process)
	{
		LOGEVENTL("Error", "Don't call ConnectAsync on a non-idle client ! Nothing happened.");
	}
	else
	{
		if (ip_s)
		{
			LOGEVENTL("NetLib_Info", log_::n("ptr") << log_::h((std::size_t)this) << ", ConnectAsync: "  //直接打指针gcc会自己带0x，就不统一了，于是转了再打
				<< log_::n("ip") << ip_s << log_::n("port") << port << log_::n("flags") << log_::h(flags));

			m_dest_ip = ip_s;
			m_dest_ip_u = 0;
		}
		else
		{
			LOGEVENTL("NetLib_Info", log_::n("ptr") << log_::h((std::size_t)this) << ", ConnectAsync: "  //直接打指针gcc会自己带0x，就不统一了，于是转了再打
				<< log_::n("ip") << ip_u << log_::n("port") << port << log_::n("flags") << log_::h(flags));

			m_dest_ip_u = ip_u;
		}
		m_manually_disconnect = false;

		m_tcp_port = port;
		m_flags = flags;
		if (m_flags & NETLIB_CLIENT_ENABLE_RECONNECT_ON_FIRST_CONNECT)
		{
			m_will_reconnect_if_disconnected = true; //首次连接也允许重连
		}
		else
		{
			m_will_reconnect_if_disconnected = false; //首次连接，默认不允许进入重连过程
		}

		connect_async();
	}
}

void NetLib_Client_Imp::ConnectAsync(const char* ip, uint16_t port, uint64_t _flags)
{	
	_connect_async(ip, 0, port, _flags);
}

void NetLib_Client_Imp::ConnectAsync(uint32_t ip, uint16_t port, uint64_t _flags)
{
	_connect_async(nullptr, ip, port, _flags);
}

void NetLib_Client_Imp::SendAsyncFailed(std::shared_ptr<NetLib_Client_Imp> keep_alive, const char* data, void* pHint)
{
	SendFailedHandler(data, pHint);
}

void NetLib_Client_Imp::SendCopyAsyncFailed(std::shared_ptr<NetLib_Client_Imp> keep_alive, const char* data_copy, void* pHint)
{
	if (!SendCopyFailedHandler(data_copy, pHint))
	{
		delete[] data_copy; //当Handler返回false的时候，把刚new出来的copy删掉
	}
}

void NetLib_Client_Imp::SendAsync(const char* data, void* pHint)
{	
	//原先这里会把Client shared一份，但后来觉得不必要，去掉了。仅当两个线程操作同一个shared_ptr对象，一个线程在SendAsync，另一个线程同时reset了这个shared_ptr导致析构，才会出问题
	client_mutex.lock();
	if (connected_session && connected_session->is_connected())
	{
		connected_session->send_async(data, false, pHint);
		client_mutex.unlock();
	}
	else
	{
		client_mutex.unlock();
		boostioservice.post(boost::bind(&NetLib_Client_Imp::SendAsyncFailed, this, shared_from_this(), data, pHint)); //Handler必须直接或间接的经过post才能调用，否则有可能导致死锁
	}
}

void NetLib_Client_Imp::SendCopyAsync(const char* data, void* pHint)
{	
	//原先这里会把Client shared一份，但后来觉得不必要，去掉了。仅当两个线程操作同一个shared_ptr对象，一个线程在SendCopyAsync，另一个线程同时reset了这个shared_ptr导致析构，才会出问题

	//不管怎样先拷贝一份。因为即使是进下面的else，这份copy也可能会被使用，因为有插入到failed_data_queue的可能
	char* data_copy = new char[*(uint32_t*)data];
	memcpy(data_copy, data, *(uint32_t*)data);

	client_mutex.lock();
	if (connected_session && connected_session->is_connected())
	{
		connected_session->send_async(data_copy, true, pHint);
		client_mutex.unlock();
	}
	else
	{
		client_mutex.unlock();
		boostioservice.post(boost::bind(&NetLib_Client_Imp::SendCopyAsyncFailed, this, shared_from_this(), data_copy, pHint)); //Handler必须直接或间接的经过post才能调用，否则有可能导致死锁
	}
}

void NetLib_Client_Imp::SendFailedHandler(const char* data, void* pHint)
{
	if (theDelegate->SendFailedHandler(shared_from_this(), data, pHint))
	{	
		//LOGEVENTL("NetLib_Trace", "SendFailed and Push into failed_data_queue. data: 0x" << std::hex << (std::size_t)data);

		boost::lock_guard<boost::recursive_mutex> lock(client_mutex);		

		failed_data_queue.push( netlib_packet(data, false, pHint));

		//LOGEVENTL("NetLib_Trace", "failed_data_queue.back().data: 0x" << std::hex << (std::size_t)failed_data_queue.back().data);
	}
}

bool NetLib_Client_Imp::SendCopyFailedHandler(const char* data_copy, void* pHint)
{
	if (theDelegate->SendCopyFailedHandler(shared_from_this(), data_copy, pHint))
	{			
		//LOGEVENTL("NetLib_Trace", "SendCopyFailed and Push into failed_data_queue. data_copy: 0x" << std::hex << (std::size_t)data_copy);
		boost::lock_guard<boost::recursive_mutex> lock(client_mutex);
		failed_data_queue.push( netlib_packet(data_copy, true, pHint));
		return true;
	}
	else
	{
		return false;
	}
}

//在连接前用
void NetLib_Client_Imp::DisableReconnect()
{
	boost::system::error_code ignored_error;

	boost::lock_guard<boost::recursive_mutex> lock(client_mutex);

	reconnect_retry_timer.cancel(ignored_error);
	m_enable_reconnect = false;
	m_will_reconnect_if_disconnected = false;	
	m_max_continuous_retries = 0;
}

//EnableReconnect方法要在连接前用，连上之后再设置就晚了
void NetLib_Client_Imp::EnableReconnect(int reconnect_interval_ms, int max_continuous_retries)
{
	boost::lock_guard<boost::recursive_mutex> lock(client_mutex);
	m_enable_reconnect = true;
	m_reconnect_interval_ms = reconnect_interval_ms;
	m_max_continuous_retries = max_continuous_retries;
}

boost::asio::io_service* NetLib_Client_Imp::GetWorkIoService()
{
	return &boostioservice;
}

void NetLib_Client_Imp::SetKeepAliveIntervalSeconds(int keepalive_interval_seconds)
{
	m_keepalive_interval_seconds = keepalive_interval_seconds;
}
#include "NetLibPlusClient.h"
#include "../include/ioservice_thread.h"
#include "Log/Log.h"

//暂不开启，否则打Log的时候太烦了
//#define NLP_LOGDETAIL

NetLibPlus_Client_Imp::NetLibPlus_Client_Imp(int ServerID) : m_ServerID(ServerID), m_RemoteServerPort(0)
	//之所以在构造函数时要传入delegate，是因为FailedDataReleaseHandler可能会被调用，即使这个连接从来都没有被调用过ResetClient
{
}

void NetLibPlus_Client_Imp::InitializeDelegate(std::shared_ptr<NetLibPlus_Client_Delegate> d)
{
	if (m_client)
	{
		LOGEVENTL("Warn", "Dangerous: InitializeDelegate should be called before the first ResetClient. ServerID: " << m_ServerID);
	}
	m_delegate = d;
}

NetLibPlus_Client_Imp::~NetLibPlus_Client_Imp()
{
	while (! failed_data_queue.empty())
	{
		netlib_packet& front_packet = failed_data_queue.front();
		if (front_packet.copy)
		{
			delete[] front_packet.data;
		}
		else
		{
			m_delegate->FailedDataReleaseHandler(front_packet.data, front_packet.pHint);
		}
		failed_data_queue.pop();
	}
}

void NetLibPlus_Client_Imp::ResetClient(const char* ip, uint16_t tcp_port, ioservice_thread* ioservice_th, uint64_t flags)
{	
	if (m_client)
	{
		m_client->Disconnect();

		LOGEVENTL("NetLib_Trace", "NetLibPlus_Client_Imp::ResetClient, disconnect m_client: " << log_::h((std::size_t)(m_client.get())));
	}

	m_client = NetLib_NewClient(shared_from_this(), ioservice_th);

	m_client->ConnectAsyncTCP(ip, tcp_port, flags | NETLIB_CLIENT_ENABLE_RECONNECT_ON_FIRST_CONNECT | NETLIB_CLIENT_FLAG_KEEP_ALIVE);

	m_RemoteServerIP = ip;
	m_RemoteServerPort = tcp_port;
}

void NetLibPlus_Client_Imp::ReleaseClient()
{
	if (m_client)
	{
		m_client->Disconnect();

		void* client_ptr = m_client.get(); //for debug

		m_client.reset(); //这里不reset，NetLib_Client是不会释放的，而NetLibPlus_Client_Imp作为NetLib_Client_Delegate，NetLib_Client里面有一份NetLibPlus_Client_Imp的引用，形成环了。因此必须手动reset。

		LOGEVENTL("NetLib_Trace", "NetLibPlus_Client_Imp::ReleaseClient, disconnect and reset m_client: " << log_::h((std::size_t)client_ptr));
	}
}

void NetLibPlus_Client_Imp::SendFinishHandler(NetLib_Client_ptr clientptr, char* data, void* pHint)
{
	if (pHint)
	{
		std::pair<int, void*>* pp = (std::pair<int, void*>*)pHint;
		if (pp->first == 0)
		{
			m_delegate->SendFinishHandler(shared_from_this(), data, pp->second);
		}
		else
		{
			delete[] data;
		}
		delete pp;
	}
	else //特例
	{
		m_delegate->SendFinishHandler(shared_from_this(), data, nullptr);
	}
}

void NetLibPlus_Client_Imp::RecvFinishHandler(NetLib_Client_ptr clientptr, char* data)
{
	m_delegate->RecvFinishHandler(shared_from_this(), data);
}

void NetLibPlus_Client_Imp::ResendFailedData()
{
	if (! failed_data_queue.empty())
	{
		std::string ip;
		int port;
		GetRemoteServerAddress(ip, port);
		LOGEVENTL("NLP_ResendFailedData", "Client (" << GetRemoteServerID() << ", " << ip << ":" << port << ") Resend failed data. queue size: " << failed_data_queue.size());

		do
		{
			netlib_packet& front_packet = failed_data_queue.front();
			if (front_packet.copy)
			{
				SendTheCopyAsync(front_packet.data);
			}
			else
			{
				SendAsync(front_packet.data, front_packet.pHint);
			}
			failed_data_queue.pop();
		} while (! failed_data_queue.empty());
	}
}

void NetLibPlus_Client_Imp::ConnectedHandler(NetLib_Client_ptr clientptr)
{
	std::string ip;
	int port;
	GetRemoteServerAddress(ip, port);
	LOGEVENTL("NLP_Connected", ip << ":" << port);

	m_delegate->ConnectedHandler(shared_from_this());
	ResendFailedData();
}

void NetLibPlus_Client_Imp::ReconnectedHandler(NetLib_Client_ptr clientptr)
{
	std::string ip;
	int port;
	GetRemoteServerAddress(ip, port);
	LOGEVENTL("NLP_Reconnected", ip << ":" << port);

	m_delegate->ReconnectedHandler(shared_from_this());
	ResendFailedData();
}

void NetLibPlus_Client_Imp::ReconnectFailedHandler(NetLib_Client_ptr clientptr, bool& will_continue_reconnect)
{
	/*
	std::string ip;
	int port;
	GetRemoteServerAddress(ip, port);
	LOGEVENTL("NLP_ReconnectFailed", _ln("clientptr") << hex((std::size_t)clientptr.get()) << ", " << ip << ":" << port); */
}

void NetLibPlus_Client_Imp::DisconnectedHandler(NetLib_Client_ptr clientptr, NetLib_Error error, int inner_error_code, bool& will_reconnect)
{
	std::string ip;
	int port;
	GetRemoteServerAddress(ip, port);
	LOGEVENTL("NLP_Disconnected", ip << ":" << port << ", " << _ln("error") << error << _ln("inner_error") << inner_error_code << _ln("will_reconnect") << will_reconnect);

	m_delegate->DisconnectedHandler(shared_from_this(), error, inner_error_code);
}

void NetLibPlus_Client_Imp::add_failed_data(const char* data, bool is_copy, void* pHint)
{	
#ifdef NLP_LOGDETAIL
	std::string IP;
	int port;
	GetRemoteServerAddress(IP, port);

	LOGEVENTL("NetLib_Trace", "Client(to " << GetRemoteServerID() << ", " << IP << ":" << port << "): Add failed data to queue, will resend as connected");
#endif
	failed_data_queue.push(netlib_packet(data, is_copy, pHint));
}

bool NetLibPlus_Client_Imp::SendFailedHandler(NetLib_Client_ptr clientptr, const char* data, void* pHint)
{
	if (pHint)
	{
		std::pair<int, void*>* pp = (std::pair<int, void*>*)pHint;
		if (pp->first == 0)
		{
			if (m_delegate->SendFailedHandler(shared_from_this(), data, pp->second))
			{
				add_failed_data(data, false, pp->second);
			}
		}
		else
		{
			if (m_delegate->SendCopyFailedHandler(shared_from_this(), data))
			{
				add_failed_data(data, true, pp->second);
			}
			else
			{
				delete[] data;
			}
		}
		delete pp;
	}
	else //特例
	{
		if (m_delegate->SendFailedHandler(shared_from_this(), data, nullptr))
		{
			add_failed_data(data, false, nullptr);		
		}
	}

	return false;
}

void NetLibPlus_Client_Imp::SendAsync(const char* data, void* pHint)
{	
#ifdef NLP_LOGDETAIL
	std::string IP;
	int port;
	GetRemoteServerAddress(IP, port);

	if (*(uint32_t*)data >= 12)
	{
		LOGEVENTL("NetLib_Trace", "Client(to " << GetRemoteServerID() << ", " << IP << ":" << port << "): SendAsync. CmdID: " << log_::h16(*(uint16_t*)(data + 10)));
	}
	else
	{
		LOGEVENTL("NetLib_Trace", "Client(to " << GetRemoteServerID() << ", " << IP << ":" << port << "): SendAsync. size: " << *(uint32_t*)data);
	}
#endif

	if (m_client)
	{
		if (pHint)
		{
			m_client->SendAsync(data, new std::pair<int, void*>(0, pHint));
		}
		else //对于pHint为空的情况特殊处理，因为这种情况应该是最多见的，避免多new个东西
		{
			m_client->SendAsync(data);
		}
	}
	else
	{
		add_failed_data(data, false, pHint);
	}
}

void NetLibPlus_Client_Imp::SendCopyAsync(const char* data)
{
#ifdef NLP_LOGDETAIL
	std::string IP;
	int port;
	GetRemoteServerAddress(IP, port);

	if (*(uint32_t*)data >= 12)
	{
		LOGEVENTL("NetLib_Trace", "Client(" << GetRemoteServerID() << ", " << IP << ":" << port << "): SendCopyAsync. CmdID: " << log_::h16(*(uint16_t*)(data + 10)));
	}
	else
	{
		LOGEVENTL("NetLib_Trace", "Client(" << GetRemoteServerID() << ", " << IP << ":" << port << "): SendCopyAsync. size: " << *(uint32_t*)data);
	}
#endif

	char* data_copy = new char[*(uint32_t*)data];
	memcpy(data_copy, data, *(uint32_t*)data);

	SendTheCopyAsync(data_copy);
}

void NetLibPlus_Client_Imp::SendTheCopyAsync(const char* data_copy)
{
	if (m_client)
	{
		m_client->SendAsync(data_copy, new std::pair<int, void*>(1, nullptr));
	}
	else
	{		
		add_failed_data(data_copy, true);
	}
}

int NetLibPlus_Client_Imp::GetRemoteServerID() const
{
	return m_ServerID;
}

void NetLibPlus_Client_Imp::GetRemoteServerAddress(std::string& IP, int& port)
{	
	IP = m_RemoteServerIP;
	port = m_RemoteServerPort;
}
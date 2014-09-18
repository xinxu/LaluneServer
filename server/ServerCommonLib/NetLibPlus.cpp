#include "NetLibPlus.h"
#include "NetLibPlusClient.h"
#include "../include/ioservice_thread.h"
#include <map>
#include <set>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>
#include "Log/Log.h"

typedef struct tag_delegate_and_flag
{
	std::shared_ptr<NetLibPlus_Client_Delegate> delegateptr;
	ioservice_thread* ioservice_th;
	uint64_t flags;
	tag_delegate_and_flag()
	{
	}
	tag_delegate_and_flag(std::shared_ptr<NetLibPlus_Client_Delegate>& d, ioservice_thread* it, uint64_t f) : delegateptr(d), ioservice_th(it), flags(f)
	{
	}
} delegate_and_flag;

boost::shared_mutex _NetLibPlus_mutex;

bool NetLibPlus_Clients_Shutdown = false;
delegate_and_flag global_delegate_info;
std::map<std::string, delegate_and_flag> init_info;
std::map<int, std::shared_ptr<NetLibPlus_Client_Imp> > clients;
std::map<int, NetLibPlus_ServerInfo > serverinfos;
std::map<std::string, std::set<int> > serverid_groupby_type;
std::map<std::string, int> serverid_iterator;
boost::mutex _serverid_iterator_mutex;
int MyServerID = 0;

delegate_and_flag select_delegate_and_flag(const std::string ServerType)
{
	auto it = init_info.find(ServerType);
	if (it != init_info.end())
	{
		return it->second;
	}
	else
	{
		return global_delegate_info;
	}
}

void try_start_a_client(const NetLibPlus_ServerInfo& ServerInfo) //���������ڵ���
{
	delegate_and_flag delegate_to_use = select_delegate_and_flag(ServerInfo.ServerType);
	
	if (delegate_to_use.delegateptr)
	{		
		auto it = clients.find(ServerInfo.ServerID);
		if (it == clients.end())
		{
			it = clients.insert(std::make_pair(ServerInfo.ServerID, std::make_shared<NetLibPlus_Client_Imp>(ServerInfo.ServerID))).first;
		}
		
		it->second->InitializeDelegate(delegate_to_use.delegateptr);
		it->second->ResetClient(ServerInfo.IP.c_str(), ServerInfo.port, delegate_to_use.ioservice_th, delegate_to_use.flags);
	}
	else
	{
		LOGEVENTL("Fatal", "try_start_a_client: NetLibPlus_InitializeClients havn't been called yet!! Type: " << ServerInfo.ServerType << ", ServerID: " << ServerInfo.ServerID);
	}
}

//�÷�����ControlServerConnector����
NETLIB_API void _NetLibPlus_UpdateServerInfo(int ServerID, const char* Ip, int Port, const char* ServerType)
{	
	boost::lock_guard<boost::shared_mutex> lock(_NetLibPlus_mutex); 

	if (NetLibPlus_Clients_Shutdown) return;

	std::map<int, NetLibPlus_ServerInfo >::iterator it_info = serverinfos.find(ServerID);

	if (it_info != serverinfos.end())
	{
		if (it_info->second.IP != Ip || it_info->second.port != Port || it_info->second.ServerType != ServerType) //Ip��˿ڱ仯�˲�������Ȼ�ܽ����һ�㶼�Ǳ仯��
		{
			it_info->second.IP = Ip;
			it_info->second.port = Port;
			it_info->second.ServerType = ServerType;
			try_start_a_client(it_info->second);
		}
	}
	else //�·�������Ϣ��
	{
		NetLibPlus_ServerInfo info;
		info.IP = Ip;
		info.port = Port;
		info.ServerType = ServerType;
		info.ServerID = ServerID;		
		serverinfos[ServerID] = info;

		try_start_a_client(info);
	}

	serverid_groupby_type[ServerType].insert(ServerID);
	{
		boost::lock_guard<boost::mutex> lock_guard(_serverid_iterator_mutex);
		if (serverid_iterator.count(ServerType) == 0)
		{
			serverid_iterator.insert(std::make_pair(ServerType, -1));
		}
	}

	LOGEVENTL("NetLib_Info", log_::n("ServerInfoSize") << serverinfos.size() << log_::n("ServerID") << ServerID << log_::n("IP") << Ip << log_::n("Port") << Port);
}

NETLIB_API void NetLibPlus_InitializeClients(std::shared_ptr<NetLibPlus_Client_Delegate> d, ioservice_thread* ioservice_th, uint64_t flags)
{	
	boost::lock_guard<boost::shared_mutex> lock(_NetLibPlus_mutex); 
	
	if (NetLibPlus_Clients_Shutdown) 
	{
		LOGEVENTL("Error", "Do anything after NetLibPlus_UnInitializeClients is no use.");
		return;
	}

	if (global_delegate_info.delegateptr) //�Ѿ���ֵ�ˣ��Ͳ��ٸ���
	{
		LOGEVENTL("Warn", "global_delegate has value, ignore the new one");
		return;
	}

	global_delegate_info.delegateptr = d;
	global_delegate_info.ioservice_th = ioservice_th;
	global_delegate_info.flags = flags;
}

NETLIB_API void NetLibPlus_InitializeClients(const char* ServerType, std::shared_ptr<NetLibPlus_Client_Delegate> d, ioservice_thread* ioservice_th, uint64_t flags)
{
	boost::lock_guard<boost::shared_mutex> lock(_NetLibPlus_mutex); 

	if (NetLibPlus_Clients_Shutdown) 
	{
		LOGEVENTL("Error", "Do anything after NetLibPlus_UnInitializeClients is no use.");
		return;
	}

	if (init_info.count(ServerType))
	{
		LOGEVENTL("Warn", "init_info[" << ServerType << "] has value, ignore the new one");
		return;
	}

	init_info[ServerType] = delegate_and_flag(d, ioservice_th, flags);
}

NETLIB_API std::shared_ptr<NetLibPlus_Client> NetLibPlus_getClient(int ServerID) //������shared_ptr������ֻҪget������һ����Ч��ֻ�ǿ�������һ�����Ӷ��ˣ����޷����⡣
{	
	boost::shared_lock<boost::shared_mutex> lock(_NetLibPlus_mutex); 

	if (NetLibPlus_Clients_Shutdown) 
	{
		LOGEVENTL("Error", "NetLibPlus_getClient: Do anything after NetLibPlus_UnInitializeClients is no use.");
		return std::shared_ptr<NetLibPlus_Client> ();
	}
	
	if (clients.count(ServerID) == 0) 
	{
		LOGEVENTL("NetLib_Debug", "NetLibPlus_getClient: ServerID " << ServerID << " not found in map `clients`, construct a new NetLibPlus_Client(without inner Client).");
		auto it = clients.insert(std::make_pair(ServerID, std::make_shared<NetLibPlus_Client_Imp>(ServerID))).first;

		if (global_delegate_info.delegateptr)
		{
			it->second->InitializeDelegate(global_delegate_info.delegateptr);
		}
		else
		{
			LOGEVENTL("Warn", "global_delegate not found when get a Client with ServerID " << ServerID << " but unknown ServerType. use Default Delegate.");
			it->second->InitializeDelegate(std::make_shared<NetLibPlus_Client_Delegate>());
		}
	}
	
	return clients[ServerID];
}

NETLIB_API std::shared_ptr<NetLibPlus_Client> NetLibPlus_get_first_Client(const char* ServerType)
{
	boost::shared_lock<boost::shared_mutex> lock(_NetLibPlus_mutex); 

	std::shared_ptr<NetLibPlus_Client> ret;

	if (NetLibPlus_Clients_Shutdown)
	{
		LOGEVENTL("Error", "NetLibPlus_get_first_Client: Do anything after NetLibPlus_UnInitializeClients is no use.");
	}
	else
	{
		auto it_group = serverid_groupby_type.find(ServerType);
		if (it_group == serverid_groupby_type.end())
		{
			LOGEVENTL("Warn", "NetLibPlus_get_first_Client: Type " << ServerType << " has no servers ");
		}
		else
		{
			auto it_id = it_group->second.begin();
			if (it_id != it_group->second.end())
			{
				auto it_client = clients.find(*it_id);
				if (it_client == clients.end())
				{
					LOGEVENTL("Fatal", "UNEXPECTED. ServerID " << *it_id << " exist in serverid_groupby_type, but not exist in clients");
				}
				else
				{
					ret = it_client->second;
				}
			}
			else
			{
				LOGEVENTL("Fatal", "UNEXPECTED. serverid_groupby_type(" << ServerType << ") is an empty set.");
			}
		}
	}

	return ret;
}

NETLIB_API std::shared_ptr<NetLibPlus_Client> NetLibPlus_get_next_Client(const char* ServerType)
{
	boost::shared_lock<boost::shared_mutex> lock(_NetLibPlus_mutex); 

	std::shared_ptr<NetLibPlus_Client> ret;

	if (NetLibPlus_Clients_Shutdown)
	{
		LOGEVENTL("Error", "NetLibPlus_get_next_Client: Do anything after NetLibPlus_UnInitializeClients is no use.");
	}
	else
	{
		auto it_group = serverid_groupby_type.find(ServerType);
		if (it_group == serverid_groupby_type.end())
		{
			LOGEVENTL("Warn", "NetLibPlus_get_next_Client: Type " << ServerType << " has no servers ");
		}
		else if (it_group->second.empty())
		{
			LOGEVENTL("Fatal", "UNEXPECTED. serverid_groupby_type(" << ServerType << ") is an empty set.");
		}
		else
		{
			boost::lock_guard<boost::mutex> lock_guard(_serverid_iterator_mutex);
			auto it_id_iterator = serverid_iterator.find(ServerType);
			if (it_id_iterator == serverid_iterator.end())
			{
				LOGEVENTL("Fatal", "UNEXPECTED. ServerType " << ServerType << " exist in serverid_groupby_type, but not exist in serverid_iterator.");
			}
			else
			{
				auto it_id = it_group->second.upper_bound(it_id_iterator->second);
				if (it_id == it_group->second.end())
				{
					//����һ�ܣ��ص�ͷ��
					it_id = it_group->second.begin();
				}

				auto it_client = clients.find(*it_id);
				if (it_client == clients.end())
				{
					LOGEVENTL("Fatal", "UNEXPECTED. ServerID " << *it_id << " exist in serverid_groupby_type, but not exist in clients");
				}
				else
				{
					ret = it_client->second;
				}
				it_id_iterator->second = *it_id;
			}
		}
	}

	return ret;
}

#ifdef _STATIC_NETLIB_

std::shared_ptr<NetLibPlus_Clients> NetLibPlus_getClients(const char* ServerType)
{
	std::shared_ptr<NetLibPlus_Clients> ret = std::make_shared<NetLibPlus_Clients> ();

	boost::shared_lock<boost::shared_mutex> lock(_NetLibPlus_mutex); 

	if (NetLibPlus_Clients_Shutdown)
	{
		LOGEVENTL("Error", "NetLibPlus_getClients: Do anything after NetLibPlus_UnInitializeClients is no use.");
	}
	else
	{
		auto it_group = serverid_groupby_type.find(ServerType);
		if (it_group == serverid_groupby_type.end())
		{
			LOGEVENTL("Warn", "NetLibPlus_getClients: Type " << ServerType << " has no servers yet.");
		}
		else
		{
			for (auto it_id = it_group->second.begin(); it_id != it_group->second.end(); ++it_id)
			{
				auto it_client = clients.find(*it_id);
				if (it_client == clients.end())
				{
					LOGEVENTL("Fatal", "UNEXPECTED. ServerID " << *it_id << " exist in serverid_groupby_type, but not exist in clients");
				}
				else
				{
					ret->push_back(it_client->second);
				}
			}
		}
	}

	return ret;
}

std::map<int, NetLibPlus_ServerInfo> NetLibPlus_getClientsInfo(const char* ServerType)
{
	std::map<int, NetLibPlus_ServerInfo> ret;

	boost::shared_lock<boost::shared_mutex> lock(_NetLibPlus_mutex); 

	if (NetLibPlus_Clients_Shutdown)
	{
		LOGEVENTL("Error", "NetLibPlus_getClientsInfo: Do anything after NetLibPlus_UnInitializeClients is no use.");
	}
	else
	{
		auto it_group = serverid_groupby_type.find(ServerType);
		if (it_group == serverid_groupby_type.end())
		{
			LOGEVENTL("Warn", "NetLibPlus_getClientsInfo: Type " << ServerType << " has no servers yet.");
		}
		else
		{
			for (auto it_id = it_group->second.begin(); it_id != it_group->second.end(); ++it_id)
			{
				auto it_info = serverinfos.find(*it_id);
				if (it_info == serverinfos.end())
				{
					LOGEVENTL("Fatal", "UNEXPECTED. ServerID " << *it_id << " exist in serverid_groupby_type, but not exist in serverinfos");
				}
				else
				{
					ret.insert(*it_info);
				}
			}
		}
	}

	return ret;
}

void NetLibPlus_Clients::SendCopyAsync(const char* data)
{
	for (auto it_client = begin(); it_client != end(); ++it_client)
	{
		(*it_client)->SendCopyAsync(data);
	}
}

#endif

NETLIB_API void NetLibPlus_UnInitializeClients(const char* ServerType)
{	
	boost::lock_guard<boost::shared_mutex> lock(_NetLibPlus_mutex); 

	if (NetLibPlus_Clients_Shutdown) return; //����NetLibPlus_UnInitializeClients���ö�Σ���ֻ�е�һ��ʵ����Ч

	auto it_idgroup = serverid_groupby_type.find(ServerType);
	if (it_idgroup != serverid_groupby_type.end())
	{
		for (auto it_id = it_idgroup->second.begin(); it_id != it_idgroup->second.end(); ++it_id)
		{
			auto it_client = clients.find(*it_id);
			if (it_client == clients.end())
			{
				LOGEVENTL("Fatal", "When NetLibPlus_UnInitializeClients(" << ServerType << "), client to " << *it_id << " exist in serverid_groupby_type, but not exist in map clients");
			}
			else
			{
				it_client->second->ReleaseClient(); //ֻ�ǶϿ����ӣ��ݲ����ͷŶ�������飬����з�ʧ�ܵ����ݻᱣ��������������IP�˿ڱ仯�˻��������
			}
		}
	}
}

NETLIB_API void NetLibPlus_UnInitializeClients()
{	
	boost::lock_guard<boost::shared_mutex> lock(_NetLibPlus_mutex); 

	if (NetLibPlus_Clients_Shutdown) return; //����NetLibPlus_UnInitializeClients���ö�Σ���ֻ�е�һ��ʵ����Ч

	NetLibPlus_Clients_Shutdown = true; //NetLibPlus_Clients_Shutdown����Ϊtrue�󣬾���ζ��clients�е����ݲ������ٱ�����

	for (auto it = clients.begin(); it != clients.end(); ++it)
	{
		it->second->ReleaseClient();
	}
	clients.clear();

	init_info.clear();

	global_delegate_info.delegateptr.reset();
}

NETLIB_API void NetLibPlus_DisableClients()
{
	NetLibPlus_UnInitializeClients();
}
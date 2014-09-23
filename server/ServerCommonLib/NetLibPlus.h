#pragma once

#include "NetLib/NetLib.h"
#include <memory>
#include <stdint.h>
#include <string>
#include "NetLib/NetLib_Error.h"

void _initialize_thread(class ioservice_thread* thread);

//该方法通常由ServerCommonLib内部调用，也可以自行调用
void _NetLibPlus_UpdateServerInfo(int ServerID, const char* Ip, int Port, int ServerType);

class NetLibPlus_Client
{
public:
	virtual void SendAsync(const char* data, void* pHint = nullptr) = 0;
	virtual void SendCopyAsync(const char* data) = 0;
	virtual int GetRemoteServerID() const = 0; 
	virtual void GetRemoteServerAddress(std::string& IP, int& port) = 0;
};

class NetLibPlus_Client_Delegate
{
public:	
	virtual void RecvFinishHandler(std::shared_ptr<NetLibPlus_Client> clientptr, char* data) {}

	virtual void ConnectedHandler(std::shared_ptr<NetLibPlus_Client> clientptr) {}
	virtual void ReconnectedHandler(std::shared_ptr<NetLibPlus_Client> clientptr) {}
	virtual void DisconnectedHandler(std::shared_ptr<NetLibPlus_Client> clientptr, NetLib_Error error, int inner_error_code) {} //以上三个主要作通知用，不建议在里面放逻辑代码
	//DisconnectedHandler触发后clientptr内的指针值也不会变，用的是同一个

	virtual void SendFinishHandler(std::shared_ptr<NetLibPlus_Client> clientptr, char* data, void* pHint) { delete[] data; }

	virtual bool SendFailedHandler(std::shared_ptr<NetLibPlus_Client> clientptr, const char* data, void* pHint) { return true; } //返回true表示由内部重发，返回false表示内部不重发
	virtual bool SendCopyFailedHandler(std::shared_ptr<NetLibPlus_Client> clientptr, const char* data) { return true; } //返回true表示由内部重发，返回false表示内部不重发

	virtual void FailedDataReleaseHandler(const char* data, void* pHint) { delete[] data; } //到这儿的概率很低。仅发生在：当Client即将释放时，失败队列里有数据，且这数据不是copy出来的。
	//FailedDataReleaseHandler方法是在析构函数内被直接调用，方法内进锁要慎重
};

//返回值保证不是空shared_ptr。如果该ServerID对应的信息还没有获取到，那么发的包都会存在失败队列里；ServerID对应的信息改变了，用同一个Client也能照常发
std::shared_ptr<NetLibPlus_Client> NetLibPlus_getClient(int ServerID);

//这两个方法可能会用不上，或者挪到Gateway里去
/*
//返回值可能为空shared_ptr。本方法不受get_first_Client方法的影响。第一次调用get_next_Client会返回第一个Client，遍历完一遍又会从第一个开始
std::shared_ptr<NetLibPlus_Client> NetLibPlus_get_next_Client(const char* ServerType);
*/

typedef struct tagNetLibPlus_ServerInfo
{
	std::string IP;
	int port;
	int ServerType;
	int ServerID;	
} NetLibPlus_ServerInfo;

#include <vector>

class NetLibPlus_Clients : public std::vector<std::shared_ptr<NetLibPlus_Client> >
{
public:
	void SendCopyAsync(const char* data);
};

std::shared_ptr<NetLibPlus_Clients> NetLibPlus_getClients(int ServerType);

#include <map>

std::map<int, NetLibPlus_ServerInfo> NetLibPlus_getClientsInfo(int ServerType);

void NetLibPlus_InitializeClients(int ServerType, std::shared_ptr<NetLibPlus_Client_Delegate> d, uint64_t flags = 0); //设置某个类型服务器的Delegate

void NetLibPlus_InitializeClients(std::shared_ptr<NetLibPlus_Client_Delegate> d, uint64_t flags = 0); //设置全局的Delegate，不覆盖各类型的Delegate

template <typename Delegate>
void NetLibPlus_InitializeClients(int ServerType, uint64_t flags = 0) //设置某个类型服务器的Delegate
{
	NetLibPlus_InitializeClients(ServerType, std::make_shared<Delegate>(), flags);
}

template <typename Delegate>
void NetLibPlus_InitializeClients(uint64_t flags = 0) //设置全局的Delegate，不覆盖各类型的Delegate
{
	NetLibPlus_InitializeClients(std::make_shared<Delegate>(), flags);
}

void NetLibPlus_UnInitializeClients(int ServerType); //将这一类的连接关闭。通常是一些一次性的连接，获取信息后就不再用了。但是如果这一类连接的某些连接的地址发生变更，则又会再次连上。

void NetLibPlus_UnInitializeClients();

void NetLibPlus_DisableClients(); //如果调用了Disable，表示不使用NetLibPlus的Clients，同时程序结束时不需要调用NetLibPlus_UnInitializeClients


//==============================================================



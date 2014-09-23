// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the NETLIB_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// NETLIB_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifndef _NETLIB_H_
#define _NETLIB_H_

#define _NETLIB_VERSION_ "1_02"

#ifndef _NOT_LINK_NETLIB_LIB_
#ifdef _DEBUG
#define _NETLIB_LIBNAME_ "NetLibd"
#else
#define _NETLIB_LIBNAME_ "NetLib"
#endif
#pragma comment(lib, _NETLIB_LIBNAME_)  //this is not cross-platform. you should write Makefile on linux.

#endif

#include <string>
namespace boost { namespace asio { class io_service; } }

//for windows:   Need VS2010(or above)
//for gnu linux: Need gcc4.6.3(or above)

#include "stdint.h"
#include "NetLib_Error.h"
#include <memory>


//=====NetLib_Flags=====
#define NETLIB_CLIENT_FLAG_KEEP_ALIVE						(0x00000001)	//定期发送size为4的包，时间间隔可以设置
#define NETLIB_FLAG_TCP_NODELAY								(0x00000002)
#define NETLIB_CLIENT_ENABLE_RECONNECT_ON_FIRST_CONNECT		(0x00000080)	//默认首次连接不自动重连。开了这个FLAG后首次连接也会自动重连
#define NETLIB_SERVER_LISTEN_KEEP_ALIVE_EVENT				(0x00000100)	//收到size为4的包会触发ReceiveKeepAliveHandler。不开这个flag的话只是不触发Handler，但也有KeepAlive效果。
																			//无论怎样size为4的包都不触发Server的ReceiveFinishHandler
//=====NetLib_Flags=====

void NetLib_Set_MaxPacketSize(unsigned int MaxPacketSize); //default: 512 * 1024
void NetLib_Set_Server_Recv_Buffer_Size(int Server_Recv_Buffer_Size); //default: 16 * 1024 * 1024
void NetLib_Set_Client_Recv_Buffer_Size(int Client_Recv_Buffer_Size); //default: 16 * 1024

#define NETLIBDataSize(A) (*(uint32_t*)(A))

class NetLib_Client_Interface
{
public:
	virtual void Disconnect() = 0; //多次调用Disconnect不会有问题
	virtual bool IsConnected() = 0; //不建议编写逻辑时过于依赖该方法的返回值，因为判断前后连接状态可能发生改变。

	//only ip is supported, domain or hostname is not supported. Resolver not implemented
	virtual void ConnectAsync(const char* ip, uint16_t port, uint64_t flags = 0) = 0;
	virtual void ConnectAsync(uint32_t ip, uint16_t port, uint64_t flags = 0) = 0;

	//'data' must be retained until SendFinishHandler has been called
	//data的前4个字节是长度
	virtual void SendAsync(const char* data, void* pHint = nullptr) = 0;

	virtual void SendCopyAsync(const char* data, void* pHint = nullptr) = 0; //you can release `data` after SendCopyAsync been executed. 7.19修改: 也增加了pHint参数，且发送结束会触发SendCopyFinishHandler

	//以下两个方法应在Connect之前调用
	virtual void DisableReconnect() = 0;
	virtual void EnableReconnect(int reconnect_interval_ms = 5000, int max_continuous_retries = -1) = 0; 
	//重连默认为Enable
	//reconnect_interval_ms表示每次重连失败后等待多少毫秒后再次尝试重连。第一次断线时会立即重连。
	//max_continuous_retries表示重连失败多少次后不再重连
	//只有连着的连接断线了才会重连，没连上的连接不会反复尝试连

	virtual void ResetFailedData() = 0;	
	
	virtual class boost::asio::io_service* GetWorkIoService() = 0;

	virtual void SetKeepAliveIntervalSeconds(int keepalive_interval_seconds = 240) = 0;
};

#define NetLib_Client_ptr std::shared_ptr<NetLib_Client_Interface>

class NetLib_Client_Delegate
{
public:
	virtual void ConnectedHandler(NetLib_Client_ptr clientptr) {} //在连接前就发数据也是可以的，因为默认连上后会自动发送之前累积的数据

	//因为底层实现不同，SendFinish的定义对于TCP和UDP是不同的。UDP会确认对方收到才触发SendFinish。
	virtual void SendFinishHandler(NetLib_Client_ptr clientptr, char* data, void* pHint) { delete[] data; } //如果不是用new[]申请的内存，请重写此方法
	virtual void SendCopyFinishHandler(NetLib_Client_ptr clientptr, char* data, void* pHint) {}

	//RecvFinishHandler一旦返回，data就会被释放，所以必要时请对data进行拷贝
	virtual void RecvFinishHandler(NetLib_Client_ptr clientptr, char* data) {} //通常都需要重写此方法，除非你只发数据完全不收数据

	//刚发生断线总是会进这里，无论是否enable了reconnect。要等DisconnectedHandler执行完毕才会开始重连过程。
	virtual void DisconnectedHandler(NetLib_Client_ptr clientptr, NetLib_Error error, int inner_error_code, bool& will_reconnect) {};	
	//will_reconnect和下面ReconnectFailedHandler的参数意义相同，可读可写。

	//注意: 在UDP协议下，发送失败不能保证100%是发送失败，也有可能对方收到了包，只是本地还没来得及收到确认，就断线了。
	virtual bool SendFailedHandler(NetLib_Client_ptr clientptr, const char* data, void* pHint) { return true; } //当返回true时，表示等连成功了重发该包；当返回false时，data需要被释放
	virtual bool SendCopyFailedHandler(NetLib_Client_ptr clientptr, const char* data_copy, void* pHint) { return true; } //当返回true时，表示等连成功了重发该包；无论如何，data_copy都不需要被释放
	virtual void FailedDataReleaseHandler(const char* data, void* pHint) { delete[] data; } //到这儿的概率很低。仅发生在：当Client即将释放时，失败队列里有数据，且这数据不是copy出来的
	virtual void FailedDataReleaseHandler(NetLib_Client_ptr clientptr, const char* data, void* pHint) { delete[] data; } //仅发生在Client调用了ResetFailedData()时
	
	virtual void ReconnectedHandler(NetLib_Client_ptr clientptr) {} //重连成功
	virtual void ReconnectFailedHandler(NetLib_Client_ptr clientptr, bool& will_continue_reconnect) {}  
	//本次重连失败。
	//will_continue_reconnect为true则表示将继续重连，否则说明重连次数已达到或超过max_continuous_retries。
	//用户也可以手动将will_continue_reconnect置为false，表示放弃继续重连；
	//或者将本已false的will_continue_reconnect置为true, 表示再试一次。对于这种情况，下一次ReconnectFailedHandler触发时will_continue_reconnect依然会是false
	//对于主动断开连接或者DisableReconnect了的，无论will_continue_reconnect的值是什么，都不会进行重连
};

/*
NetLib_NewClient() returns a new intance of NetLib_Client_Imp:

class NetLib_Client_Imp Thread Safety:
	Distinct objects: Safe.
	Shared objects:	  Safe.

All the handlers share a single thread,
So long-time-consuming work is ok but not recommended to be called within the handler.
*/
NetLib_Client_ptr NetLib_NewClient(std::shared_ptr<NetLib_Client_Delegate> d);

//如果使用外部传入的ioservice，务必要保证在Client完全停止前ioservice不能被释放。
NetLib_Client_ptr NetLib_NewClient(std::shared_ptr<NetLib_Client_Delegate> d, class ioservice_thread* thread); //dll版本的话只能传nullptr，否则会有问题，因为ioservice_thread是复杂类型

template <typename ClientDelegate>
NetLib_Client_ptr NetLib_NewClient(ioservice_thread* ioservice = nullptr)
{
	return NetLib_NewClient(std::make_shared<ClientDelegate>(), ioservice);
}


//不一定需要调用本方法。但不调用的话可能会有些一次性的内存泄漏 -- 调用的话会阻塞，直到所有东西都释放。
//必须保证调用本方法前已经没有还处于连接状态的Client，否则会释放不掉。
//服务器端使用的话，要先调用CTRL_ReleaseConnection()和NetLibPlus_UnInitializeClients()
void NetLib_Clients_WaitForStop(); //不得在Handler中调用，调用则死锁。


//======= Server =========




class NetLib_ServerSession_Interface
{	
public:	
	//'data' must be retained until SendFinishHandler has been called
	virtual void SendAsync(const char* data, void* pHint = nullptr) = 0;
	virtual void SendCopyAsync(const char* data, void* pHint = nullptr) = 0; //7.19修改: 也增加了pHint参数，且发送结束会触发SendCopyFinishHandler
	virtual bool GetRemoteAddress(char* ip, uint16_t& port) = 0; //ip must have a length of at least 16
	virtual void Disconnect() = 0;

	virtual bool GetRemoteAddress(std::string& ip, uint16_t& port) = 0;
	virtual uint32_t GetRemoteIPu() = 0;
	virtual std::string GetRemoteIP() = 0;
	virtual std::string GetLocalAddress() = 0;
};

#define NetLib_ServerSession_ptr std::shared_ptr<NetLib_ServerSession_Interface>

class NetLib_ServerSession_Delegate
{
public:
	virtual void ConnectedHandler(NetLib_ServerSession_ptr sessionptr) {};

	virtual void SendFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data, void* pHint = nullptr) { delete[] data; } //如果不是用new[]申请的内存，请重写此方法
	virtual void SendCopyFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data, void* pHint = nullptr) {}

	virtual void RecvKeepAliveHandler(NetLib_ServerSession_ptr sessionptr) {};
	//'data' will be released just after RecvFinishHandler returns
	virtual void RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data) = 0;

	//下面两个方法的返回值没有意义
	virtual bool SendFailedHandler(NetLib_ServerSession_ptr clientptr, const char* data, void* pHint = nullptr) { delete[] data; return true; } //`data` should be deleted
	virtual bool SendCopyFailedHandler(NetLib_ServerSession_ptr clientptr, const char* data_copy, void* pHint) { return true; } //`data_copy` should not be deleted.

	virtual void DisconnectedHandler(NetLib_ServerSession_ptr sessionptr, NetLib_Error error, int inner_error_code) {};

	//virtual void RecvKeepAliveHandler(NetLib_ServerSession_ptr sessionptr) = 0;
};

class NetLib_Server_Interface
{
public:
	//已过时。相当于直接调用StartTCP
	virtual bool Start(int tcp_listen_port, int udp_listen_port, int work_thread_num = 1, int timeout_seconds = 300, uint64_t flags = 0) = 0;
	virtual bool StartTCP(int listen_port, int work_thread_num = 1, int timeout_seconds = 300, uint64_t flags = 0) = 0;
	//已过时，执行则什么也不发生
	virtual bool StartUDP(int listen_port, int work_thread_num = 1, int timeout_seconds = 300, uint64_t flags = 0) = 0;
	virtual void Stop() = 0;

	virtual class boost::asio::io_service* GetWorkIoService() = 0;

	virtual ~NetLib_Server_Interface() {}
};

#define NetLib_Server_ptr std::shared_ptr<NetLib_Server_Interface>

class NetLib_Server_Delegate
{
public:
	 virtual NetLib_ServerSession_Delegate* New_SessionDelegate() = 0; //factory function
	 virtual void Release_SessionDelegate(NetLib_ServerSession_Delegate* d)
	 {
		 delete d; //如果有多个Session共用Delegate的情况，那么请重写这个方法
	 }
	 
	 virtual void ErrorHandler(NetLib_Error error, int error_code)
	 {
		 //printf("server produced an error: %d, %d\n", error, error_code);
	 }
};

NetLib_Server_ptr NetLib_NewServer(std::shared_ptr<NetLib_Server_Delegate> d);

NetLib_Server_ptr NetLib_NewServer(std::shared_ptr<NetLib_ServerSession_Delegate> d);

NetLib_Server_ptr NetLib_NewServer(std::shared_ptr<NetLib_Server_Delegate> d, class ioservice_thread* thread); //dll版本的话只能传nullptr，否则会有问题，因为ioservice_thread是复杂类型

NetLib_Server_ptr NetLib_NewServer(std::shared_ptr<NetLib_ServerSession_Delegate> d, class ioservice_thread* thread);

template <typename SessionDelegate>
class NetLib_Server_Default_Delegate : public NetLib_Server_Delegate
{
public:
	NetLib_ServerSession_Delegate* New_SessionDelegate()
	{
		return new SessionDelegate();
	}
};

template <typename SessionDelegate>
NetLib_Server_ptr NetLib_NewServer(class ioservice_thread* thread = nullptr)
{
	return NetLib_NewServer(std::make_shared<NetLib_Server_Default_Delegate<SessionDelegate> > (), thread);
}

//不一定需要调用本方法。但不调用的话可能会有些一次性的内存泄漏 -- 调用的话会阻塞，直到和各Server相关的所有东西都释放。
//调用前必须保证Server都已经调用过Stop方法，且外部已经没有Server的shared_ptr的引用，否则会一直阻塞在那儿
void NetLib_Servers_WaitForStop(); //不得在Handler中调用，调用则死锁

/*
NetLib_NewServer() returns a new intance of NetLib_Server_Imp:

class NetLib_Server_Imp Thread Safety:
	Distinct objects: Safe.
	Shared objects:	  Safe.

class NetLib_ServerSession_Interface Thread Safety:
	Distinct objects: Safe.
	Shared objects:	  Safe.
*/

bool NetLib_CheckVersion(const char* version);

#define NETLIB_CHECK_VERSION NetLib_CheckVersion(_NETLIB_VERSION_)

#endif

#ifndef _NETLIB_SESSION_OWNER_H_
#define _NETLIB_SESSION_OWNER_H_

#include <memory>
#include "NetLib_Error.h"
#include <boost/thread/recursive_mutex.hpp>

class NetLib_Session_Owner //session usually maintain a share_ptr to session_owner
{
	friend class NetLib_Connected_TcpSession;
	friend class NetLib_Client_Imp;
protected:
	virtual boost::recursive_mutex& get_mutex() = 0;
	
	int m_pending_ops_count;

	void increase_pending_ops_count()  //该方法不进锁，由外部保证在锁内
	{
		m_pending_ops_count ++;
	} 
	virtual void decrease_pending_ops_count() {}
	virtual void handle_error(NetLib_Error error, int internal_error_code) = 0;

	//以下五个方法调用的时候保证了不在锁里，owner的实现那里直接调用上层的delegate就可以
	virtual void SendFinishHandler(char* data, void* pHint) = 0; //所有会被post执行的方法都要keep_alive一份本对象
	virtual void SendCopyFinishHandler(char* data, void* pHint) = 0; //SendCopyFinish方法本身不会被post，但他的上一层会被post执行。

	//'data' will be released just after RecvFinishHandler returns
	virtual void RecvFinishHandler(char* data) = 0;

	virtual void SendFailedHandler(const char* data, void* pHint) = 0;
	virtual bool SendCopyFailedHandler(const char* data_copy, void* pHint) = 0; //返回false则表示data_copy可以删了

public:
	NetLib_Session_Owner() : m_pending_ops_count(0) {}
};
#endif
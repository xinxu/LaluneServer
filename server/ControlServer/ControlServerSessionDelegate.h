#ifndef __CONTROL_SERVER_SESSION_DELEGATE_H_
#define __CONTROL_SERVER_SESSION_DELEGATE_H_

#include "NetLib/NetLib.h"

class ControlServerSessionDelegate : public NetLib_ServerSession_Delegate
{
public:
	void ConnectedHandler(NetLib_ServerSession_ptr sessionptr);
	void DisconnectedHandler(NetLib_ServerSession_ptr sessionptr);
	void RecvKeepAliveHandler(NetLib_ServerSession_ptr sessionptr);
	void RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data);
};

extern bool during_startup;

#endif
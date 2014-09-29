#ifndef __GATEWAY_USER_SESSION_DELEGATE_H_
#define __GATEWAY_USER_SESSION_DELEGATE_H_

#include "NetLib/NetLib.h"

class GatewayUserSessionDelegate : public NetLib_ServerSession_Delegate
{
public:
	void ConnectedHandler(NetLib_ServerSession_ptr sessionptr);

	void DisconnectedHandler(NetLib_ServerSession_ptr sessionptr);

	void RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data);
};

#endif
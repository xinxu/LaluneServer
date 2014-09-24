#ifndef __GATEWAY_SESSION_DELEGATE_H_
#define __GATEWAY_SESSION_DELEGATE_H_

#include "NetLib/NetLib.h"

class GatewaySessionDelegate : public NetLib_ServerSession_Delegate
{
public:
	void RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data);
};

#endif
#ifndef __USER_SESSION_DELEGATE_H_
#define __USER_SESSION_DELEGATE_H_

#include "NetLib/NetLib.h"

class UserSessionDelegate : public NetLib_ServerSession_Delegate
{
public:
	void ConnectedHandler(NetLib_ServerSession_ptr sessionptr);

	void DisconnectedHandler(NetLib_ServerSession_ptr sessionptr);

	void RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data);
};

#endif
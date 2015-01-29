#ifndef __USER_SESSION_DELEGATE_H_
#define __USER_SESSION_DELEGATE_H_

#include "NetLib/NetLib.h"
#include "ServerHeaderDef.h"
#include "AutoMatchServer.h"

class UserSessionDelegate : public NetLib_ServerSession_Delegate
{
public:
	//void ConnectedHandler(NetLib_ServerSession_ptr sessionptr);

	void DisconnectedHandler(NetLib_ServerSession_ptr sessionptr);

	BEGIN_HANDLER(NetLib_ServerSession_ptr)

	HANDLE_MSG_SESSION(boids::AUTO_MATCH_REQUEST, boids::MatchRequest, ams.MatchRequest)

	END_HANDLER(UserSessionDelegate)
};

#endif
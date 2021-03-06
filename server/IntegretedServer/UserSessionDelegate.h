#ifndef __USER_SESSION_DELEGATE_H_
#define __USER_SESSION_DELEGATE_H_

#include "NetLib/NetLib.h"
#include "ServerHeaderDef.h"
#include "AutoMatchServer.h"

class UserSessionDelegate : public NetLib_ServerSession_Delegate
{
public:
	//void ConnectedHandler(NetLib_ServerSession_ptr sessionptr);

	void DisconnectedHandler(NetLib_ServerSession_ptr sessionptr, NetLib_Error error, int inner_error_code);

	BEGIN_HANDLER(UserSession, NetLib_ServerSession_ptr)

	HANDLE_MSG_SESSION(boids::AUTO_MATCH_REQUEST, boids::MatchRequest, ams.MatchRequest)

	END_HANDLER(UserSession)
};

#endif
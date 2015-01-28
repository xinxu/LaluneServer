#include "UserSessionDelegate.h"
#include "Log/Log.h"
#include "ServerHeaderDef.h"
#include "boids.pb.h"
#include "Battle.pb.h"
#include "AutoMatchServer.h"

void UserSessionDelegate::RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
{
	LOGEVENTL("DEBUG", "Receive from user: " << log_::bytes_display(data, MSG_LENGTH(data)));

	BEGIN_SWITCH

	HANDLE_MSG(boids::MessageType::AUTO_MATCH_REQUEST, boids::MatchRequest, ams.MatchRequest)

	END_SWITCH("UserSessionDelegate")
}
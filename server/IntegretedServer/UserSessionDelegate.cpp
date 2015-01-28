#include "UserSessionDelegate.h"
#include "MessageTypeDef.h"
#include "Log/Log.h"
#include "ServerHeaderDef.h"
#include "Battle.pb.h"
#include "AutoMatchServer.h"

void UserSessionDelegate::RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
{
	if (MSG_LENGTH(data) >= MSG_HEADER_BASE_SIZE)
	{
		LOGEVENTL("DEBUG", "Receive from user: " << log_::bytes_display(data, MSG_LENGTH(data)));

		switch (MSG_TYPE(data))
		{
			HANDLE_MSG(MSG_TYPE_AUTOMATCH_MATCH_REQUEST, boids::MatchRequest, ams.MatchRequest);
		default:
			UNRECOGNIZE("UserSessionDelegate", MSG_TYPE(data))
		}
	}
	else
	{
		MSG_TOO_SHORT("UserSessionDelegate", MSG_LENGTH(data));
	}
}
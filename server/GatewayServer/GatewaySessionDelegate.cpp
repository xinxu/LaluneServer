#include "GatewaySessionDelegate.h"
#include "ServerCommon.h"
#include "GatewayServer.h"
#include "Log/Log.h"

void GatewaySessionDelegate::RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
{
	if (SERVER_MSG_LENGTH(data) >= SERVER_MSG_HEADER_BASE_SIZE)
	{
		
	}
	else
	{
		LOGEVENTL("ERROR", "msg too short");
	}
}
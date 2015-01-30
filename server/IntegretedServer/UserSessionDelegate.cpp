#include "UserSessionDelegate.h"

void UserSessionDelegate::DisconnectedHandler(NetLib_ServerSession_ptr sessionptr, NetLib_Error error, int inner_error_code)
{
	//到时候要处理的逻辑应该更多，不只是取消匹配
	ams.MatchCancel(sessionptr);
}
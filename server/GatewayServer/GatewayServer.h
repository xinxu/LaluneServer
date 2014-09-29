#pragma once

#include "NetLib/NetLib.h"

int GetTmpUserId();
void ReleaseTmpUserId(int _id);
void UpdateUserSession(int uid, NetLib_ServerSession_ptr session);
void UserSessionLeft(int uid);
NetLib_ServerSession_ptr GetSessionById(int uid);
#ifndef __VERSION_SERVER_SESSION_DELEGATE_H_
#define __VERSION_SERVERY_SESSION_DELEGATE_H_

#include "NetLib/NetLib.h"

class VersionServerSessionDelegate : public NetLib_ServerSession_Delegate
{
public:
	void RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data);
};

#endif
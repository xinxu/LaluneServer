#ifndef __ACCOUNT_SERVER_SESSION_DELEGATE_H_
#define __ACCOUNT_SERVER_SESSION_DELEGATE_H_

#include "../NetLib/NetLib.h"

#include "mongo/client/dbclient.h"

class AccountServerSession : public NetLib_ServerSession_Delegate
{
public:
	void RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data);
};

#endif
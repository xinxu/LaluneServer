#pragma
#include <boost/asio.hpp>
#include "include/ioservice_thread.h"
#include "Log/Log.h"
#include "NetLib/NetLib.h"
#include "memory.h"
#include "string.h"
#include <google/protobuf/stubs/common.h>
#include "../../LaluneCommon/include/MessageTypeDef.h"
#include "ServerCommonLib/ServerCommon.h"
#include "Battle.pb.h"
class MatchAndCombatCommunicate :public NetLibPlus_Client_Delegate
{
	void RecvFinishHandler(std::shared_ptr<NetLibPlus_Client> clientptr, char* data);
	void ConnectedHandler(std::shared_ptr<NetLibPlus_Client> clientptr);
	void ReconnectedHandler(std::shared_ptr<NetLibPlus_Client> clientptr);

};
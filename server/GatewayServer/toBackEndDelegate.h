#pragma once

#include "NetLibPlus.h"

class toBackEndDelegate : public NetLibPlus_Client_Delegate
{
	void RecvFinishHandler(std::shared_ptr<NetLibPlus_Client> clientptr, char* data);
};
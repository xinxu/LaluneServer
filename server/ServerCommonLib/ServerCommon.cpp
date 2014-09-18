#include "ServerCommon.h"
#include "NetLib.h"

CommonLibDelegate* __commonlib_delegate;



void InitializeCommonLib(CommonLibDelegate* d)
{
	__commonlib_delegate = d;
}

void ReportLoad(float load_factor)
{

}

void UpdateCorrespondingServer(uint64_t user_id, const CorrespondingServer& header_ex)
{

}
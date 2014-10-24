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
class MatchAndUserCommunicate : public NetLib_ServerSession_Delegate
{
public:
	void RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data);
	template<typename P>
	void SendMsg(NetLib_ServerSession_ptr &sessionptr,uint32_t msg_type, P& proto)
	{
		int proto_size = proto.ByteSize();

		char* send_buf = new char[MSG_HEADER_BASE_SIZE + proto_size];
		MSG_LENGTH(send_buf) = MSG_HEADER_BASE_SIZE + proto_size;		// 数据包的字节数（含msghead）
		MSG_TYPE(send_buf) = msg_type;
		memset(send_buf + MSG_AFTER_TYPE_POS, 0, MSG_HEADER_BASE_SIZE - MSG_AFTER_TYPE_POS);

		proto.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)MSG_DATA(send_buf));

		sessionptr->SendAsync(send_buf);
	}

private:

};

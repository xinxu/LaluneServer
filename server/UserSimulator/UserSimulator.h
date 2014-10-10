#pragma once

#include "NetLib/NetLib.h"
#include "MessageTypeDef.h"

class UserSimulator : public NetLib_Client_Delegate, public std::enable_shared_from_this<UserSimulator>
{
protected:
	NetLib_Client_ptr _client;

	void ConnectedHandler(NetLib_Client_ptr clientptr);

	void RecvFinishHandler(NetLib_Client_ptr clientptr, char* data);
	
	template<typename P>
	bool ParseMsg(char* data, P& proto) //包头无UserID的版本
	{
		return proto.ParseFromArray(MSG_DATA(data), MSG_DATA_LEN(data));
	}

	template<typename P>
	void SendMsg(uint32_t msg_type, P& proto)
	{
		int proto_size = proto.ByteSize();

		char* send_buf = new char[MSG_HEADER_BASE_SIZE + proto_size];
		MSG_LENGTH(send_buf) = MSG_HEADER_BASE_SIZE + proto_size;		// 数据包的字节数（含msghead）
		MSG_TYPE(send_buf) = msg_type;
		memset(send_buf + MSG_AFTER_TYPE_POS, 0, MSG_HEADER_BASE_SIZE - MSG_AFTER_TYPE_POS);

		proto.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)MSG_DATA(send_buf));

		_client->SendAsync(send_buf);
	}

public:
	void Connect(const std::string& ip, int port);
	void Register();
	void Version();
};
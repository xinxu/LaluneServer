#pragma once

#include "NetLib/NetLib.h"
#include "MessageTypeDef.h"
#include "NetLib/NetLib.h"
#include "include/ioservice_thread.h"
#include "MessageTypeDef.h"
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "Log/Log.h"
#include <google/protobuf/stubs/common.h>
#include "Battle.pb.h"
#include "include/ptime2.h"
#include <map>
#include <vector>
#include <boost/bind.hpp>
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
	void Version(const std::string& version_name);
	void Combat(int id);
	void StartupTimer(const boost::system::error_code& error);
	void ReconnectedHandler(NetLib_Client_ptr clientptr);
	UserSimulator();
private:
	boost::asio::deadline_timer timer1;
	unsigned int p_id;
	//std::vector<uint64_t> ping[10];
#define PING_RANGE_COUNT (30)
//	int ping[PING_RANGE_COUNT + 1];
//	std::string section[PING_RANGE_COUNT + 1];
//	uint64_t time_begin, time_now;
};
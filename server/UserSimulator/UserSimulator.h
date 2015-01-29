#pragma once

#include "NetLib/NetLib.h"
#include "NetLib/NetLib.h"
#include "include/ioservice_thread.h"
#include "ServerHeaderDef.h"
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

	BEGIN_HANDLER(UserSimulator, NetLib_Client_ptr)
		HANDLE_MSG(boids::AUTO_MATCH_RESPONSE, boids::MatchResponse, [](const boids::MatchResponse& res) {
			LOGEVENTL("MATCH", _ln("ret_value") << res.ret_value() << _ln("ret_info") << res.ret_info()
				<< _ln("Ip") << res.game_server_ip() << _ln("Port") << res.game_server_port() << _ln("game_id") << res.game_uuid());
		});
	END_HANDLER(UserSimulator)

	template<typename P>
	void SendMsg(boids::MessageType msg_type, P& proto)
	{
		boids::BoidsMessageHeader proto_with_header;
		proto_with_header.set_type(msg_type);
		proto.SerializeToString(proto_with_header.mutable_data());

		int proto_size = proto_with_header.ByteSize();

		char* send_buf = new char[MSG_HEADER_LEN + proto_size];
		MSG_LENGTH(send_buf) = MSG_HEADER_LEN + proto_size;

		proto_with_header.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)MSG_DATA(send_buf));
		_client->SendAsync(send_buf);
	}

public:
	void Connect(const std::string& ip, int port);
	void Register();
	void Version(const std::string& version_name);
	void Combat(int id);
	void StartupTimer(const boost::system::error_code& error);
	void ReconnectedHandler(NetLib_Client_ptr clientptr);
	void UserSimulator::Match();
	/*UserSimulator();*/
private:
	/*boost::asio::deadline_timer timer1;*/
	unsigned int p_id;
	//std::vector<uint64_t> ping[10];
#define PING_RANGE_COUNT (30)
//	int ping[PING_RANGE_COUNT + 1];
//	std::string section[PING_RANGE_COUNT + 1];
//	uint64_t time_begin, time_now;
};
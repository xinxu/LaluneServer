#ifndef __SERVER_COMMON_H
#define __SERVER_COMMON_H

//无锁版

#include <cstdint>
#include <map>

//服务端包头定义

#define SERVER_MSG_LENGTH(d) (*(uint32_t*)(d))
#define SERVER_MSG_TYPE(d) (*(uint32_t*)((d) + 4))
#define SERVER_MSG_OPERATION_ID(d) (*(uint32_t*))((d) + 8)
#define SERVER_MSG_ERROR(d) (*(uint8_t*)((d) + 12)) //后端服务给前端服务返回的错误。如果有错误，那么默认后面的Protobuf就没有了，除非另有约定
#define SERVER_MSG_HEADER_EX_LEN(d) (*(uint8_t*)((d) + 13))
#define SERVER_MSG_RESERVED(d) (*(uint16_t*)((d) + 14))
#define SERVER_MSG_HEADER_BASE_SIZE (16)
#define SERVER_MSG_HEADER_EX(d) (*(uint16_t*)((d) + SERVER_MSG_HEADER_BASE_SIZE))
#define SERVER_MSG_DATA(d) (*(uint16_t*)((d) + SERVER_MSG_HEADER_BASE_SIZE + MSG_HEADER_EX_LEN(d)))

#include "commonlib/HeaderEx.pb.h"

class CommonLibDelegate
{
public:
	virtual void onConfigRefresh(const std::string& content) = 0;
	virtual void onGlobalConfigRefresh(const std::string& content) {}

	virtual void onServerRemoved(int server_type, int server_id) {}
	virtual void onServerAdded(int server_type, int server_id) {}
};

void InitializeCommonLib(class ioservice_thread& thread, CommonLibDelegate* d, int argc = 0, char* argv[] = nullptr);

void ReportLoad(float load_factor);

void UpdateCorrespondingServer(uint64_t user_id, const CorrespondingServer& header_ex);

template<typename P>
bool SendMsg(uint32_t server_id, uint32_t msg_type, uint64_t related_user_id, uint32_t op_id, const CorrespondingServer& cs, P proto)
{
	HeaderEx header_ex;
	header_ex.mutable_servers()->CopyFrom(cs);
	header_ex.set_uid(related_user_id);

	int ex_size = header_ex.ByteSize();
	int proto_size = proto.ByteSize();

	char* send_buf = new char[SERVER_MSG_HEADER_BASE_SIZE + ex_size + proto_size];
	SERVER_MSG_LENGTH(send_buf) = SERVER_MSG_HEADER_BASE_SIZE + ex_size + proto_size;		// 数据包的字节数（含msghead）
	SERVER_MSG_TYPE(send_buf) = msg_type;
	SERVER_MSG_OPERATION_ID(send_buf) = op_id;


	pb.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)CMDEX0_DATA(send_buf));

	std::shared_ptr<NetLibPlus_Client> ls_client = NetLibPlus_get_first_Client(__LS_ServerTypeNameForQuery.c_str());

	if (ls_client)
	{
		ls_client->SendAsync(send_buf);
	}
	else
	{
		delete[] send_buf;
	}
	return true;
}

template<typename P>
bool SendMsg(uint32_t msg_type, uint64_t related_user_id, uint32_t op_id, P proto, bool require_corresponding_server)
{
	

	//TOMODIFY
	int pb_size = proto.ByteSize();

	char* send_buf = new char[CMDEX0_HEAD_SIZE + pb_size];
	CMD_SIZE(send_buf) = CMDEX0_HEAD_SIZE + pb_size;					// 数据包的字节数（含msghead）
	CMD_FLAG(send_buf) = 0;							// 标志位，表示数据是否压缩、加密等
	CMD_CAT(send_buf) = CAT_LOGSVR;				// 命令分类
	CMD_ID(send_buf) = CmdID;
	int tid = new_trans_id();
	CMDEX0_TRANSID(send_buf) = tid;
	{
		boost::lock_guard<boost::mutex> lock(_log_query_result_handlers_mutex);
		result_handlers[tid] = rh;
	}

	pb.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)CMDEX0_DATA(send_buf));

	std::shared_ptr<NetLibPlus_Client> ls_client = NetLibPlus_get_first_Client(__LS_ServerTypeNameForQuery.c_str());

	if (ls_client)
	{
		ls_client->SendAsync(send_buf);
	}
	else
	{
		delete[] send_buf;
	}
	return true;
}

template<typename P>
bool SendMsg(uint32_t msg_type, P proto)
{

	return true;
}

bool SendMsg(uint32_t msg_type, uint32_t op_id, uint8_t error_code)
{

	return true;
}

template<typename P>
bool SendMsg(uint32_t msg_type, uint32_t op_id, uint8_t error_code, P proto)
{
	return true;
}


#endif
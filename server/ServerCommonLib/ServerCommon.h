#ifndef __SERVER_COMMON_H
#define __SERVER_COMMON_H

//无锁版

#ifdef _DEBUG
#define _SERVER_COMMON_LIBNAME_ "ServerCommond"
#else
#define _SERVER_COMMON_LIBNAME_ "ServerCommon"
#endif

#pragma comment(lib, _SERVER_COMMON_LIBNAME_)  //this is not cross-platform. you should write Makefile on linux.

#include <cstdint>
#include <map>

//服务端包头定义

#define SERVER_MSG_LENGTH(d) (*(uint32_t*)(d))
#define SERVER_MSG_TYPE(d) (*(uint32_t*)((d) + 4))
#define SERVER_MSG_AFTER_TYPE_POS (8)
#define SERVER_MSG_ERROR(d) (*(uint8_t*)((d) + SERVER_MSG_AFTER_TYPE_POS)) //后端服务给前端服务返回的错误。如果有错误，那么默认后面的Protobuf就没有了，除非另有约定
#define SERVER_MSG_HEADER_EX_LEN(d) (*(uint8_t*)((d) + SERVER_MSG_AFTER_TYPE_POS + 1))
#define SERVER_MSG_RESERVED(d) (*(uint16_t*)((d) + SERVER_MSG_AFTER_TYPE_POS + 2))
#define SERVER_MSG_HEADER_BASE_SIZE (SERVER_MSG_AFTER_TYPE_POS + 4)
#define SERVER_MSG_AFTER_HEADER_BASE(d) ((d) + SERVER_MSG_HEADER_BASE_SIZE)
#define SERVER_MSG_DATA(d) ((d) + SERVER_MSG_HEADER_BASE_SIZE + SERVER_MSG_HEADER_EX_LEN(d))
#define SERVER_MSG_DATA_LEN(d) (SERVER_MSG_LENGTH(d) - SERVER_MSG_HEADER_BASE_SIZE - SERVER_MSG_HEADER_EX_LEN(d))

#include "commonlib/CommonLib.pb.h"

class CommonLibDelegate
{
public:
	virtual void onInitialized() {}

	virtual void onConfigRefresh(const std::string& content) = 0;
	virtual void onGlobalConfigRefresh(const std::string& content) {}

	virtual void onServerRemoved(int server_type, int server_id) {}
	virtual void onServerAdded(int server_type, int server_id) {}
};

//服务端默认流程：先开监听端口并启动服务，然后告诉控制服务我的端口
//网关服务对外的端口和对内的端口不是一个。对外的端口晚开一会儿
void InitializeCommonLib(class ioservice_thread& thread, CommonLibDelegate* d, int my_listening_port, int my_server_type, int argc = 0, char* argv[] = nullptr);

void ReportLoad(float load_factor);

//TODO 该方法尚有待改进
//void UpdateCorrespondingServer(uint64_t user_id, const common::CorrespondingServer& cs);

#define CONTROL_SERVER_ID (0)
#define CONTROL_SERVER_DEFAULT_PORT (5432)

template<typename P>
bool ParseMsg(char* data, P& proto) //包头无HeaderEx的版本
{
	return proto.ParseFromArray(SERVER_MSG_AFTER_HEADER_BASE(data), SERVER_MSG_LENGTH(data) - SERVER_MSG_HEADER_BASE_SIZE);
}

bool ParseHeaderEx(char* data, common::HeaderEx& proto);

template<typename P>
bool ParseMsgEx(char* data, common::HeaderEx& ex, P& proto)
{
	if (ex.ParseFromArray(SERVER_MSG_AFTER_HEADER_BASE(data), SERVER_MSG_HEADER_EX_LEN(data)))
	{
		return proto.ParseFromArray(SERVER_MSG_DATA(data), SERVER_MSG_DATA_LEN(data));
	}
	return false;
}

template<typename P>
bool ParseMsgOpId(char* data, uint32_t& operation_id, P& proto)
{
	common::HeaderEx ex;
	if (ex.ParseFromArray(SERVER_MSG_AFTER_HEADER_BASE(data), SERVER_MSG_HEADER_EX_LEN(data)))
	{
		operation_id = ex.operation_id();
		return proto.ParseFromArray(SERVER_MSG_DATA(data), SERVER_MSG_DATA_LEN(data));
	}
	return false;
}

template<typename P>
bool ParseMsgUid(char* data, uint32_t& uid, P& proto)
{
	common::HeaderEx ex;
	if (ex.ParseFromArray(SERVER_MSG_AFTER_HEADER_BASE(data), SERVER_MSG_HEADER_EX_LEN(data)))
	{
		uid = ex.uid();
		return proto.ParseFromArray(SERVER_MSG_DATA(data), SERVER_MSG_DATA_LEN(data));
	}
	return false;
}

#include "NetLibPlus.h"

template<typename P>
bool SendMsg(uint32_t server_id, uint32_t msg_type, P& proto)
{
	int proto_size = proto.ByteSize();

	char* send_buf = new char[SERVER_MSG_HEADER_BASE_SIZE + proto_size];
	SERVER_MSG_LENGTH(send_buf) = SERVER_MSG_HEADER_BASE_SIZE + proto_size;		// 数据包的字节数（含msghead）
	SERVER_MSG_TYPE(send_buf) = msg_type;
	memset(send_buf + SERVER_MSG_AFTER_TYPE_POS, 0, SERVER_MSG_HEADER_BASE_SIZE - SERVER_MSG_AFTER_TYPE_POS);

	proto.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)SERVER_MSG_AFTER_HEADER_BASE(send_buf));

	auto client = NetLibPlus_getClient(server_id);
	if (client)
	{
		client->SendAsync(send_buf);
		return true;
	}
	else
	{
		delete[] send_buf;
		return false;
	}
}
/* TODO 尚未实现的SendMsg
template<typename P>
bool SendMsg(uint32_t server_id, uint32_t msg_type, uint64_t related_user_id, uint32_t op_id, const CorrespondingServer& cs, P proto)
{
	HeaderEx header_ex;
	header_ex.mutable_servers()->CopyFrom(cs);
	header_ex.set_uid(related_user_id);

	int ex_size = header_ex.ByteSize();
	int proto_size = proto.ByteSize();

	char* send_buf = new char[SERVER_MSG_HEADER_BASE_SIZE + proto_size];
	SERVER_MSG_LENGTH(send_buf) = SERVER_MSG_HEADER_BASE_SIZE + proto_size;		// 数据包的字节数（含msghead）
	SERVER_MSG_TYPE(send_buf) = msg_type;
	SERVER_MSG_ERROR(send_buf) = 0;
	SERVER_MSG_HEADER_EX_LEN(send_buf) = 0;
	SERVER_MSG_RESERVED(send_buf) = 0;

	proto.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)SERVER_MSG_AFTER_HEADER_BASE(send_buf));

	auto client = NetLibPlus_getClient(server_id);
	if (client)
	{
		client->SendAsync(send_buf);
		return true;
	}
	else
	{
		delete[] send_buf;
		return false;
	}
}

template<typename P>
bool SendMsg(uint32_t msg_type, uint64_t related_user_id, uint32_t op_id, P proto, bool require_corresponding_server = false)
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
bool SendMsg(uint32_t msg_type, P proto) //包头无UserID的版本
{

	return true;
}

bool SendMsg(uint32_t msg_type, uint32_t op_id, uint8_t error_code) //包头无UserID的版本
{

	return true;
}

template<typename P>
bool SendMsg(uint32_t msg_type, uint32_t op_id, uint8_t error_code, P proto) //包头无UserID的版本
{

	return true;
}
*/

template<typename P>
void ReplyMsg(NetLib_ServerSession_ptr sessionptr, uint32_t msg_type, P& proto) //包头无UserID的版本
{
	int proto_size = proto.ByteSize();

	char* send_buf = new char[SERVER_MSG_HEADER_BASE_SIZE + proto_size];
	SERVER_MSG_LENGTH(send_buf) = SERVER_MSG_HEADER_BASE_SIZE + proto_size;		// 数据包的字节数（含msghead）
	SERVER_MSG_TYPE(send_buf) = msg_type;
	SERVER_MSG_ERROR(send_buf) = 0;
	SERVER_MSG_HEADER_EX_LEN(send_buf) = 0;
	SERVER_MSG_RESERVED(send_buf) = 0;

	proto.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)SERVER_MSG_AFTER_HEADER_BASE(send_buf));

	sessionptr->SendAsync(send_buf);
}

template<typename P>
void ReplyMsgEx(NetLib_ServerSession_ptr sessionptr, uint32_t msg_type, common::HeaderEx& ex, P& proto)
{
	int ex_len = ex.ByteSize();
	int proto_size = proto.ByteSize();
	int len = SERVER_MSG_HEADER_BASE_SIZE + ex_len + proto_size;
	char* send_buf = new char[len];
	SERVER_MSG_LENGTH(send_buf) = len;
	SERVER_MSG_TYPE(send_buf) = msg_type;
	SERVER_MSG_ERROR(send_buf) = 0;
	SERVER_MSG_HEADER_EX_LEN(send_buf) = ex_len;
	SERVER_MSG_RESERVED(send_buf) = 0;

	ex.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)SERVER_MSG_AFTER_HEADER_BASE(send_buf));
	proto.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)SERVER_MSG_DATA(send_buf));

	sessionptr->SendAsync(send_buf);
}

template<typename P>
void ReplyMsgOpId(NetLib_ServerSession_ptr sessionptr, uint32_t msg_type, uint32_t operation_id, P& proto)
{
	common::HeaderEx ex;
	ex.set_operation_id(operation_id);
	ReplyMsgEx(sessionptr, msg_type, ex, proto);
}

template<typename P>
void ReplyMsgUid(NetLib_ServerSession_ptr sessionptr, uint32_t msg_type, uint32_t uid, P& proto)
{
	common::HeaderEx ex;
	ex.set_uid(uid);
	ReplyMsgEx(sessionptr, msg_type, ex, proto);
}

#endif
#ifndef __Boids_Server_Header_def_h_
#define __Boids_Server_Header_def_h_

//服务端包头定义

#include <cstdint>

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

template<typename P>
bool ParseMsg(char* data, P& proto) //包头无HeaderEx的版本
{
	return proto.ParseFromArray(SERVER_MSG_AFTER_HEADER_BASE(data), SERVER_MSG_LENGTH(data) - SERVER_MSG_HEADER_BASE_SIZE);
}

#define PARSE_EXECUTE(DATA, PROTO, FUNC) \
{ \
	PROTO __p; \
	if (ParseMsg(DATA, __p)) { \
		FUNC(__p); \
	}\
	else \
{ \
	LOGEVENTL("Error", "Parse " << #PROTO << " failed"); \
} \
}

#define UNRECOGNIZE(title, t) LOGEVENTL("ERROR", title << ": unrecognized msg type: " << t);
#define MSG_TOO_SHORT(title, len) LOGEVENTL("ERROR", title << ": msg too short, got: " << len << ", expect: " << SERVER_MSG_HEADER_BASE_SIZE);

//这里假设了会有个data变量
#define BEGIN_SWITCH \
if (SERVER_MSG_LENGTH(data) >= SERVER_MSG_HEADER_BASE_SIZE) \
{ \
	switch (SERVER_MSG_TYPE(data)) \
{

#define HANDLE_MSG(T, PROTO, FUNC) \
	case T: \
	PARSE_EXECUTE(data, boids::CreateGameResponse, ams.CreateGameResponseGot); \
	break;

#define END_SWITCH(title) \
	default: \
		UNRECOGNIZE(title, SERVER_MSG_TYPE(data)); \
		break; \
	} \
} \
else \
{ \
	MSG_TOO_SHORT(title, SERVER_MSG_LENGTH(data)); \
}

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

#endif
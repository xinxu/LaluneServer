#ifndef __Boids_Server_Header_def_h_
#define __Boids_Server_Header_def_h_

//服务端包头定义

#include <cstdint>
#include "boids.pb.h"

#define MSG_LENGTH(d) (*(uint32_t*)(d))
#define MSG_DATA(d) ((uint32_t*)(d + 4))
#define MSG_HEADER_LEN (4)
#define MSG_DATA_LEN(d) (MSG_LENGTH(d) - MSG_HEADER_LEN)

#define PARSE_EXECUTE(DATA, PROTO, FUNC) \
{ \
	PROTO __p; \
	if (__p.ParseFromString(DATA)) { \
		FUNC(__p); \
	}\
	else \
	{ \
		LOGEVENTL("Error", "Parse " << #PROTO << " failed"); \
	} \
}

#define UNRECOGNIZE(title, t) LOGEVENTL("ERROR", title << ": unrecognized msg type: " << t);
#define MSG_TOO_SHORT(title, len) LOGEVENTL("ERROR", title << ": msg too short, got: " << len << ", expect at least: " << MSG_HEADER_LEN);

//这里假设了会有个data变量
#define BEGIN_SWITCH \
if (MSG_LENGTH(data) >= MSG_HEADER_LEN) \
{ \
	boids::BoidsMessageHeader __msg; \
	if (__msg.ParseFromArray(MSG_DATA(data), MSG_DATA_LEN(data))) \
	{ \
	switch (__msg.type()) \
		{

#define HANDLE_MSG(T, PROTO, FUNC) \
		case T: \
			PARSE_EXECUTE(data, PROTO, FUNC); \
			break;

//必须和BEGIN_SWITCH配套使用
#define END_SWITCH(title) \
		default: \
			UNRECOGNIZE(title, __msg.type()); \
			break; \
		} \
	} \
} \
else \
{ \
	MSG_TOO_SHORT(title, MSG_LENGTH(data)); \
}

template<typename P>
void ReplyMsg(NetLib_ServerSession_ptr sessionptr, boids::MessageType msg_type, P& proto) //包头无UserID的版本
{
	boids::BoidsMessageHeader proto_with_header;
	proto_with_header.set_type(msg_type);
	proto.SerializeToString(*proto_with_header.mutable_data());

	int proto_size = proto_with_header.ByteSize();

	char* send_buf = new char[MSG_HEADER_LEN + proto_size];
	MSG_LENGTH(send_buf) = MSG_HEADER_LEN + proto_size;

	proto_with_header.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)MSG_DATA(send_buf));

	sessionptr->SendAsync(send_buf);
}

#endif
#pragma once

//这是老的消息定义，待修改

#define CMD_HEAD_SIZE (12)
#define CMDEX0_HEAD_SIZE (24)
#define CMDEX_HEAD_SIZE (32)

//方便访问消息头数据
#define CMD_SIZE(D) (*(uint32_t*)(D))
#define CMD_FLAG(D) (*(uint32_t*)((char*)D + 4))
#define CMD_CAT(D)	(*(uint16_t*)((char*)D + 8))
#define CMD_ID(D)	(*(uint16_t*)((char*)D + 10))
#define CMD_DATA(D) ((char*)D + 12)
#define CMD_DATA_SIZE(D)	(CMD_SIZE(D) - CMD_HEAD_SIZE)

#define CMDEX0_TRANSID(D)	(*(uint64_t*)((char*)D + 12))
#define CMDEX0_RESULT(D)	(*(int32_t*)((char*)D + 20))
#define CMDEX0_DATA(D)		((char*)D + 24)
#define CMDEX0_DATA_SIZE(D)	(CMD_SIZE(D) - CMDEX0_HEAD_SIZE)

#define CMDEX_USERID(D)		(*(T_USER_ID*)((char*)D + 12))
#define CMDEX_TRANSID(D)	(*(uint64_t*)((char*)D + 20))
#define CMDEX_RESULT(D)		(*(int32_t*)((char*)D + 28))
#define CMDEX_DATA(D)		((char*)D + 32)
#define CMDEX_DATA_SIZE(D)	(CMD_SIZE(D) - CMDEX_HEAD_SIZE)


#include "CmdDefine.h"
#pragma once

#define CAT_LOGSVR	0x4150 //"Log服务器相关消息"

#define ID_LOGSVR_LOGIN 0x01 //"登陆Log服务器，目前主要是提交index1和index2的信息，并假设之后的Log都使用相同的index1和index2，避免以后每次都发"
#define ID_LOGSVR_LOG 0x02 //"打Log"
#define ID_LOGSVR_LOGIN_FAIL  		0x03 //"登陆Log服务器失败"
#define ID_LOGSVR_LOGOPTIONS  		0x04 //"Log服务器返回Log选项的更新"
#define ID_LOGSVR_NO_PRIVILEDGE  	0x05 //"对应的请求权限不足"
#define ID_LOGSVR_QUERY_123  		0x10 //"查询Log，参数为：index1/index2/index3"
#define ID_LOGSVR_QUERY_123p  		0x11 //"查询Log，参数为：index1/index2/index3_prefix"
#define ID_LOGSVR_QUERY_13  			0x12 //"查询Log，参数为：index1/index3"
#define ID_LOGSVR_QUERY_13p  		0x13 //"查询Log，参数为：index1/index3_prefix"
#define ID_LOGSVR_QUERY_1p3p  		0x14 //"查询Log，参数为：index1_prefix/index3_prefix"
#define ID_LOGSVR_QUERY_1p3  		0x15 //"查询Log，参数为：index1_prefix/index3"
#define ID_LOGSVR_QUERY_RESULT  		0x20 //"Log服务器向Log查询者返回Log(部分"
#define ID_LOGSVR_QUERY_RESULT_END  	0x21 //"Log服务器向Log查询者返回Log(最后一个包"
#define ID_LOGSVR_SHOW_QUERIES  		0x30 //"显示所有正在执行或等待执行的查询"
#define ID_LOGSVR_KILL_QUERY  		0x31 //"Kill指定TransactionID的查询"
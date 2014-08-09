#include "NetLib_Params.h"

int UDP_REPLY_INTERVAL_MS = 90;  //12.07.23 从120改为90
//默认有一个时间间隔才reply。这样对于收到后处理很快的包，可以省一个单独的reply包
//如果发送方需要尽快知道SendFinish的话，那么应将这个值设小。设小对于持续发送的流有好处。因为REPLY的时间间隔延长了RTT(Round Trip Time)

int UDP_MAX_CONSECUTIVE_RESEND_TIMES = 5; //同一包连续RESEND超过该次数则认为失败

unsigned int MAX_PACKET_SIZE = (512 * 1024);

int SERVER_RECV_BUFFER_SIZE	= 16 * 1024 * 1024;

int CLIENT_RECV_BUFFER_SIZE = 16 * 1024;
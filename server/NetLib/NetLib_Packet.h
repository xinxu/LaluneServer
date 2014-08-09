#ifndef _NETLIB_PACKET_H_
#define _NETLIB_PACKET_H_

typedef struct tag_netlib_packet {
	char* data;
	uint32_t data_size;
	bool copy;
	void* pHint;

	tag_netlib_packet()
	{
	}

	tag_netlib_packet(const char* _data, bool _copy = false, void* phint = nullptr) : data((char*)_data), data_size(*((uint32_t*)data)), copy(_copy), pHint(phint)
	{
	}
} netlib_packet;

#endif
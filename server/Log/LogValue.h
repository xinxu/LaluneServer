#pragma once

namespace log_
{
	struct h
	{
		uint64_t v;
		h(uint64_t _v) : v(_v)
		{
		}
		h(void* _v) : v((uint64_t)_v)
		{
		}
	};

	struct h16
	{
		uint64_t v;
		h16(uint64_t _v) : v(_v)
		{
		}
	};

	struct bytes
	{
		const char* v;
		int l;
		bytes(const char* _v, int len) : v(_v), l(len)
		{
		}
	};
	typedef bytes b;

	struct bytes_display
	{
		const char* v;
		int l;
		bytes_display(const char* _v, int len) : v(_v), l(len)
		{
		}
	};
	typedef bytes_display bd;
}

#define hex(V) log_::h(V)
#define hex16(V) log_::h16(V)
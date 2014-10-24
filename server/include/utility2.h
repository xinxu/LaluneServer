#pragma once

#include <string>
#include <boost/asio.hpp>
#include "include/utility1.h"

typedef std::pair<uint32_t, int> IPPort;

class utility2
{
public:
	static std::string toIPs(uint32_t IPu)
	{
		return boost::asio::ip::address_v4(IPu).to_string();
	}

	static std::string printAddr(IPPort ipport)
	{
		return utility2::toIPs(ipport.first) + ":" + utility1::int2str(ipport.second);
	}
};
#pragma once

#include <string>
#include <boost/asio.hpp>

class utility2
{
public:
	static std::string toIPs(uint32_t IPu)
	{
		return boost::asio::ip::address_v4(IPu).to_string();
	}
};
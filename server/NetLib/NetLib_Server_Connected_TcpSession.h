#ifndef _NetLib_Server_Connected_TcpSession_H_
#define _NetLib_Server_Connected_TcpSession_H_

#include "NetLib_Connected_TcpSession.h"

class NetLib_Server_Connected_TcpSession : public NetLib_Connected_TcpSession
{
protected:
	tcp::socket socket;

public:
	virtual ~NetLib_Server_Connected_TcpSession()
	{
		if (socket.is_open())
		{
			boost::system::error_code ignored_error;
			socket.close(ignored_error);
		}
	}

	NetLib_Server_Connected_TcpSession(boost::asio::io_service & ioservice) : NetLib_Connected_TcpSession(ioservice), socket(ioservice)
	{
		set_socket(&socket);
	}

	tcp::socket& get_socket()
	{
		return socket;
	}
};

#endif
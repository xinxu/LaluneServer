#include "ControlServerSessionDelegate.h"
#include "ServerCommonLib/ServerCommon.h"
#include "Log/Log.h"
#include "MessageTypeDef.h"
#include "controlserver/ControlServer.pb.h"
#include "ControlServer.h"
#include <boost/bind.hpp>
#include "include/utility1.h"
#include "include/utility2.h"
#include "ControlServerConfig.h"
#include <boost/asio.hpp>

void ControlServerSessionDelegate::ConnectedHandler(NetLib_ServerSession_ptr sessionptr)
{
	//暂时没有什么要做的。因为连上之后还得发了自己的端口才有意义
	//LOGEVENTL("DEBUG", "connected");
}

void ControlServerSessionDelegate::DisconnectedHandler(NetLib_ServerSession_ptr sessionptr)
{
	session2server.erase(sessionptr);
	//只要维护session2server map就好，servers_info另有超时事件维护
}

void ControlServerSessionDelegate::RecvKeepAliveHandler(NetLib_ServerSession_ptr sessionptr)
{
	//刷新超时时间
	auto session_it = session2server.find(sessionptr);
	if (session_it != session2server.end())
	{
		auto info_it = servers_info.find(session_it->second);
		if (info_it == servers_info.end())
		{
			LOGEVENTL("ERROR", "a server exist in session2server, but not in servers_info. " << log_::n("ip") << utility2::toIPs(session_it->second.first) << log_::n("port") << session_it->second.second);
		}
		else
		{
			//info_it->second->refresh();
		}
	}
}

//RecvFinishHandler一旦返回，data的内容就会被释放
void ControlServerSessionDelegate::RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
{
	if (SERVER_MSG_LENGTH(data) >= SERVER_MSG_HEADER_BASE_SIZE)
	{
		switch (SERVER_MSG_TYPE(data))
		{
		case MSG_TYPE_CONTROL_SERVER_SAY_HELLO:
			{
				common::Hello hello;
				if (ParseMsg(data, hello))
				{
					std::pair<int, int> ip_port(sessionptr->GetRemoteIPu(), hello.my_listening_port());

					//维护session到地址的映射				
					session2server[sessionptr] = ip_port;

					if (hello.is_server_start() == 1)
					{
						auto info_it = servers_info.find(ip_port);
						if (info_it == servers_info.end())
						{
							ServerInfo* info = new ServerInfo(ip_port, hello.server_type());
							servers_info.insert(std::make_pair(ip_port, info));

							common::HelloResult hello_result;
							hello_result.set_server_id(info->addr.server_id());
							ReplyMsg(sessionptr, MSG_TYPE_CONTROL_SERVER_SAY_HELLO_RESULT, hello_result);

							if (!during_startup)
							{
								informAddressInfo(info->addr, MSG_TYPE_CONTROL_SERVER_ADDR_INFO_ADD);
								LOGEVENTL("INFO", "Send add to all. " << _ln("server_id") << info->addr.server_id()
									<< _ln("IP") << utility2::toIPs(info->addr.ip()) << _ln("Port") << info->addr.port()
									<< _ln("server_type") << hello.server_type());
							}
						}
						else
						{
							//如果有了，说明是同一台机器，重启了。因为只有启动的时候会进这儿(is_server_start为1)。
							//server_id就不用变了，假设server_type也不会变。那么只要刷timer就好了
							//暂时可能各服务收到RESTART都不用做什么处理
							info_it->second->refresh();

							common::HelloResult hello_result;
							hello_result.set_server_id(info_it->second->addr.server_id());
							ReplyMsg(sessionptr, MSG_TYPE_CONTROL_SERVER_SAY_HELLO_RESULT, hello_result);

							if (!during_startup)
							{
								common::ServerId _id;
								_id.set_server_id(info_it->second->addr.server_id());
								informAddressInfo(_id, MSG_TYPE_CONTROL_SERVER_ADDR_INFO_RESTART);
								LOGEVENTL("INFO", "Send restart to all. " << _ln("server_id") << _id.server_id());
							}
						}

						if (!during_startup)
						{
							//告诉他其它人的信息
							common::AddressList addr_list;
							GenerateAddressList(addr_list);
							ReplyMsg(sessionptr, MSG_TYPE_CONTROL_SERVER_ADDR_INFO_REFRESH, addr_list);
							LOGEVENTL("INFO", "Send list to the new one. " << _ln("IP") << utility2::toIPs(ip_port.first) << _ln("Port") << ip_port.second);
						}
					}
					else
					{
						LOGEVENTL("INFO", "Receive Hello but not server_start. " << _ln("IP") << utility2::toIPs(ip_port.first) << _ln("Port") << ip_port.second);
					}
				}
			}
			break;
		case MSG_TYPE_CONTROL_SERVER_REPORT_LOAD:
			//TODO
			break;
		case MSG_TYPE_CONTROL_SERVER_CMD:
			if (sessionptr->GetRemoteIP() == "127.0.0.1")
			{
				control_server::Command cmd;
				if (ParseMsg(data, cmd))
				{
					if (cmd.command_name() == "reload")
					{
						LOGEVENTL("Info", "Reload config: " << config_file_path);
						thread.get_ioservice().post(boost::bind(&LoadConfig));
					}
					else
					{
						control_server::CommandResult cmd_result;
						cmd_result.set_result("CommandName not recognized: " + cmd.command_name());
						ReplyMsg(sessionptr, MSG_TYPE_CONTROL_SERVER_CMD_RESULT, cmd_result);
					}
				}
				else
				{
					control_server::CommandResult cmd_result;
					cmd_result.set_result("Protobuf parse failed");
					ReplyMsg(sessionptr, MSG_TYPE_CONTROL_SERVER_CMD_RESULT, cmd_result);
				}
			}
			else
			{
				LOGEVENTL("WARN", "None-local address tries to send command to ControlServer. IP: " << sessionptr->GetRemoteIP());
			}
			break;
		default:
			LOGEVENTL("WARN", "MsgType not recognized: " << SERVER_MSG_TYPE(data));
			break;
		}
	}
	else
	{
		LOGEVENTL("ERROR", "message not enough size: " << SERVER_MSG_LENGTH(data));
	}
}
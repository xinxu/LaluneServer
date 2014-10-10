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
#include "include/ptime2.h"

void ControlServerSessionDelegate::ConnectedHandler(NetLib_ServerSession_ptr sessionptr)
{
	//暂时没有什么要做的。因为连上之后还得发了自己的端口才有意义
	//LOGEVENTL("DEBUG", "connected");
}

void ControlServerSessionDelegate::DisconnectedHandler(NetLib_ServerSession_ptr sessionptr)
{
	int attached_data = sessionptr->GetAttachedData();
	if (attached_data) //如果为0，则说明在发Hello以前
	{
		//只要维护server2session就好，servers_info和server_groups另有超时事件维护
		server2session.erase(*(IPPort*)attached_data);
		delete (IPPort*)sessionptr->GetAttachedData();
	}
}

void ControlServerSessionDelegate::RecvKeepAliveHandler(NetLib_ServerSession_ptr sessionptr)
{
	//刷新超时时间
	int attached_data = sessionptr->GetAttachedData();
	if (attached_data) //如果为0，则说明在发Hello以前
	{
		IPPort ip_port = *(IPPort*)attached_data;
		auto info_it = servers_info.find(ip_port);
		if (info_it == servers_info.end())
		{
			LOGEVENTL("ERROR", "a server exist in session2server, but not in servers_info. " << log_::n("ip") << utility2::toIPs(ip_port.first) << log_::n("port") << ip_port.second);
		}
		else
		{
			info_it->second->refresh();
		}
	}
}

//这个名字关系到配置文件的存放目录。如果名字有修改，需要把相应的目录也做修改。
std::string GetServerTypeWrittenName(int server_type)
{
	switch (server_type)
	{
	case SERVER_TYPE_GATEWAY_SERVER:
		return "gateway";
	case SERVER_TYPE_VERSION_SERVER:
		return "version_server";
	case SERVER_TYPE_ACCOUNT_SERVER:
			return "account_server";
	case SERVER_TYPE_BASIC_INFO_SERVER:
		return "basic_info_server";
	case SERVER_TYPE_LEAGUE_SERVER:
		return "league_server";
	case SERVER_TYPE_NOTICE_SERVER:
		return "notice_server";
	case SERVER_TYPE_RANK_SERVER:
		return "rank_server";
	case SERVER_TYPE_ASYNC_BATTLE_SERVER:
		return "async_battle_server";
	case SERVER_TYPE_REPLAY_SERVER:
		return "replay_server";
	case SERVER_TYPE_STAT_SERVER:
		return "stat_server";
	case SERVER_TYPE_AUTO_MATCH_SERVER:
		return "auto_match_server";
	case SERVER_TYPE_SYNC_BATTLE_SERVER:
		return "sync_battle_server";
	case SERVER_TYPE_CONTROL_SERVER:
		return "control_server";
	case SERVER_TYPE_BACKGROUND:
		return "background";
	default:
		return "default";
	}
}

std::map<std::pair<int, std::string>, std::string*> configs;

std::string* readConfig(int server_type, const std::string& file_name)
{
	auto it_config = configs.find(std::make_pair(server_type, file_name));
	if (it_config != configs.end())
	{
		return it_config->second;
	}

	//如果内存里没有就再读一遍
	std::string* content = new std::string();
	file_utility::readFile("configs/" + GetServerTypeWrittenName(server_type) + "/" + file_name, *content);
	configs.insert(std::make_pair(std::make_pair(server_type, file_name), content));
	return content;
}

void writeConfig(int server_type, const std::string& file_name, const std::string& content)
{
	//写一份作为历史
	file_utility::writeFile("configs/" + GetServerTypeWrittenName(server_type) +
		"/[" + time_utility::ptime_to_string4(boost::posix_time::microsec_clock::local_time()) + "] " + file_name, content);

	//存到当前文件
	file_utility::writeFile("configs/" + GetServerTypeWrittenName(server_type) + "/" + file_name, content);

	//存到内存map
	auto it_config = configs.find(std::make_pair(server_type, file_name));
	if (it_config != configs.end())
	{
		delete it_config->second;
	}

	configs[std::make_pair(server_type, file_name)] = new std::string(content);	
}

//RecvFinishHandler一旦返回，data的内容就会被释放
void ControlServerSessionDelegate::RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
{
	if (SERVER_MSG_LENGTH(data) >= SERVER_MSG_HEADER_BASE_SIZE)
	{
		switch (SERVER_MSG_TYPE(data))
		{
		case MSG_TYPE_CONTROL_SERVER_INITIALIZE:
			{
				common::Initialize init;
				if (ParseMsg(data, init))
				{
					//取对应的配置文件，发给他

					common::RefreshConfig rc;
					rc.set_server_type(init.server_type());
					for (auto kvp : configs)
					{
						if (kvp.first.first == init.server_type())
						{
							common::ConfigFile* file = rc.add_file();
							file->set_file_name(kvp.first.second);
							file->set_content(*kvp.second);
						}
					}
													   
					ReplyMsg(sessionptr, MSG_TYPE_REFRESH_CONFIG, rc);
				}												   
			}
			break;
		case MSG_TYPE_CONTROL_SERVER_SAY_HELLO:
			{
				common::Hello hello;
				if (ParseMsg(data, hello))
				{
					IPPort* ip_port = new IPPort(sessionptr->GetRemoteIPu(), hello.my_listening_port());

					//维护session到地址的映射
					sessionptr->SetAttachedData((int)ip_port);
					server2session[*ip_port] = sessionptr;

					if (hello.is_server_start() == 1)
					{
						auto info_it = servers_info.find(*ip_port);
						if (info_it == servers_info.end())
						{
							ServerInfo* info = new ServerInfo(*ip_port, hello.server_type());
							servers_info.insert(std::make_pair(*ip_port, info));

							//维护sessions_groupby_servertype
							auto it_group = server_groups.find(hello.server_type());
							if (it_group != server_groups.end())
							{
								it_group->second->insert(*ip_port);
							}

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
							LOGEVENTL("INFO", "Send list to the new one. " << _ln("IP") << utility2::toIPs(ip_port->first) << _ln("Port") << ip_port->second);
						}
					}
					else
					{
						LOGEVENTL("INFO", "Receive Hello but not server_start. " << _ln("IP") << utility2::toIPs(ip_port->first) << _ln("Port") << ip_port->second);
					}
				}
			}
			break;
		case MSG_TYPE_CONTROL_SERVER_REPORT_LOAD:
			//TODO
			break;
		case MSG_TYPE_CMD2SERVER:
			{
				//从后台过来的命令。后台那边还得有个权限的验证。TODO
				common::Cmd2Server cmd;
				if (ParseMsg(data, cmd))
				{
					auto it_group = server_groups.find(cmd.to_server_type());
					if (it_group != server_groups.end())
					{
						cmd.clear_to_server_type();
						for (auto it_server = it_group->second->begin(); it_server != it_group->second->end(); ++it_server)
						{
							auto it_session = server2session.find(*it_server);
							if (it_session != server2session.end())
							{
								ReplyMsg(it_session->second, MSG_TYPE_CMD2SERVER, cmd);
							}
						}
					}
				}
			}
			break;		
		case MSG_TYPE_REFRESH_CONFIG:
			{
				//从后台过来的刷配置文件命令。后台那边还得有个权限的验证。TODO
				common::RefreshConfig rc;
				if (ParseMsg(data, rc))
				{
					auto it_group = server_groups.find(rc.server_type());
					if (it_group != server_groups.end())
					{
						for (int i = 0; i < rc.file_size(); ++i)
						{
							LOGEVENTL("INFO", "new config, " << _ln("server_type") << rc.server_type() << _ln("file_name") << rc.file(i).file_name());

							writeConfig(rc.server_type(), rc.file(i).file_name(), rc.file(i).content());

							//发给相关各服务
							rc.clear_server_type();
							for (auto it_server = it_group->second->begin(); it_server != it_group->second->end(); ++it_server)
							{
								auto it_session = server2session.find(*it_server);
								if (it_session != server2session.end())
								{
									ReplyMsg(it_session->second, MSG_TYPE_REFRESH_CONFIG, rc);
								}
							}
						}
					}
				}
			}
			break;
		case MSG_TYPE_CONTROL_SERVER_FETCH_CONFIG_REQUEST:
			{
				control_server::FetchConfigRequest fc;
				if (ParseMsg(data, fc))
				{
					common::ConfigFile cf;
					cf.set_file_name(fc.file_name());
					cf.set_content(*readConfig(fc.server_type(), fc.file_name()));
					ReplyMsg(sessionptr, MSG_TYPE_CONTROL_SERVER_FETCH_CONFIG_RESPONSE, cf);
				}
			}
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
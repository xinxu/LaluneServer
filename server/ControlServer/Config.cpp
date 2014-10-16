#include "Config.h"
#include "include/utility1.h"
#include "include/ptime2.h"
#include "rapidjson.h"
#include "document.h"
#include "stringbuffer.h"
#include "prettywriter.h"
#include "ControlServerConfig.h"
#include "Log/Log.h"
#include "MessageTypeDef.h"
#include "include/ToAbsolutePath.h"

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

	return nullptr;
}

void writeConfig(int server_type, const std::string& file_name, const std::string& content)
{
	//写一份作为历史
	file_utility::writeFile(utility3::ToAbsolutePath("configs/" + GetServerTypeWrittenName(server_type) +
		"/[" + time_utility::ptime_to_string4(boost::posix_time::microsec_clock::local_time()) + "] " + file_name), content);

	//存到当前文件
	file_utility::writeFile(utility3::ToAbsolutePath("configs/" + GetServerTypeWrittenName(server_type) + "/" + file_name), content);

	//存到内存map
	auto it_config = configs.find(std::make_pair(server_type, file_name));
	if (it_config != configs.end())
	{
		delete it_config->second;
	}
	else
	{
		//写到control_server.json

		std::string content; 
		file_utility::readFile(config.server_configs_list_file, content);

		rapidjson::Document document;		
		document.Parse<0>(content.c_str());
		if (document.IsArray())
		{
			rapidjson::Value v(rapidjson::kObjectType);
			v.AddMember("server_type", server_type, document.GetAllocator());
			std::string server_type_name = GetServerTypeWrittenName(server_type); 
			//加的字符串的生命周期必须要比json那些类的要长，否则到时候就会访问到已经释放的空间，出来/u0000，其实都非法访问了。
			//即下面那行不能直接传GetServerTypeWrittenName().c_str()
			//要让rapidjson拷贝，可以弄个kStringType的Value，且要穿allocator
			v.AddMember("server_type_name", rapidjson::GenericStringRef<char>(server_type_name.c_str(), server_type_name.size()), document.GetAllocator());
			v.AddMember("file_name", rapidjson::GenericStringRef<char>(file_name.c_str(), file_name.size()), document.GetAllocator());
			document.PushBack(v, document.GetAllocator());
			rapidjson::StringBuffer buffer;
			rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
			document.Accept(writer);

			file_utility::writeFile(config.server_configs_list_file + ".bak", content);
			file_utility::writeFile(config.server_configs_list_file, buffer.GetString());
		}
		else
		{
			LOGEVENTL("ERROR", config.server_configs_list_file << " not an array. At least an empty array is needed.");
		}
	}

	configs[std::make_pair(server_type, file_name)] = new std::string(content);
}

void initializeConfigs()
{
	//改成绝对路径
	config.server_configs_list_file = utility3::ToAbsolutePath(config.server_configs_list_file);

	std::string content;
	file_utility::readFile(config.server_configs_list_file, content);

	rapidjson::Document document;
	document.Parse<0>(content.c_str());

	if (document.IsArray())
	{
		unInitializeConfigs();
		configs.clear();

		for (int i = 0; i < document.Size(); ++i)
		{
			int server_type;
			if (document[i]["server_type"].IsNumber())
			{
				server_type = document[i]["server_type"].GetInt();
			}
			else
			{
				continue;
			}
			
			std::string file_name;
			if (document[i]["file_name"].IsString())
			{
				file_name = document[i]["file_name"].GetString();
			}
			else
			{
				continue;
			}

			std::string* content = new std::string();
			file_utility::readFile(utility3::ToAbsolutePath("configs/" + GetServerTypeWrittenName(server_type) + "/" + file_name), *content);
			configs.insert(std::make_pair(std::make_pair(server_type, file_name), content));
		}
	}
	else
	{
		LOGEVENTL("ERROR", config.server_configs_list_file << " not an array. At least an empty array is needed.");
	}
}

void unInitializeConfigs()
{
	for (auto it = configs.begin(); it != configs.end(); ++it)
	{
		delete it->second;
	}
}
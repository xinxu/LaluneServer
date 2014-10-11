#pragma once

#include <map>
#include <string>

//这个Configs指的是各服务放ControlServer这里的配置

extern std::map<std::pair<int, std::string>, std::string*> configs;

std::string* readConfig(int server_type, const std::string& file_name);

void writeConfig(int server_type, const std::string& file_name, const std::string& content);

void initializeConfigs();

void unInitializeConfigs();
#ifndef __CONTROL_SERVER_CONFIG_H_
#define __CONTROL_SERVER_CONFIG_H_

#define _CONFIG_DEFAULT_STARTUP_MS (8000)
#define _CONFIG_DEFAULT_TIMEOUT_SEC (7)

class ControlServerConfig
{
public:
	int startup_ms = _CONFIG_DEFAULT_STARTUP_MS;
	int timeout_sec = _CONFIG_DEFAULT_TIMEOUT_SEC;
	std::string server_configs_list_file = "configs/list.json";
};

extern ControlServerConfig config;

#endif
#pragma once

#include "LogOption.h"
#include <boost/asio/ip/udp.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include "../include/ioservice_thread.h"

extern std::string __LS_ServerTypeName;
extern std::string __LS_ServerTypeNameForQuery;

extern boost::shared_mutex _log_shared_mutex;

extern std::string FileNamePrefix;
extern bool EnableToConsole;
extern bool EnableToFile;
extern bool SplitByDate;
extern int LogTotalMegaBytesLimitWithinDir;
extern int LogTotalFileLimitWithinDir;
extern unsigned global_LogLevel, global_CountInterval, global_EnableDetail;
extern std::string global_index1, global_index2;

extern std::shared_ptr<boost::asio::ip::udp::socket> udpsocket_4log;
extern boost::asio::ip::udp::endpoint udpendpoint_4log;
extern boost::mutex _udpsocket_mutex;

extern std::map<std::string, LogOption> options_map;

extern ioservice_thread __log_ioservice_th;

void SendLoginToLS();
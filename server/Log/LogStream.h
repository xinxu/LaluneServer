#pragma once

#include "LogOption.h"
#include "BasicLogStream.h"

class LOG_API LogStream : public BasicLogStream //还有打到屏幕/打到文件/级别控制/统计 等功能
{
protected:
	const std::string m_index3;
	LogOption op;
	bool owns_lock;

	void LogToLocal();

	void FormatToString(std::string& formatted_log) const;
	void FormatToStringComplete(std::string& formatted_log) const;

public:
	LogStream(const char* index3);
	virtual ~LogStream();

	bool IsEnable() const;
	void Log(bool IsLocal = false); //Local则最多执行打屏幕、打文件的操作
};
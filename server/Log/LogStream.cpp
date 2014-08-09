#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/system/error_code.hpp>
#include "LogStream.h"
#include "internal.h"
#include "LogLevel.h"
#include "LogFile.h"
#include "Log.h"
#include <map>
#include "Counter.h"

LogStream::LogStream(const char* index3) : m_index3(index3), owns_lock(true)
{
	_log_shared_mutex.lock_shared();

	auto it = options_map.find(index3);
	if (it == options_map.end())
	{
		if (global_index1.empty())
		{
			op.LogLevel = global_LogLevel;
			op.CountInterval = 0;
			op.EnableDetail = 0;
		}
		else
		{
			op.LogLevel = global_LogLevel;
			op.CountInterval = global_CountInterval;
			op.EnableDetail = global_EnableDetail;
		}
	}
	else
	{
		op = it->second;
	}
}

LogStream::~LogStream()
{
	if (owns_lock)
	{
		_log_shared_mutex.unlock_shared();
	}
}

bool LogStream::IsEnable() const
{
	return global_LogLevel <= op.LogLevel;
}

void LogStream::FormatToString(std::string& formatted_log) const
{
	char left_part[28];
	left_part[0] = '[';
	time_utility::ptime_to_string_full(t, left_part + 1);

	formatted_log = left_part;
	formatted_log += "] [";
	formatted_log += m_index3;
	formatted_log += "] ";
	formatted_log += ToString(); // std::string 的 += 操作调用的是底层的append方法，比 = p1 + p2 + p3 + p4这样快
}

void LogStream::FormatToStringComplete(std::string& formatted_log) const
{
	char buf[300];
	sprintf(buf, "[%s] [%s] [%s] [%s] ", time_utility::ptime_to_string_full(t).c_str(), global_index1.c_str(), global_index2.c_str(), m_index3.c_str());
	formatted_log = buf;
	formatted_log += ToString();
}

void LogStream::LogToLocal()
{	
	if (EnableToConsole || EnableToFile)
	{
		std::string formatted_log;
		FormatToString(formatted_log);

		if (EnableToConsole)
		{
			printf("%s\n", formatted_log.c_str());
		}

		if (EnableToFile && FileNamePrefix.size()) //FileNamePrefix则不打文件。即，LogInitializeLocalOptions或LogSetIndex12至少要有一个被调用，才会打文件
		{	
			file.PrintLine(formatted_log);
		}
	}
}

void LogStream::Log(bool IsLocal)
{
	RecordTime();

	LogToLocal();

	_log_shared_mutex.unlock_shared(); //后面不再会访问全局变量了，就提前出锁
	owns_lock = false;

	if (! IsLocal)
	{
		if (op.CountInterval) //如果global_index1还没有赋值，则LogStream在构造的时候就会把CountInterval置为0
		{
			IncreaseCounter(m_index3, op.CountInterval);
		}

		if (op.EnableDetail) //如果global_index1还没有赋值，则LogStream在构造的时候就会把EnableDetail置为0
		{
			//这里不能在_log_shared_mutex锁里，是因为里面可能进NetLib的锁，这里如果在锁里可能导致进锁顺序不一致而死锁。
			//在进锁顺序上，假设是先NetLib的锁，再Log的锁

			LogToNet(m_index3);
		}
	}
}
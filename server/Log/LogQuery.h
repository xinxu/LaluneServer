#pragma once

#ifndef _LOG_DLL_

#include <memory>
#include <string>
#include <stdint.h>

namespace kit{ namespace log{ class LogEvent; } }

class ResultHandler
{
public:
	virtual void result_end() {} //result_end执行结束后，Log模块就不再持有ResultHandler的shared_ptr
	virtual void handle_result(const kit::log::LogEvent& log_event) = 0;
};

class CountResultHandler : public ResultHandler
{
protected:
	uint64_t count;

public:
	CountResultHandler() : count(0)
	{
	}

	void handle_result(const kit::log::LogEvent& log_event)
	{
		count ++;
	}
};

class AsStringResultHandler : public ResultHandler
{
protected:
	void handle_result(const kit::log::LogEvent& log_event);

public:
	static void ToString(const kit::log::LogEvent& log_event, std::ostringstream& os);

	virtual void handle_string(const std::string& log) = 0;
};

#include "LogDataReader.h"

class AsDataResultHandler : public ResultHandler
{
protected:
	bool m_enable_find_by_name;
	void handle_result(const kit::log::LogEvent& log_event);

public:
	AsDataResultHandler(bool enable_find_by_name = true) : m_enable_find_by_name(enable_find_by_name)
	{
	}

	virtual void handle_data(const std::string& index1, const std::string& index2, const std::string& index3, uint64_t time, const LogDataReader& reader) = 0;
};

#include "const_time.h"

void Query123(std::shared_ptr<ResultHandler> rh, const std::string& index1, const std::string& index2, const std::string& index3, 
	bool no_sort, uint64_t begin_time = TIME64_NEGATIVE_INFINITY, uint64_t end_time = TIME64_POSITIVE_INFINITY);

void Query123p(std::shared_ptr<ResultHandler> rh, const std::string& index1, const std::string& index2, const std::string& index3_prefix, 
	bool no_sort, uint64_t begin_time = TIME64_NEGATIVE_INFINITY, uint64_t end_time = TIME64_POSITIVE_INFINITY);

void Query13(std::shared_ptr<ResultHandler> rh, const std::string& index1, const std::string& index3, 
	bool no_sort, uint64_t begin_time = TIME64_NEGATIVE_INFINITY, uint64_t end_time = TIME64_POSITIVE_INFINITY);

void Query13p(std::shared_ptr<ResultHandler> rh, const std::string& index1, const std::string& index3_prefix, 
	bool no_sort, uint64_t begin_time = TIME64_NEGATIVE_INFINITY, uint64_t end_time = TIME64_POSITIVE_INFINITY);

void Query1p3p(std::shared_ptr<ResultHandler> rh, const std::string& index1_prefix, const std::string& index3_prefix, 
	bool no_sort, uint64_t begin_time = TIME64_NEGATIVE_INFINITY, uint64_t end_time = TIME64_POSITIVE_INFINITY);

void Query1p3(std::shared_ptr<ResultHandler> rh, const std::string& index1_prefix, const std::string& index3, 
	bool no_sort, uint64_t begin_time = TIME64_NEGATIVE_INFINITY, uint64_t end_time = TIME64_POSITIVE_INFINITY);

#endif
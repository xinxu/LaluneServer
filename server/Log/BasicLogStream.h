#pragma once

#include "SemanticStream.h"

class BasicLogStream : public SemanticStream //只有记录时间，并发到服务器的功能
{
protected:
	uint64_t t;

public:
	void RecordTime();
	void RecordTime(uint64_t time) //外部传入时间
	{
		t = time;
	}

	void LogToNet(const std::string& index3, const std::string& index1 = "", const std::string& index2 = ""); 
	//index3必须指定，前两者可以留空(前两者留空的话必须事先发过Login包), 但要有的话必须一起有
};

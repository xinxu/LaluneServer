#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/asio/placeholders.hpp>
#include "ptime2.h"
#include "../protobuf/log/log.pb.h"
#include "NetLib/NetLib.h"
#include "internal.h"
#include "BasicLogStream.h"
#include "HeaderDefine.h"

uint64_t __last_t = 0;
boost::mutex __last_t_mutex;

void BasicLogStream::RecordTime()
{	
	//如果精度不要求那么高，获取时间这里可以优化，用一个初始值和GetTickCount相加。(也优化不掉多少)
	t = ptime2(boost::posix_time::microsec_clock::local_time()).get_u64();

	{
		boost::lock_guard<boost::mutex> lock(__last_t_mutex);
		if (t <= __last_t)
		{
			t = __last_t + 1;
		}
		__last_t = t;
	}
}

void BasicLogStream::LogToNet(const std::string& index3, const std::string& index1, const std::string& index2)
{
	kit::log::LogDetail detail_pb;
	for (auto it = lexical_entries.begin(); it != lexical_entries.end(); ++it)
	{			
		kit::log::LexicalEntry* entry = detail_pb.add_entry();
		if (it->has_name())
		{
			entry->set_entry_name(it->name.v);
		}
		entry->set_value_type(it->value_type);
		switch (it->value_type)
		{
			case LexicalEntryValue::int_value_type:
				entry->set_i(it->ivalue);
				break;
			case LexicalEntryValue::uint_value_type:
			case LexicalEntryValue::uint_value_type_display_as_hex:
			case LexicalEntryValue::uint16_value_type_display_as_hex:
				entry->set_u(it->ivalue);
				break;
			case LexicalEntryValue::string_value_type:				
			case LexicalEntryValue::wstring_value_type:
				entry->set_s(it->svalue);
				break;
			case LexicalEntryValue::bytes_value_type_no_display:
			case LexicalEntryValue::bytes_value_type_display_as_hex:
				entry->set_b(it->bytes, it->bytes_length);
				break;
			case LexicalEntryValue::double_value_type:
				entry->set_d(it->dvalue);
				break;
			default:
				break;
		}
	}

	kit::log::LogEvent pb;
	if (!index1.empty())
	{
		pb.set_index1(index1);
		pb.set_index2(index2);
	}
	pb.set_index3(index3);
	pb.set_time(t);
	pb.set_data(detail_pb.SerializeAsString());

	//TODO TOMODIFY
	/*
	int pb_size = pb.ByteSize();
	char* send_buf = new char[CMDEX0_HEAD_SIZE + pb_size];
	CMD_SIZE(send_buf)			= CMDEX0_HEAD_SIZE + pb_size;					// 数据包的字节数（含msghead）
	CMD_FLAG(send_buf)			= 0;							// 标志位，表示数据是否压缩、加密等
	CMD_CAT(send_buf)			= CAT_LOGSVR;				// 命令分类
	CMD_ID(send_buf)			= ID_LOGSVR_LOG;

	pb.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)CMDEX0_DATA(send_buf));

	//TODO: 改成1000条换一次之类的
	std::shared_ptr<NetLibPlus_Client> c = NetLibPlus_get_first_Client(__LS_ServerTypeName.c_str());
	if (c) 
	{
		c->SendAsync(send_buf);
	}
	else
	{
		delete[] send_buf;
	}
	*/
}
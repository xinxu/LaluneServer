#ifndef _LOG_DLL_

#include "../protobuf/log/log.pb.h"
#include "../include/ptime2.h"
#include "../NetLib/NetLib.h"
#include "HeaderDefine.h"
#include "to_LS_ClientDelegate.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include "LogQuery.h"

std::map<int, std::shared_ptr<ResultHandler>> result_handlers;
int current_trans_id = 0;
boost::mutex _log_query_result_handlers_mutex;

int new_trans_id()
{
	boost::lock_guard<boost::mutex> lock(_log_query_result_handlers_mutex);
	return ++ current_trans_id;
}

void AsStringResultHandler::ToString(const kit::log::LogEvent& log_event, std::ostringstream& os)
{
	os << "[" << time_utility::ptime_to_string_full(log_event.time()) << "] [" << log_event.index1() << "] [" << log_event.index2() << "] [" << log_event.index3() << "] ";

	bool last_entry_has_description = false;
	
	kit::log::LogDetail detail_pb;
	if (! detail_pb.ParseFromString(log_event.data()))
	{
		os << "{ data corrupt }";
	}

	for (auto it = detail_pb.entry().begin(); it != detail_pb.entry().end(); ++it)
	{
		if (it->has_entry_name())
		{
			if (last_entry_has_description)
			{
				os << ", "; //连续的带名字项用逗号隔开
			}
			else
			{
				last_entry_has_description = true;
			}
			os << it->entry_name() << " = ";
		}
		else
		{
			last_entry_has_description = false;
		}

		switch (it->value_type())
		{
			case LexicalEntryValue::int_value_type:
				os << it->i();
				break;
			case LexicalEntryValue::uint_value_type:
				os << it->u();
				break;
			case LexicalEntryValue::string_value_type:				
				os << it->s();
				break;
#ifdef WIN32
			case LexicalEntryValue::wstring_value_type:
				os << LexicalEntry::UnicodeToAscii(LexicalEntry::UTF8ToUnicode(it->s()));
				break;
#endif
			case LexicalEntryValue::uint16_value_type_display_as_hex:
				os << "0x" << std::hex << (uint16_t)it->u() << std::dec;
				break;
			case LexicalEntryValue::uint_value_type_display_as_hex:
				os << "0x" << std::hex << it->u() << std::dec;
				break;
			case LexicalEntryValue::bytes_value_type_no_display:						
			case LexicalEntryValue::bytes_value_type_display_as_hex:
				{
					const std::string& bytes = it->b();
					os << "{ bytes length: " << bytes.size() << " }";
					if (it->value_type() == LexicalEntryValue::bytes_value_type_display_as_hex)
					{
						os << std::endl;
						#define BYTES_PER_LINE (16)
						char buf[4];
						buf[3] = 0;
						for (unsigned int i = 0; i < bytes.size(); ++i)
						{
							sprintf(buf, " %02x", (uint8_t)bytes[i]);
							os << buf;
							if (i % BYTES_PER_LINE == BYTES_PER_LINE - 1 && i != bytes.size() - 1) //最后一行不打换行
							{
								os << std::endl;
							}
						}
					}
				}
				break;
			case LexicalEntryValue::double_value_type:
				os << it->d();
				break;
			default:
				break;
		}
	}
}

void AsStringResultHandler::handle_result(const kit::log::LogEvent& log_event)
{
	std::ostringstream os;
	AsStringResultHandler::ToString(log_event, os);
	handle_string(os.str());
}

void AsDataResultHandler::handle_result(const kit::log::LogEvent& log_event)
{
	LogDataReader reader(log_event.data(), m_enable_find_by_name);
	handle_data(log_event.index1(), log_event.index2(), log_event.index3(), log_event.time(), reader);
}

void SendQuery(std::shared_ptr<ResultHandler> rh, uint16_t CmdID, const kit::log::Query& pb)
{
	int pb_size = pb.ByteSize();

	char* send_buf = new char[CMDEX0_HEAD_SIZE + pb_size];
	CMD_SIZE(send_buf)			= CMDEX0_HEAD_SIZE + pb_size;					// 数据包的字节数（含msghead）
	CMD_FLAG(send_buf)			= 0;							// 标志位，表示数据是否压缩、加密等
	CMD_CAT(send_buf)			= CAT_LOGSVR;				// 命令分类
	CMD_ID(send_buf)			= CmdID;
	int tid	= new_trans_id();
	CMDEX0_TRANSID(send_buf)	= tid;
	{
		boost::lock_guard<boost::mutex> lock(_log_query_result_handlers_mutex);
		result_handlers[tid] = rh;
	}

	pb.SerializeWithCachedSizesToArray((google_lalune::protobuf::uint8*)CMDEX0_DATA(send_buf));
	
	//TODO TOMODIFY
	/*
	std::shared_ptr<NetLibPlus_Client> ls_client = NetLibPlus_get_first_Client(__LS_ServerTypeNameForQuery.c_str());

	if (ls_client)
	{
		ls_client->SendAsync(send_buf);
	}
	else
	{
		delete[] send_buf;
	}*/
}

void Query123(std::shared_ptr<ResultHandler> rh, const std::string& index1, const std::string& index2, const std::string& index3, bool no_sort, uint64_t begin_time, uint64_t end_time)
{
	kit::log::Query pb;
	pb.set_index1(index1);
	pb.set_index2(index2);
	pb.set_index3(index3);
	if (begin_time != TIME64_NEGATIVE_INFINITY)
	{
		pb.set_begin_time(begin_time);
	}
	if (end_time != TIME64_POSITIVE_INFINITY)
	{
		pb.set_end_time(end_time);
	}
	if (no_sort)
	{
		pb.set_no_sort(no_sort);
	}
	//optional string condition_clause= 6;
	//optional int32	is_count	  = 7;

	SendQuery(rh, ID_LOGSVR_QUERY_123, pb);
}

void Query123p(std::shared_ptr<ResultHandler> rh, const std::string& index1, const std::string& index2, const std::string& index3_prefix, bool no_sort, uint64_t begin_time, uint64_t end_time)
{
	kit::log::Query pb;
	pb.set_index1(index1);
	pb.set_index2(index2);
	pb.set_index3(index3_prefix);
	if (begin_time != TIME64_NEGATIVE_INFINITY)
	{
		pb.set_begin_time(begin_time);
	}
	if (end_time != TIME64_POSITIVE_INFINITY)
	{
		pb.set_end_time(end_time);
	}
	if (no_sort)
	{
		pb.set_no_sort(no_sort);
	}

	SendQuery(rh, ID_LOGSVR_QUERY_123p, pb);
}

void Query13(std::shared_ptr<ResultHandler> rh, const std::string& index1, const std::string& index3, bool no_sort, uint64_t begin_time, uint64_t end_time)
{
	kit::log::Query pb;
	pb.set_index1(index1);
	pb.set_index3(index3);
	if (begin_time != TIME64_NEGATIVE_INFINITY)
	{
		pb.set_begin_time(begin_time);
	}
	if (end_time != TIME64_POSITIVE_INFINITY)
	{
		pb.set_end_time(end_time);
	}
	if (no_sort)
	{
		pb.set_no_sort(no_sort);
	}

	SendQuery(rh, ID_LOGSVR_QUERY_13, pb);
}

void Query13p(std::shared_ptr<ResultHandler> rh, const std::string& index1, const std::string& index3_prefix, bool no_sort, uint64_t begin_time, uint64_t end_time)
{
	kit::log::Query pb;
	pb.set_index1(index1);
	pb.set_index3(index3_prefix);
	if (begin_time != TIME64_NEGATIVE_INFINITY)
	{
		pb.set_begin_time(begin_time);
	}
	if (end_time != TIME64_POSITIVE_INFINITY)
	{
		pb.set_end_time(end_time);
	}
	if (no_sort)
	{
		pb.set_no_sort(no_sort);
	}

	SendQuery(rh, ID_LOGSVR_QUERY_13p, pb);
}

void Query1p3p(std::shared_ptr<ResultHandler> rh, const std::string& index1_prefix, const std::string& index3_prefix, bool no_sort, uint64_t begin_time, uint64_t end_time)
{
	kit::log::Query pb;
	pb.set_index1(index1_prefix);
	pb.set_index3(index3_prefix);
	if (begin_time != TIME64_NEGATIVE_INFINITY)
	{
		pb.set_begin_time(begin_time);
	}
	if (end_time != TIME64_POSITIVE_INFINITY)
	{
		pb.set_end_time(end_time);
	}
	if (no_sort)
	{
		pb.set_no_sort(no_sort);
	}

	SendQuery(rh, ID_LOGSVR_QUERY_1p3p, pb);
}

void Query1p3(std::shared_ptr<ResultHandler> rh, const std::string& index1_prefix, const std::string& index3, bool no_sort, uint64_t begin_time, uint64_t end_time)
{
	kit::log::Query pb;
	pb.set_index1(index1_prefix);
	pb.set_index3(index3);
	if (begin_time != TIME64_NEGATIVE_INFINITY)
	{
		pb.set_begin_time(begin_time);
	}
	if (end_time != TIME64_POSITIVE_INFINITY)
	{
		pb.set_end_time(end_time);
	}
	if (no_sort)
	{
		pb.set_no_sort(no_sort);
	}

	SendQuery(rh, ID_LOGSVR_QUERY_1p3, pb);
}

#endif
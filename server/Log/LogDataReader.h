#pragma once

#ifndef _LOG_DLL_

#include <string>
#include <stdint.h>
#include <map>
#include "LexicalEntryValueType.h"
#include <sstream>
#include "../protobuf/log/log.pb.h"
#include "LexicalEntry.h"

class LogDataReader
{
protected:
	kit::log::LogDetail detail;
	std::map<std::string, const kit::log::LexicalEntry*> name_map;

public:
	LogDataReader(const std::string& data, bool enable_find_by_name = true);

	inline int EntriesCount() const
	{
		return detail.entry_size();
	}

	//在LogDataReader这一层，
	//GetType(int index)无越界检查(protobuf那里,debug版有检查，release版无检查)，
	//GetType(const std::string& name)有越界检查，如果没有名字为name的，则返回默认值。如果有以name为名的，但类型不对，则返回错误的类型的默认值

	inline LexicalEntryValue::value_type_enum GetType(int index) const
	{
		return static_cast<LexicalEntryValue::value_type_enum>(detail.entry(index).value_type());
	}
	inline LexicalEntryValue::value_type_enum GetType(const std::string& name, LexicalEntryValue::value_type_enum default_value = LexicalEntryValue::undefined_value_type) const
	{
		auto it = name_map.find(name);
		if (it != name_map.end())
		{
			return static_cast<LexicalEntryValue::value_type_enum>(it->second->value_type());
		}
		else
		{
			return default_value;
		}
	}

	inline bool Contains(const std::string& name) const
	{
		return name_map.count(name) > 0;
	}

	inline bool HasName(int index) const
	{
		return detail.entry(index).has_entry_name();
	}

	inline const std::string& GetName(int index) const
	{
		return detail.entry(index).entry_name();
	}

	inline int64_t GetInt(int index) const
	{
		return detail.entry(index).i();
	}
	inline int64_t GetInt(const std::string& name, int64_t default_value = 0) const
	{
		auto it = name_map.find(name);
		if (it != name_map.end())
		{
			return it->second->i();
		}
		else
		{
			return default_value;
		}
	}

	inline uint64_t GetUInt(int index) const
	{
		return detail.entry(index).u();
	}
	inline uint64_t GetUInt(const std::string& name, uint64_t default_value = 0) const
	{
		auto it = name_map.find(name);
		if (it != name_map.end())
		{
			return it->second->u();
		}
		else
		{
			return default_value;
		}
	}

	inline int64_t ToInt(int index, bool& not_integer_type) const
	{
		switch (detail.entry(index).value_type())
		{
			case LexicalEntryValue::int_value_type:
				return detail.entry(index).i();
			case LexicalEntryValue::uint_value_type:
			case LexicalEntryValue::uint16_value_type_display_as_hex:
			case LexicalEntryValue::uint_value_type_display_as_hex:
				return detail.entry(index).u();
			default:
				not_integer_type = true;
				return 0;
		}
	}

	inline int64_t ToInt(const std::string& name, bool& not_integer_type, int64_t default_value = 0) const
	{
		auto it = name_map.find(name);
		if (it == name_map.end())
		{
			return default_value;
		}
		else
		{
			switch (it->second->value_type())
			{
				case LexicalEntryValue::int_value_type:
					return it->second->i();
				case LexicalEntryValue::uint_value_type:
				case LexicalEntryValue::uint16_value_type_display_as_hex:
				case LexicalEntryValue::uint_value_type_display_as_hex:
					return it->second->u();
				default:
					not_integer_type = true;
					return 0;
			}
		}
	}

	inline std::string ConvertToStr(int index) const
	{		
		std::ostringstream os;
		switch (detail.entry(index).value_type())
		{
			case LexicalEntryValue::int_value_type:
				os << detail.entry(index).i();
				break;
			case LexicalEntryValue::uint_value_type:
				os << detail.entry(index).u();
				break;
			case LexicalEntryValue::string_value_type:				
				return detail.entry(index).s();
#ifdef WIN32
			case LexicalEntryValue::wstring_value_type:
				return LexicalEntry::UnicodeToAscii(LexicalEntry::UTF8ToUnicode(detail.entry(index).s()));
				break;
#endif
			case LexicalEntryValue::uint16_value_type_display_as_hex:
				os << "0x" << std::hex << (uint16_t)detail.entry(index).u() << std::dec;
				break;
			case LexicalEntryValue::uint_value_type_display_as_hex:
				os << "0x" << std::hex << detail.entry(index).u() << std::dec;
				break;
			case LexicalEntryValue::bytes_value_type_no_display:						
			case LexicalEntryValue::bytes_value_type_display_as_hex:
				return detail.entry(index).b();
				break;
			case LexicalEntryValue::double_value_type:
				os << detail.entry(index).d();
				break;
			default:
				break;
		}

		return os.str();
	}

	inline std::string ConvertToStr(const std::string& name, const std::string& default_value = "") const
	{		
		auto it = name_map.find(name);
		if (it != name_map.end())
		{
			std::ostringstream os;
			switch (it->second->value_type())
			{
				case LexicalEntryValue::int_value_type:
					os << it->second->i();
					break;
				case LexicalEntryValue::uint_value_type:
					os << it->second->u();
					break;
				case LexicalEntryValue::string_value_type:				
					return it->second->s();
	#ifdef WIN32
				case LexicalEntryValue::wstring_value_type:
					return LexicalEntry::UnicodeToAscii(LexicalEntry::UTF8ToUnicode(it->second->s()));
					break;
	#endif
				case LexicalEntryValue::uint16_value_type_display_as_hex:
					os << "0x" << std::hex << (uint16_t)it->second->u() << std::dec;
					break;
				case LexicalEntryValue::uint_value_type_display_as_hex:
					os << "0x" << std::hex << it->second->u() << std::dec;
					break;
				case LexicalEntryValue::bytes_value_type_no_display:						
				case LexicalEntryValue::bytes_value_type_display_as_hex:
					return it->second->b();
					break;
				case LexicalEntryValue::double_value_type:
					os << it->second->d();
					break;
				default:
					break;
			}

			return os.str();
		}
		else
		{
			return default_value;
		}
	}

	inline const std::string& GetStr(int index) const
	{		
		return detail.entry(index).s();
	}
	inline const std::string& GetStr(const std::string& name, const std::string& default_value = "") const
	{
		auto it = name_map.find(name);
		if (it != name_map.end())
		{
			return it->second->s();
		}
		else
		{
			return default_value;
		}
	}

	inline const std::string& GetBytes(int index) const
	{
		return detail.entry(index).b();
	}
	inline const std::string& GetBytes(const std::string& name, const std::string& default_value = "") const
	{
		auto it = name_map.find(name);
		if (it != name_map.end())
		{
			return it->second->b();
		}
		else
		{
			return default_value;
		}
	}

	inline const std::string& ToStr(int index, bool& not_string_type) const
	{
		switch (detail.entry(index).value_type())
		{
			case LexicalEntryValue::string_value_type:
				return detail.entry(index).s();
			case LexicalEntryValue::bytes_value_type_no_display:
			case LexicalEntryValue::bytes_value_type_display_as_hex:
				return detail.entry(index).b();
			default:
				not_string_type = true;
				return detail.entry(index).s();//一般就是空字符串
		}
	}

	inline const std::string& ToStr(const std::string& name, bool& not_string_type, const std::string& default_value = "") const
	{
		auto it = name_map.find(name);
		if (it == name_map.end())
		{
			return default_value;
		}
		else
		{
			switch (it->second->value_type())
			{
				case LexicalEntryValue::string_value_type:
					return it->second->s();
				case LexicalEntryValue::bytes_value_type_no_display:
				case LexicalEntryValue::bytes_value_type_display_as_hex:
					return it->second->b();
				default:
					not_string_type = true;
					return it->second->s();
			}
		}
	}

	inline double GetDouble(int index) const
	{
		return detail.entry(index).d();
	}
	inline double GetDouble(const std::string& name, double default_value = 0.0) const
	{
		auto it = name_map.find(name);
		if (it != name_map.end())
		{
			return it->second->d();
		}
		else
		{
			return default_value;
		}
	}
};

#endif
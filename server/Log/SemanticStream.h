#pragma once

#include <vector>
#include <iostream>
#include <sstream>
#include <stdint.h>
#include "LogValue.h"
#include "LexicalEntry.h"

class SemanticStream
{
protected:
	std::vector<LexicalEntry> lexical_entries;
	log_::EntryName name;

	std::string ToString() const;

public:
	SemanticStream()
	{
	}

	SemanticStream& operator << (const log_::EntryName& _name)
	{
		name = _name;
		return *this;
	}

	inline void PushEntry(LexicalEntry&& entry)
	{
		lexical_entries.push_back(entry);
	}

	inline void PushLogValue(int32_t v)
	{
		PushEntry(LexicalEntry(LexicalEntryValue::int_value_type, v));
	}

	inline void PushLogValue(int64_t v)
	{
		PushEntry(LexicalEntry(LexicalEntryValue::int_value_type, v));
	}	
	
	inline void PushLogValue(uint32_t v)
	{
		PushEntry(LexicalEntry(LexicalEntryValue::uint_value_type, v));
	}
	
	template <typename T>
	inline void PushLogValue(T v)
	{
		PushEntry(LexicalEntry(LexicalEntryValue::uint_value_type, v));
	}

	inline void PushLogValue(uint64_t v)
	{
		PushEntry(LexicalEntry(LexicalEntryValue::uint_value_type, v));
	}

	inline void PushLogValue(log_::h v)
	{
		PushEntry(LexicalEntry(LexicalEntryValue::uint_value_type_display_as_hex, v.v));
	}

	inline void PushLogValue(log_::h16 v)
	{
		PushEntry(LexicalEntry(LexicalEntryValue::uint16_value_type_display_as_hex, v.v));
	}

	inline void PushLogValue(char* v)
	{
		PushEntry(LexicalEntry(v));
	}

	inline void PushLogValue(const char* v)
	{
		PushEntry(LexicalEntry(v));
	}

#ifdef WIN32
	inline void PushLogValue(wchar_t* v)
	{
		PushEntry(LexicalEntry(v));
	}

	inline void PushLogValue(const wchar_t* v)
	{
		PushEntry(LexicalEntry(v));
	}
#endif

	inline void PushLogValue(std::string& v)
	{
		PushEntry(LexicalEntry(v));
	}

	inline void PushLogValue(const std::string& v)
	{
		PushEntry(LexicalEntry(v));
	}

#ifdef WIN32
	inline void PushLogValue(std::wstring& v)
	{
		PushEntry(LexicalEntry(v));
	}

	inline void PushLogValue(const std::wstring& v)
	{
		PushEntry(LexicalEntry(v));
	}
#endif

	inline void PushLogValue(log_::b v)
	{
		PushEntry(LexicalEntry(LexicalEntryValue::bytes_value_type_no_display, v.v, v.l));
	}

	inline void PushLogValue(log_::bd v)
	{
		PushEntry(LexicalEntry(LexicalEntryValue::bytes_value_type_display_as_hex, v.v, v.l));
	}	
	
	inline void PushLogValue(double v)
	{
		PushEntry(LexicalEntry(v));
	}

	template <typename T>
	SemanticStream& operator << (const T& v)
	{
		PushLogValue(v);
		if (! name.empty())
		{
			lexical_entries.back().name = name;
			name.reset();
		}
		return *this;
	}
};
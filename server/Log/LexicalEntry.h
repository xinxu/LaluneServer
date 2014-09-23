#pragma once

#include "EntryName.h"
#include "LexicalEntryValueType.h"
#include <string>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

class LexicalEntry
{
public:
	log_::EntryName name;	

	LexicalEntryValue::value_type_enum value_type;

	uint64_t ivalue;
	const char* bytes;
	int bytes_length;
	double dvalue;
	std::string svalue;

	bool has_name() const
	{
		return ! name.empty();
	}

	LexicalEntry(LexicalEntryValue::value_type_enum vt, uint64_t i) : value_type(vt), ivalue(i)
	{
	}

	LexicalEntry(const char* s) : value_type(LexicalEntryValue::string_value_type), svalue(s)
	{
	}

#ifdef WIN32
	LexicalEntry(const wchar_t* s) : value_type(LexicalEntryValue::wstring_value_type)
	{
		int nNeedChars = WideCharToMultiByte( CP_UTF8, 0, s, -1, 0, 0, 0, 0 );
		if (nNeedChars > 0)//再次判断一下
		{	
			svalue.resize(nNeedChars);
			::WideCharToMultiByte( CP_UTF8, 0, s, -1, (char*)svalue.c_str(), nNeedChars, 0, 0 );
			svalue.erase(svalue.size() - 1); //最后一位是\n，是string所不需要的
		}
	}
#endif

	LexicalEntry(const std::string& s) : value_type(LexicalEntryValue::string_value_type), svalue(s)
	{
	}

#ifdef WIN32
	LexicalEntry(const std::wstring& s) : value_type(LexicalEntryValue::wstring_value_type)
	{
		int nNeedChars = WideCharToMultiByte( CP_UTF8, 0, s.c_str(), -1, 0, 0, 0, 0 );
		if (nNeedChars > 0)//再次判断一下
		{	
			svalue.resize(nNeedChars);
			::WideCharToMultiByte( CP_UTF8, 0, s.c_str(), -1, (char*)svalue.c_str(), nNeedChars, 0, 0 );
			svalue.erase(svalue.size() - 1); //最后一位是\n，是string所不需要的
		}
	}

	static std::wstring UTF8ToUnicode(const std::string& svalue)
	{
		int nNeedWchars = MultiByteToWideChar( CP_UTF8, 0, svalue.c_str(), -1, NULL, 0 );
		if (nNeedWchars > 0)
		{
			std::wstring ret(nNeedWchars, 0);
			::MultiByteToWideChar( CP_UTF8, 0, svalue.c_str(), -1, (wchar_t*)ret.c_str(), nNeedWchars );
			return ret.substr(0, ret.size() - 1); //最后一位是\n，是string所不需要的
		}

		return std::wstring();
	}

	static std::string UnicodeToAscii( const std::wstring& in_str )
	{
		int nNeedChars = WideCharToMultiByte( CP_ACP, 0, in_str.c_str(), -1, 0, 0, 0, 0 );
		if (nNeedChars > 0)//再次判断一下
		{
			std::string ret(nNeedChars, 0);
			::WideCharToMultiByte( CP_ACP, 0, in_str.c_str(), -1, (char*)ret.c_str(), nNeedChars, 0, 0 );
			return ret.substr(0, ret.size() - 1); //最后一位是\n，是string所不需要的
		}

		return std::string();
	}
#endif

	LexicalEntry(LexicalEntryValue::value_type_enum vt, const char* _bytes, int length) : value_type(vt), bytes(_bytes), bytes_length(length)
	{
	}	
	
	LexicalEntry(double d) : value_type(LexicalEntryValue::double_value_type), dvalue(d)
	{
	}
};
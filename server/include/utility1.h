#pragma once

#include <string>
#include <vector>

class utility1
{
public:
	static int str2int(const char* str, int default_value = 0)
	{
		int v = default_value;
		sscanf(str, "%d", &v);
		return v;
	}

	static std::string int2str(int iValue)
	{
		char buf[30];
		sprintf(buf, "%d", iValue);
		return std::string(buf);
	}

	static void split(const std::string& in_str, std::vector<std::string>& out, char split_char = '/', unsigned int start_pos = 0)
	{
		unsigned int word_begin = start_pos;
		for (unsigned int i = start_pos; i < in_str.size(); ++i)
		{
			if (in_str[i] == split_char)
			{
				out.push_back(in_str.substr(word_begin, i - word_begin));
				word_begin = i + 1;
			}
		}
		if (word_begin <= in_str.size())
		{
			out.push_back(in_str.substr(word_begin, in_str.size() - word_begin));
		}
	}

	static std::string generateRandomString(int len = 8)
	{
		std::string ret(len, ' ');
		for (int i = 0; i < len; ++i)
		{
			int r = rand() % 62;
			if (r < 10)
			{
				ret[i] = '0' + r;
			}
			else if (r < 36)
			{
				ret[i] = 'a' + r - 10;
			}
			else
			{
				ret[i] = 'A' + r - 36;
			}
		}
		return ret;
	}
};

template<typename T>
class AvailableIDs
{
protected:
	T _next_id = 1;
	std::vector<T> _available_ids;

public:
	T getId()
	{
		if (_available_ids.size())
		{
			int _id = _available_ids.back();
			_available_ids.pop_back();
			return _id;
		}
		else
		{
			return _next_id++;
		}
	}

	void releaseId(T _id)
	{
		if (_id + 1 == _next_id)
		{
			_next_id--;
		}
		else
		{
			_available_ids.push_back(_id);
		}
	}
};
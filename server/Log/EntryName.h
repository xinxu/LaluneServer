#pragma once

#include <string>

namespace log_
{
	struct EntryName
	{
		std::string v;

		EntryName()
		{
		}

		EntryName(const char* _name) : v(_name)
		{
		}

		EntryName(const std::string& _name) : v(_name)
		{
		}

		bool empty() const
		{
			return v.empty();
		}

		void reset()
		{
			v.clear();
		}
	};
	typedef EntryName n;
}
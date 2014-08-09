#pragma once

#include <string>
#include <iostream>
#include <sstream>

inline std::string InputStr(std::string description, std::string default_value)
{
	std::cout << description << " [" << default_value << "] ";

	std::string ret;
	std::getline(std::cin, ret);
	if (ret.empty())
	{
		return default_value;
	}
	else
	{
		return ret;
	}
}

template <typename InputType>
InputType Input(std::string description, InputType default_value)
{
	std::cout << description << " [" << default_value << "] ";

	std::string line;
	std::getline(std::cin, line);
	if (line.empty())
	{
		return default_value;
	}
	else
	{
		std::istringstream sin(line);
		InputType ret;
		sin >> ret;
		return ret;
	}
}

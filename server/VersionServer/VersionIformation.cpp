#include "VersionIformation.h"
vector<string> VersionIformation::GetVerctor(const string &str)
{
	string str_temp;
	vector<string> vector_temp;
	int i;
	str_temp = "";
	for (i = 0; i < str.size(); i++)
	{
		if (str[i] == '|')
		{
			vector_temp.push_back(str_temp);
			str_temp = "";
		}
		else
		{
			str_temp += str[i];
		}
	}
	return vector_temp;
}
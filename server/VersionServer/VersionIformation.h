#ifndef __VERSION_IFORMATION_
#define __VERSION_IFORMATION_
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/date_time.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <vector>
#include <boost/foreach.hpp>
using namespace boost::property_tree;
using namespace boost::gregorian;
using namespace boost;
using namespace std;
class VersionIformation
{
public:
	 vector<string> file_path;//需要更新的文件名
	 vector<string> file_url;//需要更新的文件路径
	 string now_version;//现在的版本名
	 string next_version;//下一个的版本名
	static vector<string> GetVerctor(const string &str);//将长字符串分解



};
extern vector<VersionIformation> version_infor;
#endif

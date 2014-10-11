#ifndef __UPADATE_VERSION_H
#define __UPADATE_VERSION_H

#include <string>
#include <iostream>
#include <vector>
#include "Version.pb.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/date_time.hpp>

#include <boost/foreach.hpp>
using namespace boost::property_tree;
using namespace boost::gregorian;
using namespace boost;
using namespace std;
class UpdateVersion
{
public:
	UpdateVersion();
	~UpdateVersion();
	bool SendIformation(const string &file_information);//发送信息给controlserver
	bool Uploading();//上传全部文件
	string UploadingOne(const string name);//上传一个文件，返回url
	bool Input();
	bool DelFile();//删除所有文件
	string op_command;

private:
	vector<string> file_name;//需要更新或删除的文件名
	vector<string> file_url;//需要更新的文件路径，空表示删除
	string pro_version;//前一个版本名
	string now_version;//现在的版本名
	string common_url;


};



#endif
#include "VersionServerSessionDelegate.h"
#include "ServerCommon.h"
#include "MessageTypeDef.h"
#include "Log/Log.h"
#include "Version.pb.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/date_time.hpp>
#include "VersionIformation.h"
#include <map>

void VersionServerSessionDelegate::RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
{
	if (SERVER_MSG_LENGTH(data) >= SERVER_MSG_HEADER_BASE_SIZE)
	{
		switch (SERVER_MSG_TYPE(data))
		{
		case MSG_CHECK_VERSION:   //客户端请求版本更新
		{
								  //  lalune::CheckVersionResult result;

								  // lalune::VersionFile* file = result.add_file();


								  //TODO
			
								  uint32_t op_id = 0;
								  lalune::CheckVersion now_version;
						
								  if (ParseMsgOpId(data, op_id, now_version))
								  {
							
									  int i, j;
									//  cout << version_infor.size() << endl;
									//  now_version.set_version_name("1.0");
									  lalune::CheckVersionResult response;
									
									  map<string, string> map_node;
									 
									  response.set_now_version(now_version.version_name());
									  for (i = 0; i < version_infor.size(); i++)
									  {
										  if (now_version.version_name() == version_infor[i].now_version)
										  {
											  
											  for (j = i; j < version_infor.size(); j++)
											  {
												  if (response.now_version() == version_infor[j].now_version)
												  {
													  response.set_now_version(version_infor[j].next_version);
													  int k;
													  for (k = 0; k < version_infor[j].file_path.size(); k++)
													  {
														  map_node[version_infor[j].file_path[k]] = version_infor[j].file_url[k];
													  }
													  

												  }
											  }
											  break;
										  }
									  }


									  lalune::VersionFile *file_temp;
								
									  auto temp_node = map_node.begin();
									  for (; temp_node != map_node.end(); temp_node++)
									  {
										  file_temp = response.add_file();
										  file_temp->set_file_path(temp_node->first);
										  file_temp->set_url_prefix(temp_node->second);
									  }
									  LOGEVENTL("INFO", _ln("now_version") << now_version.version_name()<< _ln("new_version")<<response.now_version());
									
									/*  for (i = 0; i < response.file_size(); i++)
									  {
										  cout << response.file(i).file_path() << endl;
										  cout << response.file(i).url_prefix() << endl;
									  }*/
									  
									  ReplyMsgOpId(sessionptr, MSG_CHECK_VERSION_RESULT, op_id, response);
								  }
								  else
								  {
									  LOGEVENTL("info", "protobuf error");
								  }
						

								//	  cout << response.now_version() << endl;
								  break;
		}
		default:
			LOGEVENTL("WARN", "unrecognized message type. " << SERVER_MSG_TYPE(data));
			break;
		}
	}
	else
	{
		LOGEVENTL("WARN", "message length not enough: " << SERVER_MSG_LENGTH(data));
	}
}
lalune::CheckVersionResult VersionServerSessionDelegate::GetResult(lalune::CheckVersion auto_register)
{
	lalune::CheckVersionResult response;

	return response;

}
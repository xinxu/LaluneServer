#include "VersionServerSessionDelegate.h"
#include "ServerCommon.h"
#include "MessageTypeDef.h"
#include "Log/Log.h"
#include "Version.pb.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/date_time.hpp>
#include "VersionIformation.h"
#include <set>
void VersionServerSessionDelegate::RecvFinishHandler(NetLib_ServerSession_ptr sessionptr, char* data)
{
	if (SERVER_MSG_LENGTH(data) >= SERVER_MSG_HEADER_BASE_SIZE)
	{
		switch (SERVER_MSG_TYPE(data))
		{
		case MSG_CHECK_VERSION:
		{
								 //  lalune::CheckVersionResult result;

								 // lalune::VersionFile* file = result.add_file();
								 
								  
								  //TODO
								  uint32_t op_id = 0;
								  lalune::CheckVersion now_version;
								  vector<VersionIformation> version_infor;
								  if (ParseMsgOpId(data, op_id, now_version))
								  {
									  int i, j;
									  lalune::CheckVersionResult response;
									  set<string> set_path;
									  set<string> set_url;
									  response.set_now_version( now_version.version_name());
									  response.set_url_prefix("");
									  for (i = 0; i < version_infor.size(); i++)
									  {
										  if (now_version.version_name() == version_infor[i].now_version)
										  {
											 // string version_temp;
											 // version_temp = now_version.version_name;
											  for (j = i; j < version_infor.size(); j++)
											  {
												  if (response.now_version() == version_infor[j].now_version)
												  {
													  response.set_now_version( version_infor[j].next_version);
													  int k;
													  for (k = 0; k < version_infor[j].file_path.size(); k++)
													  {
														  set_path.insert(version_infor[j].file_path[k]);
													  }
													  for (k = 0; k < version_infor[j].file_url.size(); k++)
													  {
														  set_url.insert(version_infor[j].file_url[k]);
													  }

												  }
											  }
											  break;
										  }
									  }
									  lalune::VersionFile *file_temp;
									  vector<string> vector_path;
									  vector<string> vector_url;
									  auto temp_path=set_path.begin();
									  for (; temp_path != set_path.end(); temp_path++)
									  {
										  vector_path.push_back(*temp_path);
									  }
									  auto temp_url = set_url.begin();
									  for (; temp_url != set_url.end(); temp_url++)
									  {
										  vector_url.push_back(*temp_url);
									  }
									  for (i = 0; i < vector_path.size(); i++)
									  {
										  file_temp = response.add_file();
										  file_temp->set_file_path ( vector_path[i]);
										  file_temp->set_url_prefix ( vector_url[i]);
									  }
									  ReplyMsgOpId(sessionptr, MSG_CHECK_VERSION_RESULT, op_id, response);
								  }
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
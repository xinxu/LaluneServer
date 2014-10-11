#include "UpdateVersion.h"
int main()
{
	UpdateVersion Up;
	//Up = new UpdateVersion();
	Up.Input();
	if (Up.op_command == "add")
	{
		Up.Uploading();
		Up.SendIformation();
	}
	else if (Up.op_command == "del")
	{
		Up.DelFile();
		Up.SendIformation();
	}
	//delete Up;
	return 0;
}
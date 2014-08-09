#pragma once

#include "Log_Dll.h"

#ifndef _LOG_DLL_

#ifdef _DEBUG
#define _LOG_LIBNAME_ "Logd"
#else
#define _LOG_LIBNAME_ "Log"
#endif

#pragma comment(lib, _LOG_LIBNAME_)  //this is not cross-platform. you should write Makefile on linux.

#endif

#include "LogStream.h"
#include <sstream>

#ifdef WIN32
#define _FUNCTION_AND_LINE_ log_::n("Function") << __FUNCTION__ << log_::n("Line") << __LINE__ << ", "
#define _FUNCTION_AND_LINE_NOCOMMA log_::n("Function") << __FUNCTION__ << log_::n("Line") << __LINE__
#else
#define _FUNCTION_AND_LINE_ log_::n("Function") << __func__ << log_::n("Line") << __LINE__ << ", "
#define _FUNCTION_AND_LINE_NOCOMMA log_::n("Function") << __func__ << log_::n("Line") << __LINE__
#endif

#define _ln(NAME) (log_::n(NAME))

#define _LOG(I1, I2, I3, C) \
{							\
	BasicLogStream ___bls;  \
	___bls << C;			\
	___bls.RecordTime();	\
	___bls.LogToNet(I3, I1, I2);	\
}

//I1，I2, I3确定时，T要保证递增顺序。所以最好整套环境中同时只有一个人对一个(I1, I2, I3)进行写入
#define _LOGT(I1, I2, I3, T, C) \
{							\
	BasicLogStream ___bls;  \
	___bls << C;			\
	___bls.RecordTime(T);	\
	___bls.LogToNet(I3, I1, I2);	\
}

#define LOGEVENT(I3, C)		\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		___ls << C;			\
		___ls.Log();		\
	}						\
}

#define LOGLIST(I3, C, V)	\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		___ls << C;			\
		for (auto __it = V.begin(); __it != V.end(); ++__it)	\
		{					\
			if (__it != V.begin())	\
			{				\
				___ls << ", ";	\
			}				\
			___ls << *__it;	\
		}					\
		___ls.Log();		\
	}						\
}

#define LOGVECTOR(I3, C, V)	LOGLIST(I3, C, V)

#define LOGMAP(I3, C, V)	\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		___ls << C;			\
		for (auto __it = V.begin(); __it != V.end(); ++__it)	\
		{					\
			___ls << _ln(__it->first) << __it->second;	\
		}					\
		___ls.Log();		\
	}						\
}

#define LOGEVENTFL(I3, C)	LOGEVENT(I3, _FUNCTION_AND_LINE_ << C)

#define LOGLISTFL(I3, C, V)	\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		___ls << _FUNCTION_AND_LINE_ << C;	\
		for (auto __it = V.begin(); __it != V.end(); ++__it)	\
		{					\
			if (__it != V.begin())	\
			{				\
				___ls << ", ";	\
			}				\
			___ls << *__it;	\
		}					\
		___ls.Log();		\
	}						\
}

#define LOGVECTORFL(I3, C, V)	LOGLISTFL(I3, C, V)

#define LOGMAPFL(I3, C, V)	\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		___ls << _FUNCTION_AND_LINE_ << C;	\
		for (auto __it = V.begin(); __it != V.end(); ++__it)	\
		{					\
			___ls << _ln(__it->first) << __it->second;	\
		}					\
		___ls.Log();		\
	}						\
}

#define LOGEVENT_(C) LOGEVENT("", C)

#define LOGEVENTSTR(I3, C)	\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		std::ostringstream os;	\
		os << C;			\
		___ls << os.str();	\
		___ls.Log();		\
	}						\
}

#define LOGEVENTL(I3, C)	\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		___ls << C;			\
		___ls.Log(true);	\
	}						\
}

#define LOGLISTL(I3, C, V)	\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		___ls << C;			\
		for (auto __it = V.begin(); __it != V.end(); ++__it)	\
		{					\
			if (__it != V.begin())	\
			{				\
				___ls << ", ";	\
			}				\
			___ls << *__it;	\
		}					\
		___ls.Log(true);	\
	}						\
}

#define LOGVECTORL(I3, C, V)	LOGLISTL(I3, C, V)

#define LOGMAPL(I3, C, V)	\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		___ls << C;			\
		for (auto __it = V.begin(); __it != V.end(); ++__it)	\
		{					\
			if (__it != V.begin())	\
			{				\
				___ls << ", ";	\
			}				\
			___ls << *__it;	\
		}					\
		___ls.Log(true);		\
	}						\
}

#define LOGEVENTLFL(I3, C)	LOGEVENTL(I3, _FUNCTION_AND_LINE_ << C)

#define LOGLISTLFL(I3, C, V)	\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		___ls << _FUNCTION_AND_LINE_ << C;	\
		for (auto __it = V.begin(); __it != V.end(); ++__it)	\
		{					\
			if (__it != V.begin())	\
			{				\
				___ls << ", ";	\
			}				\
			___ls << *__it;	\
		}					\
		___ls.Log(true);		\
	}						\
}

#define LOGVECTORLFL(I3, C, V)	LOGLISTLFL(I3, C, V)

#define LOGMAPLFL(I3, C, V)		\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		___ls << _FUNCTION_AND_LINE_ << C;	\
		for (auto __it = V.begin(); __it != V.end(); ++__it)	\
		{					\
			___ls << _ln(__it->first) << __it->second;	\
		}					\
		___ls.Log(true);		\
	}						\
}

#define LOGEVENTL_(C) LOGEVENTL("", C)

#define LOGEVENTLSTR(I3, C)	\
{							\
	LogStream ___ls(I3);	\
	if (___ls.IsEnable())	\
	{						\
		std::ostringstream os;	\
		os << C;			\
		___ls << os.str();	\
		___ls.Log(true);	\
	}						\
}

#define LOGBYTES(T, D, L) LOGEVENT(T, log_::b(D, L))
#define LOGBYTES_DISPLAY(T, D, L) LOGEVENT(T, log_::bd(D, L))

#define __LOGCMD(LOGFUNC, CMDBUF)	\
{								\
	uint32_t ___uSize = *(uint32_t*)(CMDBUF);	\
	if (___uSize >= 12)							\
	{											\
		char ___LogTypeBuf[13];					\
		sprintf(___LogTypeBuf, "_CMD%04x%04x", *(uint16_t*)((char*)CMDBUF + 8), *(uint16_t*)((char*)CMDBUF + 10));	\
		LOGFUNC(___LogTypeBuf, CMDBUF, ___uSize);	\
	}											\
	else										\
	{											\
		LOGFUNC("_ICMD", CMDBUF, ___uSize);	\
	}											\
}

#define LOGCMD(CMDBUF) __LOGCMD(LOGBYTES, CMDBUF)
#define LOGCMD_DISPLAY(CMDBUF) __LOGCMD(LOGBYTES_DISPLAY, CMDBUF)

//logfilename_prefix别超过200字节，否则里面可能溢出
LOG_API void LogInitializeLocalOptions(bool enable_to_console = false, bool enable_to_file = false, const char* logfilename_prefix = "", bool split_by_date = true, 
	int log_total_mega_bytes_limit_within_dir = 32 * 1024, int log_total_file_limit_within_dir = 200, const char* config_file_path = "logger.ini"); //如果配置文件中有值，则以配置文件中的值覆盖参数的值

#ifndef _LOG_DLL_

void SetLSServerTypeNameForQuery(const char* LS_ServerTypeNameForQuery); //一般假设ServerTypeName是"ls"，但也可能不是。本方法需要在CtrlLogin之前被调用。
void SetLSServerTypeName(const char* LS_ServerTypeName); //一般假设ServerTypeName是"ls"，但也可能不是。本方法会覆盖LS_ServerTypeNameForQuery。本方法需要在CtrlLogin之前被调用。

void _InitializeLogNetDelegate(); //由CtrlSvrConnector在CtrlLogin之前被调用

#else

//只有DLL版本有这个方法。DLL版本不使用NetLib(因为一旦用的话就DLL互相引用了，太绕，而且有个退不出的问题解决不了)，使用朴素UDP
//主要供客户端使用。所有和网络相关的一律往这个地址发。
//如果服务器有多台，则由客户端选择一台来Set给Log模块
LOG_API void SetLogServerAddress(const char* IP, int port); 

#endif

//本方法调用前(确切的说，是index1非空前)，所有通过LOGEVENT发往服务器的包都忽略，也不做统计，只是打屏幕。直到这两个参数确定。该方法可以被反复调用
//且，如果filename_prefix为空，那么本方法调用前，Log也不会打到文件，因为不知道Log的目的地文件名

LOG_API void LogSetIndex12(const char* index1, const char* index2); //index1 + index2别超过199字节，否则里面可能溢出

void LogSetIdentifier(const char* ServerType = "", int ServerID = -1);  //本方法本质上就是LogSetIndex12，把ServerType作为index1，把ServerID作为index2。ServerID为-1时index2为空字符串

//理论上这个方法要考虑引用计数，在真正的最后一份引用被卸载的时候才执行。暂时没有管，第一次调用就真正执行了。
LOG_API void LogUnInitialize();

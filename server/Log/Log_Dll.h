#pragma once

#ifdef _LOG_DLL_

#pragma warning(disable:4251 4275)

#ifdef LOG_EXPORTS
#define LOG_API __declspec(dllexport)
#else
#define LOG_API __declspec(dllimport)
#endif

#else

#define LOG_API

#endif
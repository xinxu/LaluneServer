#pragma once

#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include "../include/ptime2.h"
#include <string>
#include <stdio.h>
#include <boost/filesystem.hpp>
//#include <iostream>

class LogFile
{
protected:
	FILE* _file;
	std::string current_date; //作为输出到本地文件的文件名的一部分。新的一天到来时根据这个变量判断出变化，从而切换到新文件
	boost::mutex mutex;

	void GenerateFileName(char* file_name) //需要锁内调用
	{
		std::string time;
		if (SplitByDate) //该模式下以日期为文件名，每过一天自动开新文件
		{
			time = current_date;
		}
		else //该模式以时间为文件名，每次Log初始化用一个文件名，中途不做分割。
		{
			time = time_utility::ptime_to_string4(boost::posix_time::microsec_clock::local_time());
		}

		sprintf(file_name, "%s_%s.log", FileNamePrefix.c_str(), time.c_str());
	}

	inline void _CloseIfNeeded() //锁内调用
	{
		if (_file)
		{	
			fclose(_file);
			_file = nullptr;
		}
	}

public:
	LogFile() : _file(nullptr)
	{
	}

	inline void CloseIfNeeded()
	{
		boost::lock_guard<boost::mutex> lock(mutex);

		_CloseIfNeeded();
	}

	virtual ~LogFile()
	{
		CloseIfNeeded();
	}

	void PrintLine(const std::string& formatted_log)
	{		
		boost::lock_guard<boost::mutex> lock(mutex);		
		
		if (SplitByDate)
		{
			if (current_date != formatted_log.substr(1, 10))
			{
				current_date = formatted_log.substr(1, 10);
				_CloseIfNeeded();
			}
		}
		
		if (! _file) //如果文件还没开，则开一下文件。这个开文件过程可能会建新文件。
		{
			//开文件之前，检查同目录下的Log总大小是否超过限制。限制<=0则表示无限制。超限则按照修改时间依次删除老Log，直到不超限。
			
			if (LogTotalMegaBytesLimitWithinDir > 0 || LogTotalFileLimitWithinDir > 0) //任意一个条件满足，就要进去扫文件
			{
				//DEBUG
				//std::cout << LogTotalMegaBytesLimitWithinDir << ", " << LogTotalFileLimitWithinDir << std::endl;

				boost::filesystem::path dir;
				//实际上win下的根目录要冒号后面有杠才算合法。但我们这里只要把合法的判出来就行(把绝对路径找出来)，不用把非法的给判掉，boost会判的。
				if ((FileNamePrefix.size() >= 2 && FileNamePrefix[1] == ':') || (FileNamePrefix.size() >= 1 && FileNamePrefix[0] == '/'))
				{
					dir = FileNamePrefix;
				}
				else
				{
					//相对路径稳妥起见都加个点
					dir = ".";
					dir /= FileNamePrefix;
				}

				std::string actual_FileNamePrefix = dir.leaf().string();

				if (! is_directory(dir))
				{
					dir.remove_filename();
				}

				std::vector<std::pair<boost::filesystem::path, uintmax_t> > matched_files;
				uintmax_t total_size = 0;

				if (is_directory(dir))
				{
					boost::filesystem::directory_iterator it(dir), eod;
					for (; it != eod; ++it)
					{
						if (is_regular_file(*it) && it->path().extension() == ".log" && it->path().leaf().string().compare(0, actual_FileNamePrefix.size(), actual_FileNamePrefix) == 0)
						{
							boost::system::error_code error;
							uintmax_t size = file_size(it->path(), error);
							if (!error)
							{
								//DEBUG
								//std::cout << "matched file: " << it->path().string() << " " << size << std::endl;
								matched_files.push_back(std::make_pair(*it, size));
								total_size += size;
							}
						}
					}

					int file_count = matched_files.size();

					uintmax_t total_bytes_limit = LogTotalMegaBytesLimitWithinDir;
					total_bytes_limit *= 1024 * 1024;

					//std::cout << "total: " << total_bytes_limit << std::endl;

					//两条件之一满足，就要进去删文件
					if (total_bytes_limit > 0 && total_size > total_bytes_limit || LogTotalFileLimitWithinDir > 0 && file_count > LogTotalFileLimitWithinDir)
					{
						//std::cout << "delete files." << std::endl;
						sort(matched_files.begin(), matched_files.end()); //假设直接这么比就是按创建时间排序的。有点不严谨，正好有别的文件也符合这个FileNamePrefix的话顺序就会不完全正确。不考虑这种情况。
						auto will_delete_it = matched_files.begin();
						for ( ; will_delete_it != matched_files.end(); ++will_delete_it)
						{
							total_size -= will_delete_it->second;
							file_count --;

							//两个条件均满足，则可以break
							if ((total_bytes_limit <= 0 || total_size <= total_bytes_limit) && (file_count <= 0) || (file_count <= LogTotalFileLimitWithinDir))
							{
								break;
							}
						}
						//把will_delete_it前进一位，到时候删到will_delete_it的前一个文件为止
						if (will_delete_it != matched_files.end()) //如果相等，说明把这些文件都删光也满足不了限制条件。
						{
							will_delete_it ++;
						}
												
						boost::system::error_code error;
						for (auto it0 = matched_files.begin(); it0 != will_delete_it; ++it0)
						{
							remove(it0->first, error);
						}
					}
					else
					{
						//std::cout << "no need to delete files." << std::endl;
					}
				}
			}

			char file_name[300];
			GenerateFileName(file_name);
			_file = fopen(file_name, "a");

			if (! _file) //如果还是没开成，可能是因为权限不足或者硬盘已满，或者目录不存在
			{
				printf("ERROR: can't write to file %s\n", file_name);
				return;
			}
		}
		
		fprintf(_file, "%s\n", formatted_log.c_str());
		fflush(_file);
	}
};

extern LogFile file;
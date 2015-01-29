#pragma once

#include <boost/thread.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include "Timer.h"

class ioservice_thread
{
protected:
	boost::asio::io_service boostioservice;
	std::shared_ptr<boost::asio::io_service::work> work;
	int running_thread_count;
	boost::mutex mutex;

	void running_thread()
	{
		boost::system::error_code error;
		boostioservice.run(error);

		boost::lock_guard<boost::mutex> lock(mutex);
		running_thread_count--;
	}

public:
	ioservice_thread() : running_thread_count(0)
	{
	}

	boost::asio::io_service & get_ioservice()
	{
		return boostioservice;
	}

	std::shared_ptr<Timer> create_timer(int interval_secs, std::function<void()> handler)
	{
		Timer* t = new Timer(boostioservice, interval_secs, handler);
		return std::shared_ptr<Timer>(t);
	}

	std::shared_ptr<OneOffTimer> create_one_off_timer(int timeout_secs, std::function<void()> handler)
	{
		OneOffTimer* t = new OneOffTimer(boostioservice, timeout_secs, handler);
		return std::shared_ptr<OneOffTimer>(t);
	}

	//返回false表示已启动，不能重复启动
	bool start(int thread = 1, bool stop_when_no_work = false) 
	{
		{
			boost::lock_guard<boost::mutex> lock(mutex);
			if (running_thread_count > 0) return false;
			running_thread_count += thread;
		}

		if (!stop_when_no_work)
		{
			work = std::make_shared<boost::asio::io_service::work>(boostioservice);
		}

		for (int i = 0; i < thread; ++i)
		{
			boost::thread th(boost::bind(&ioservice_thread::running_thread, this));
			th.detach();
		}

		return true;
	}

	//返回false表示已启动，不能重复启动
	bool run_in_this_thread(bool stop_when_no_work = false) 
	{
		{
			boost::lock_guard<boost::mutex> lock(mutex);
			if (running_thread_count > 0) return false;
			running_thread_count = 1;
		}

		if (!stop_when_no_work)
		{
			work = std::make_shared<boost::asio::io_service::work>(boostioservice);
		}

		boost::system::error_code error;
		boostioservice.run(error);

		{
			boost::lock_guard<boost::mutex> lock(mutex);
			running_thread_count = 0;
		}

		return true;
	}

	void stop_when_no_work()
	{
		work.reset();
	}

	void wait_for_stop()
	{
		for (;;)
		{
			bool still_some_threads_running;
			{
				boost::lock_guard<boost::mutex> lock(mutex); 
				still_some_threads_running = running_thread_count != 0;
			}
			if (still_some_threads_running)
			{
				boost::this_thread::sleep(boost::posix_time::milliseconds(15));
			}
			else
			{
				break;
			}
		}
	}
};
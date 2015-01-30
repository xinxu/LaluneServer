#ifndef __Boids_Timer_h_
#define __Boids_Timer_h_

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/asio/deadline_timer.hpp>

class Timer
{
	friend class ioservice_thread;

protected:
	boost::asio::deadline_timer _timer;
	int _interval_secs;
	std::function<void()> _handler;

	void pulse(const boost::system::error_code& error)
	{
		if (!error)
		{
			_timer.expires_from_now(boost::posix_time::seconds(_interval_secs));
			_timer.async_wait(boost::bind(&Timer::pulse, this, boost::asio::placeholders::error));

			_handler();
		}
	}

	Timer(boost::asio::io_service& ioservice, int interval_secs, std::function<void()> handler) : _timer(ioservice), _interval_secs(interval_secs), _handler(handler)
	{
		_timer.expires_from_now(boost::posix_time::seconds(interval_secs));
		_timer.async_wait(boost::bind(&Timer::pulse, this, boost::asio::placeholders::error));
	}
};

class OneOffTimer
{
	friend class ioservice_thread;

protected:
	boost::asio::deadline_timer _timer;
	int _timeout_secs;
	std::function<void()> _handler;

	void timeout(const boost::system::error_code& error)
	{
		if (!error)
		{
			_handler();
		}
	}

	OneOffTimer(boost::asio::io_service& ioservice, int timeout_secs, std::function<void()> handler) : _timer(ioservice), _timeout_secs(timeout_secs), _handler(handler)
	{
		refresh();
	}

public:
	void refresh()
	{
		_timer.expires_from_now(boost::posix_time::seconds(_timeout_secs));
		_timer.async_wait(boost::bind(&OneOffTimer::timeout, this, boost::asio::placeholders::error));

	}
};

#endif
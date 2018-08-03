#ifndef TIME_LORD_H
#define TIME_LORD_H

#include <chrono>

/*
This file contains helper classes to deal with 
non-blocking event tracking.

TimeStamp is simply a wrapper for a steady_clock::time_point.
This enables classes to track when they last accessed a field in a CommsLink
and make use of Comms::hasNew() with a minimum of fuss.
Simply instantiate a TimeStamp, then use TimeStamp::touch() to bring it up to
date when an event occurs. Use getTimePoint() to pass the internal time_point
to hasNew() to check if a new piece of data needs to be retrieved.

TimeOut is an implementation of a nonblocking delay. Use reset() to start it
or bring it up to current time and periodically check timedOut() to determine
exactly that.
*/

class TimeStamp {
	using clock = std::chrono::steady_clock;
public:
	TimeStamp() : timestamp(clock::now()) {}
	void touch() {
		timestamp = clock::now();
	}
	const clock::time_point& getTimePoint() { return timestamp; }

private:
	clock::time_point timestamp;
};

class TimeOut {
	using clock = std::chrono::steady_clock;
public:
	// construct given a duration, but do not start
	explicit TimeOut(int milliseconds, bool _enabled = false) {
		enabled = _enabled;
		dur = std::chrono::milliseconds(milliseconds);
	}
	// start from now
	void reset(void) {
		timestamp = clock::now();
		enabled = true;
	}
	// set duration and start from now
	void reset(int milliseconds) {
		dur = std::chrono::milliseconds(milliseconds);
		reset();
	}

	// if enabled, check if duration has been exceeded
	bool timedOut(void) { // true if timeout has been exceeded
		if(enabled && clock::now() > timestamp + dur) {
			enabled = false;
			return true;
		}
		return false;
	}
private:
	// needs to a time_point to track start time
	bool enabled;
	clock::time_point timestamp;
	std::chrono::milliseconds dur;
};

#endif
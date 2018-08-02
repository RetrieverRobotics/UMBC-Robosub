#ifndef DERIVED_TASKS_H
#define DERIVED_TASKS_H

#include "Task.hpp"
#include "TimeLord.hpp"

#include "Actions.hpp"

/*

Example declaration:

class XXX : public Task {
public:
	explicit XXX(ThreadManager&, Comms&);

	const Result update(void);
private:
};

*/

class WaitForStart : public Task {
public:
	explicit WaitForStart(ThreadManager&, Comms&);

	const Result update(void);
private:
	TimeOut timeout;
};

class Submerge : public Task {
public:
	explicit Submerge(ThreadManager&, Comms&);

	const Result update(void);
private:
	TimeStamp depth_ts;
	TimeOut timeout;

	float pressure_target;
	float pressure_tolerance;
};

class ValidationGate : public Task {
public:
	explicit ValidationGate(ThreadManager&, Comms&);

	const Result update(void);
private:
};

class SurfaceAndWait : public Task {
public:
	explicit SurfaceAndWait(ThreadManager&, Comms&);

	const Result update(void);
};

class EStopDaemon : public Task {
public:
	explicit EStopDaemon(ThreadManager&, Comms&);

	const Result update(void);
};

class CommsDaemon : public Task {
public:
	explicit CommsDaemon(ThreadManager&, Comms&);

	const Result update(void);

private:
	action::Interpreter a_interpreter;
	
	int keep_alive_ticker;
	TimeOut keep_alive;

	void doStuff(void);
};

// DEFERRED - These will not be used this year

class QualifierGateEntry : public Task {
public:
	explicit QualifierGateEntry(ThreadManager&, Comms&);

	const Result update(void);
private:
	action::SearchForQualGate a_search;
	action::MoveTowardsQualGate a_move;
};

class QualifierPin : public Task {
public:
	explicit QualifierPin(ThreadManager&, Comms&);

	const Result update(void);
};

class QualifierGateExit : public Task {
public:
	explicit QualifierGateExit(ThreadManager&, Comms&);

	const Result update(void);
};

#endif
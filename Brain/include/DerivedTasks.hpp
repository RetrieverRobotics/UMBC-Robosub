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
};

class Setup : public Task {
public:
	explicit Setup(ThreadManager&, Comms&);

	const Result update(void);
private:
	TimeOut delay;
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
	TimeOut delay;
	int operation;
	int thrust;
	int dur;
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
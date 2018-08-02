#ifndef TASK_H
#define TASK_H

#include <iostream>
#include <string>
#include <tuple>

#include "NamedClass.hpp"
#include "ThreadManager.hpp"
#include "Comms/Comms.hpp"

/*
NEVER USE BLOCKING FUNCTIONS IN TASKS

A Task has a State, which determines its RunType and returns a ReturnStatus.
A template for implementation can be found in DerivedClasses.[hpp/cpp]
When instantiated, the task has State::Ready. The TaskManager can start it,
which changes State to Running, and may set RunType to Init, unless the
corresponding option has been set to false in the Task::launch() function.
From that point on, any calls to update will happen with RunType as Normal.

Task::update must always return a Result, which is a combination of a ReturnStatus
and a string message. Under most circumstances, the ReturnStatus will be Continue.
When a task determines itself to be complete it can return ReturnStatus::Success
or ReturnStatus::Failure. On the next call to update, the RunType will be Stop, 
at which point the task is expected to clean up any memory it may have allocated
and close connections.

A task has the ability to load threads into a ThreadManager, either directly or
through the wrapper functions. It is HIGHLY recommended that you use the wrapper
functions. The Threadable can be defined either internally or elsewhere -
Actions.[hpp/cpp] is recommended for commonly used Threadables.
Task::unloadAll() should always be called in RunType::Stop.

ThreadManager wrapper functions:
load(...) ~ loads a Threadable into the manager with the given name and RunLevel.
RunLevel::Worker is fine for most things. For Threadables that are expected to run
forever (and have very few side-effects), use RunLevel::Critical.
status(...) ~ get the status of a Threadable, useful for determining whether it 
is safe to retrieve data from it, ie by making sure it is Paused (loaded and not
working).
unload(...) ~ Tell the manager to clean up the given Threadable.
unloadAll(...) ~ Tell the manager to clean up all threads launched by this task.
*/

class Task : public NamedClass {

public:

	enum class State {
		Running,
		Done,
		Ready
	};
	enum class RunType {
		Init,
		Normal,
		Stop
	};
	enum class ReturnStatus {
		Success = 0,
		Continue,
		Failure
	};
	static const std::string ReturnMsg[];

	class Result {
	public:
		Result(ReturnStatus _status, std::string _message) : status(_status), message(_message) {}
		const ReturnStatus& getStatus(void) { return status; }
		const std::string& getMessage(void) { return message; }
	private:
		ReturnStatus status;
		std::string message;
	};

	Task(std::string _instance_name, ThreadManager& _thread_manager, Comms& _comms);
	virtual ~Task() {}

	const State getState();

	void launch(bool skip_init = false);

	virtual const Result update(void) = 0;
	
	void kill(bool reset_state = true);

	// ThreadManager function wrappers
	void load(Threadable&, const std::string&, const th_man::RunLevel);
	const th_man::ThreadStatus status(Threadable&);
	void resume(Threadable&);
	void unload(Threadable&);
	void unloadAll(void);

protected:
	Comms& comms;
	State state;
	const RunType getRunType();
	
private:
	ThreadManager& thread_manager;
	RunType run_type;
};

#endif
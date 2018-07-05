#ifndef TASK_H
#define TASK_H

#include <iostream>
#include <string>
#include <tuple>

#include "NamedClass.hpp"
#include "ActionManager.hpp"

class Task : public NamedClass {

public:

	enum class State {
		Running,
		Done,
		Stopped
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

	Task(std::string _instance_name,
		std::tuple<ReturnStatus, std::string> (* _task_function)(Task& task),
		ActionManager& _action_manager);


	const State getState();
	const RunType getRunType();

	void launch(bool skip_init = false);
	void skipInit(void);
	std::tuple<ReturnStatus, std::string> update(void);
	void kill(bool reset_state = true);

private:
	ActionManager& action_manager;
	std::tuple<ReturnStatus, std::string> (* task_function)(Task& task);
	State state;
	RunType run_type;

	// also need a way to store persistent results data from actions between task runs
};

#endif
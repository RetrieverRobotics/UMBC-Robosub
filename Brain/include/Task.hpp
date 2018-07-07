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

	Task(std::string _instance_name, ActionManager& _action_manager);

	const State getState();
	const RunType getRunType();

	void launch(bool skip_init = false);

	typedef std::tuple<ReturnStatus, std::string> task_return_t;
	virtual task_return_t update(void) = 0;
	
	void kill(bool reset_state = true);

protected:
	ActionManager& action_manager;
	State state;
	RunType run_type;
};

#endif
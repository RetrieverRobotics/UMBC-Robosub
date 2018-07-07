#include "Task.hpp"

Task::Task(std::string _instance_name, ActionManager& _action_manager)
	: action_manager(_action_manager), NamedClass("Task", _instance_name)
{
	state = State::Stopped;
	run_type = RunType::Init;
}

const std::string Task::ReturnMsg[]= { "Success", "Continue", "Failure" };

void Task::launch(bool skip_init) {
	if(skip_init) run_type = Task::RunType::Normal; else run_type = Task::RunType::Init;
	state = Task::State::Running;
	update();
}
void Task::kill(bool reset_state) {
	run_type = Task::RunType::Stop;
	update();
	if(reset_state) state = Task::State::Stopped; else state = Task::State::Done;
}

const Task::State Task::getState() {
	return state;
}
const Task::RunType Task::getRunType() {
	Task::RunType tmp = run_type;
	if(run_type == Task::RunType::Init) {
		run_type = Task::RunType::Normal;
	}
	return tmp;
}
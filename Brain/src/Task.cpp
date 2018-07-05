#include "Task.hpp"

Task::Task(std::string _instance_name,
	std::tuple<ReturnStatus, std::string> (* _task_function)(Task& task),
	ActionManager& _action_manager) :
		task_function(_task_function),
		action_manager(_action_manager),
		NamedClass("Task", _instance_name)
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
void Task::skipInit(void) {
	if(run_type == Task::RunType::Init) run_type = Task::RunType::Normal;
}
std::tuple<Task::ReturnStatus, std::string> Task::update(void) {
	return task_function(*this);
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
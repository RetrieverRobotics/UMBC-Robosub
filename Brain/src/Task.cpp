#include "Task.hpp"

Task::Task(std::string _instance_name, ThreadManager& _thread_manager, Comms& _comms)
	: thread_manager(_thread_manager), comms(_comms), NamedClass("Task", _instance_name)
{
	state = State::Ready;
	run_type = RunType::Init;
}

const std::string Task::ReturnMsg[]= { "Success", "Continue", "Failure" };

void Task::launch(bool skip_init) {
	if(skip_init) run_type = RunType::Normal; else run_type = RunType::Init;
	state = State::Running;
	update();
}
void Task::kill(bool reset_state) {
	run_type = RunType::Stop;
	update();
	if(reset_state) state = State::Ready; else state = State::Done;
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


void Task::load(Threadable& th, const std::string& display_name, const th_man::RunLevel run_level) {
	thread_manager.load(*this, th, display_name, run_level);
}

const th_man::ThreadStatus Task::status(Threadable& th) {
	return thread_manager.status(th);
}

void Task::resume(Threadable& th) {
	thread_manager.resume(th);
}

void Task::unload(Threadable& th) {
	thread_manager.unload(th);
}

void Task::unloadAll(void) {
	thread_manager.unloadAllFromParent(*this);
}


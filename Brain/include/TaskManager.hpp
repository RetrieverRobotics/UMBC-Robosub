#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <string>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <memory>

#include "NamedClass.hpp"
#include "Task.hpp"

/*
In Brain, there are two levels of execution - as a Task in a state machine managed by
this class and as a Threadable (See ThreadManager).

A Task is a logical group of nonblocking behavior. This may correspond directly to
a competition task or may be more general like a text interpreter or file monitor.

The principle of TaskManager as it stands is to limit both the burden on and control
given to the end user for the general management of behaviors. To that end, a 
familiar ternary syntax has been implemented. To use TaskManager, first subclass
'Task' then register a shared pointer to it with TaskManager::registerTask() and
give it references to Comms and ThreadManager instances.
At this point a Task is ready to be used.
Example:
manager_instance.registerTask(std::make_shared<ExampleTask>(_comms, _thread_manager));

To start a task when a TaskManager is started, add the Task's name (which ideally
would match the name of the derived class that implements it) to a comma-separated
string in TaskManager::onStart(). Note: The comma is in a single string, not between
multiple strings - the function takes only one argument.
Example:
manager_instance.onStart("ExampleStartTask1, ExampleStartTask2");

To configure behavior for when Tasks end, use TaskManager::configureTree().
The syntax is below:
[ EndingTask ? SuccessTask1(, SuccessTask2) : FailureTask1(, FailureTask2) ]
[ EndingTask ? SuccessTask1(, SuccessTask2) ]
[ EndingTask : FailureTask1(, FailureTask2) ]
[ EndingTask ] // this will not cause an error, however it also won't result in
	an entry
Note that in the second and third line, an entire branch has been skipped. If no
branch is provided, nothing will happen. Note also that if specifying multiple
Tasks in a branch, a comma is required.
Again, configureTree() takes a single argument, however instead of commas, the
brackets themselves form the separators. Utilizing the fact that C++ concatenates
strings placed next to each other, this allows a syntax as below.
Example:
manager_instance.configureTree(
	"[ WakeUp ? BrushTeeth, CombHair : GoBackToSleep ]"
	"[ BrushTeeth : GoBackToSleep ]"
	"[ CombHair ? EatBreakfast ]"
);

Then call TaskManager::start() to add the aforementioned tasks specified in
TaskManager::onStart() to the run queue.

As TaskManager runs a state machine, it must now be stepped with
TaskManager::update(). update() has an option called reset_after_branch which 
is true by default. Setting this to false makes it impossible to successfully
start a task more than once.

To determine if there's any tasks in the run queue, call TaskManager::tasksRunning().
To get a nicely formatted string of tasks and their states, call
TaskManager::listTasks(). This can then be written to std::out, logged, or used
to render a graphical view.
Example output:
Tasks
------
 - | Task1#Task // ready to run, but not
 * | Task2#Task // running
 ^ | Task3#Task // done, can't be started until set to ready


*/

class TaskManager : public NamedClass {
public:
	TaskManager();

	bool registerTask(std::shared_ptr<Task>);

	void onStart(const std::string&);
	void configureTree(const std::string&);
	void start();

	void update(bool reset_after_branch = true);
	void branch(std::shared_ptr<Task>, const Task::ReturnStatus&);
	void killAll(bool reset_after_kill = true);

	bool tasksRunning();
	std::string listTasks(std::string); // takes argument for which tasks to include: all, running, stopped
	std::string listTasks();

private:
	std::vector<std::string> tags_start;
	std::unordered_map<std::string, std::shared_ptr<Task>> tasks;

	// tuple to hold two vectors of strings, the first with the success tags, and the second with the fail tags
	typedef std::tuple< std::vector<std::string>, std::vector<std::string> > branch_t;
	std::unordered_map<std::string, branch_t> branches;

	std::vector<std::string> splitAndVerify(const std::string&, std::vector<std::string>&);
	bool isRegistered(std::string);
};

#endif
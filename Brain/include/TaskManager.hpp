#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <regex>

#include "NamedClass.hpp"
#include "Task.hpp"
#include "string_util.hpp"

class TaskManager : public NamedClass {
public:
	TaskManager();

	void registerTask(Task&);
	// void registerTasks(std::tuple<std::string, std::tuple< Task::ReturnStatus, std::string > (*)(void) >; // as variadic function
	void onStart(const std::string&);
	void onBlock(const std::string&);
	void configureTree(const std::string&);
	void start();
	void block();

	void update(bool reset_after_branch = true);
	void branch(Task&, Task::ReturnStatus&);

	bool tasksRunning();
	std::string listTasks(std::string); // takes argument for which tasks to include: all, running, stopped
	std::string listTasks();

private:
	std::vector<std::string> tags_start;
	std::vector<std::string> tags_block;
	std::unordered_map<std::string, Task&> tasks;

	// tuple to hold two vectors of strings, the first with the success tags, and the second with the fail tags
	typedef std::tuple< std::vector<std::string>, std::vector<std::string> > branch_t;
	std::unordered_map<std::string, branch_t> branches;

	std::vector<std::string> splitAndVerify(const std::string&, std::vector<std::string>&);
	bool isRegistered(std::string);
};

#endif
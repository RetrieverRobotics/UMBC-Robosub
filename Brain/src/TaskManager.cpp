#include "TaskManager.hpp"

#include <iostream>
#include <regex>

#include "plog/Log.h"

#include "string_util.hpp"

TaskManager::TaskManager() : NamedClass("TaskManager") {
}

bool TaskManager::registerTask(std::shared_ptr<Task> task) {
	if(!isRegistered(task->getInstanceName())) {
		tasks.insert( { task->getInstanceName(), task } );
		return true;
	}
	LOG_WARNING << task->getInstanceName() << " is already registered - Ignoring." << std::endl;
	return false;
}

void TaskManager::onStart(const std::string& task_list) {
	std::string clean = string_util::removeWhitespace(task_list);

	std::vector<std::string> rejects = splitAndVerify(clean, tags_start);
	for(auto& tag : rejects) LOG_WARNING << "Task '" << tag << "' not registered - Ignoring." << std::endl;
}

void TaskManager::configureTree(const std::string& config) {
	branches.clear(); // wipe previous contents of the branches to allow reconfig

	std::string clean_config = string_util::removeWhitespace(config);

	std::regex e_inside_brackets("(?:\\[)(.*?)(?:\\])"); // content inside brackets available in capture group 1
	// [ root ? success_task1, success_task2 : failure_task1, failure_task2] - ? and :, and the trailing tasks are optional, however
	// the root will be skipped if there is no exit behavior
	// this next expression assumes that all whitespace has been removed, which simplifies parsing into leaf vectors
	std::regex e_branch_parts("([a-zA-Z_,]+)(?:\\?)?([a-zA-Z_,]+)?(?::)?([a-zA-Z_,]+)?"); // always return 3 groups, however 2nd and/or third may be empty

	std::smatch match_inside_brackets;
	std::string::const_iterator cc_start(clean_config.cbegin()); // initialize a local const_iterator from a const_iterator pointing to the beginning of config
	while(regex_search(cc_start, clean_config.cend(), match_inside_brackets, e_inside_brackets)) {
		cc_start += match_inside_brackets.length();

		std::string branch_text = match_inside_brackets[1];
		std::smatch match_parts;
		std::string::const_iterator bt_start(branch_text.cbegin());
		if(regex_search(bt_start, branch_text.cend(), match_parts, e_branch_parts)) {
			LOG_INFO << branch_text;

			std::string root_tag = match_parts[1];
			if(!isRegistered(root_tag)) {
				LOG_WARNING << "\tRoot task '" << root_tag << "' not registered [ from '" << branch_text << "' ] - Skipping branch.";
				continue;
			}

			try {
				branches.at(root_tag);
				LOG_WARNING << "\tA branch already exists for '" << root_tag << "' [ from root / '" << branch_text << "' ] - Skipping branch.";
				continue;
			} catch( const std::out_of_range& e) {}

			std::vector<std::string> success_leaves;
			std::vector<std::string> failure_leaves;

			if(!std::string(match_parts[2]).empty()) {
				std::vector<std::string> rejects = splitAndVerify(match_parts[2], success_leaves);
				for(auto& tag : rejects) LOG_WARNING << "\tTask '" << tag << "' not registered [ from success / '" << branch_text << "' ] : Ignoring.";
			}

			if(!std::string(match_parts[3]).empty()) {
				std::vector<std::string> rejects = splitAndVerify(match_parts[3], failure_leaves);
				for(auto& tag : rejects) LOG_WARNING << "\tTask '" << tag << "' not registered [ from failure / '" << branch_text << "' ] : Ignoring.";
			}

			if(success_leaves.size() == 0 && failure_leaves.size() == 0) {
				LOG_WARNING << "\tNo valid responses [ from '" << branch_text << "' ] : Skipping branch.";
				continue;
			}

			LOG_DEBUG << "S leaves: " << success_leaves.size() << ", F leaves: " << failure_leaves.size();
			branches.insert( {root_tag, branch_t(success_leaves, failure_leaves)} );
			LOG_INFO << "\t*** Branch added ***";
		} else {
			LOG_WARNING << "Bad branch syntax [ from '" << branch_text << "' ] : Skipping branch.";
		}

	}
}

void TaskManager::start() {
	killAll();
	for(std::string& tag : tags_start) {
		LOG_DEBUG << "Launching " << tag;
		tasks.at(tag)->launch();
	}
}

void TaskManager::killAll(bool reset_after_kill) {
	for(auto& task_entry : tasks) {
		if(task_entry.second->getState() == Task::State::Running) {
			task_entry.second->kill(reset_after_kill);
			LOG_DEBUG << "Killing " << task_entry.first;
		}
	}
}

void TaskManager::update(bool reset_after_branch) {
	for(auto& task_entry : tasks) {
		std::shared_ptr<Task> task = task_entry.second;
		if(task->getState() == Task::State::Running) {
			Task::Result result = task->update();
			if(result.getStatus() != Task::ReturnStatus::Continue) {
				task->kill(reset_after_branch);

				LOG_INFO << task->getFullName() << " { " << Task::ReturnMsg[(int)result.getStatus()] << ": " << result.getMessage() << " }";

				branch(task, result.getStatus());
			}
		}
	}
}

void TaskManager::branch(std::shared_ptr<Task> task, const Task::ReturnStatus& status) {
	try { // not all tasks have branches
		branch_t& b = branches.at(task->getInstanceName());
		if(status == Task::ReturnStatus::Success) {
			std::vector<std::string> list = std::get<0>(b);
			if(list.size() > 0) {
				for(auto& t : list) {
					tasks.at(t)->launch();
				}
			}
		} else if(status == Task::ReturnStatus::Failure) {
			std::vector<std::string> list = std::get<1>(b);
			if(list.size() > 0) {
				for(auto& t : list) {
					tasks.at(t)->launch();
				}
			}
		}
		
	} catch( const std::out_of_range& e) {}
}

bool TaskManager::tasksRunning() {
	for(auto& task_entry : tasks) {
		if(task_entry.second->getState() == Task::State::Running) return true;
	}
	return false;
}

std::string TaskManager::listTasks() {
	std::string output = "Tasks\n--------\n";
	for(auto& task_entry : tasks) {
		output += " ";
		switch(task_entry.second->getState()) {
			case Task::State::Running:
				output += "*";
				break;
			case Task::State::Done:
				output += "x";
				break;
			case Task::State::Ready:
				output += "-";
				break;
			default:
				output += "?";
				break;
		}
		output += " | " + task_entry.second->getInstanceName() + "\n";
	}
	output += "\n";
	return output;
}

std::string TaskManager::listTasks(std::string which) {
	std::string output = "";
	if(which == "all") {
		for(auto& task_entry : tasks) {
			output += task_entry.first + "\n";
		}
	} else if(which == "running") {
		for(auto& task_entry : tasks) {
			if(task_entry.second->getState() == Task::State::Running) output += task_entry.first + "\n";
		}
	} else if(which == "onStart") {
		for(auto& tag : tags_start) {
			output += tag + "\n";
		}
	}
	return output;
}

std::vector<std::string> TaskManager::splitAndVerify(const std::string& src, std::vector<std::string>& target) {
	std::vector<std::string> tmp_split;
	std::vector<std::string> rejects;
	string_util::splitOnChar(src, ',', tmp_split);
	for(auto& tag : tmp_split) {
		if(isRegistered(tag)) {
			target.push_back(tag);
		} else {
			rejects.push_back(tag);
		}
	}
	return rejects;
}
bool TaskManager::isRegistered(const std::string task_tag) {
	try {
		tasks.at(task_tag);
		return true;
	} catch(const std::out_of_range& e) {
		return false;
	}
}

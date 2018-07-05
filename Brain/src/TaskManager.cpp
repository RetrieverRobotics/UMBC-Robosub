#include "TaskManager.hpp"

TaskManager::TaskManager(ActionManager& _action_manager) : action_manager(_action_manager), NamedClass("TaskManager") {
}

void TaskManager::registerTask(std::string instance_name, std::tuple< Task::ReturnStatus, std::string > (* task_function)(Task& task)) {
	tasks.insert( { instance_name, *(new Task(instance_name, task_function, action_manager)) } );
}

void TaskManager::onStart(const std::string& task_list) {
	std::string clean = string_util::removeWhitespace(task_list);

	std::vector<std::string> rejects = splitAndVerify(clean, tags_start);
	for(auto& tag : rejects) std::cerr << "Task '" << tag << "' not registered [ in onStart() ] : Ignoring." << std::endl;
}
void TaskManager::onBlock(const std::string& task_list) {
	std::string clean = string_util::removeWhitespace(task_list);

	std::vector<std::string> rejects = splitAndVerify(clean, tags_block);
	for(auto& tag : rejects) std::cerr << "Task '" << tag << "' not registered [ in onBlock() ] : Ignoring." << std::endl;
}

void TaskManager::configureTree(const std::string& config) {
	std::string clean_config = string_util::removeWhitespace(config);

	std::regex e_inside_brackets("(?:\\[)(.*?)(?:\\])"); // content inside brackets available in capture group 1
	// [ root ? success_task1, success_task2 : failure_task1, failure_task2] - colon and failure tasks are optional
	// this next expression assumes that all whitespace has been removed, which simplifies parsing into leaf vectors
	std::regex e_branch_parts("([a-zA-Z_,]+)(?:\\?)([a-zA-Z_,]+)(?::)?([a-zA-Z_,]+)?"); // always return 3 groups, however 3rd may be empty

	std::smatch match_inside_brackets;
	std::string::const_iterator cc_start(clean_config.cbegin()); // initialize a local const_iterator from a const_iterator pointing to the beginning of config
	while(regex_search(cc_start, clean_config.cend(), match_inside_brackets, e_inside_brackets)) {
		cc_start += match_inside_brackets.length();

		std::string branch_text = match_inside_brackets[1];
		std::smatch match_parts;
		std::string::const_iterator bt_start(branch_text.cbegin());
		if(regex_search(bt_start, branch_text.cend(), match_parts, e_branch_parts)) {
			std::cout << branch_text << std::endl;

			std::string root_tag = match_parts[1];
			if(!isRegistered(root_tag)) {
				std::cerr << "\tRoot task '" << match_parts[1] << "' not registered [ from '" << branch_text << "' ] : Skipping Branch." << std::endl;
				continue;
			}

			try {
				branches.at(root_tag);
				std::cerr << "\tA branch already exists for '" << root_tag << "' [ from root / '" << branch_text << "' ] : Skipping branch." << std::endl;
				continue;
			} catch( const std::out_of_range& e) {}

			std::vector<std::string> success_leaves;
			std::vector<std::string> failure_leaves;

			std::vector<std::string> rejects = splitAndVerify(match_parts[2], success_leaves);
			for(auto& tag : rejects) std::cerr << "\tTask '" << tag << "' not registered [ from success / '" << branch_text << "' ] : Ignoring." << std::endl;

			if(!std::string(match_parts[3]).empty()) {
				std::vector<std::string> rejects = splitAndVerify(match_parts[3], failure_leaves);
				for(auto& tag : rejects) std::cerr << "\tTask '" << tag << "' not registered [ from failure / '" << branch_text << "' ] : Ignoring." << std::endl;
			}

			if(success_leaves.size() == 0 && failure_leaves.size() == 0) {
				std::cerr << "\tNo valid responses [ from '" << branch_text << "' ] : Skipping this branch." << std::endl;
				continue;
			}

			branches.insert( {root_tag, *(new branch_t(success_leaves, failure_leaves))} );
			std::cout << "\t*** Branch added ***" << std::endl;
		} else {
			std::cerr << "Bad branch syntax [ from '" << branch_text << "' ] : Skipping this branch." << std::endl;
		}

	}
}

void TaskManager::start() {
	for(auto& task_entry : tasks) {
		if(task_entry.second.getState() == Task::State::Running) task_entry.second.kill();
	}
	for(std::string& tag : tags_start) {
		tasks.at(tag).launch();
	}
}

void TaskManager::block() {
	// stop all tasks, start tasks in tags_block, then update running tasks in a while loop; DO NOT BRANCH
	std::cerr << "block() is not implemented." << std::endl;
}

void TaskManager::update(bool reset_after_branch) {
	for(auto& task_entry : tasks) {
		Task& task = task_entry.second;
		if(task.getState() == Task::State::Running) {
			auto ret = task.update();
			Task::ReturnStatus status = std::get<0>(ret);
			if(status != Task::ReturnStatus::Continue) {
				task.kill(reset_after_branch);

				std::cout << task.getFullName() << " { " << Task::ReturnMsg[(int)status] << ": " << std::get<1>(ret) << " }" << std::endl;

				branch(task, status);
			}
		}
	}
}

void TaskManager::branch(Task& task, Task::ReturnStatus& status) {
	try { // not all tasks have branches
		branch_t b = branches.at(task.getInstanceName());
		if(status == Task::ReturnStatus::Success) {
			std::vector<std::string> list = std::get<0>(b);
			for(auto& t : list) {
				tasks.at(t).launch();
			}
		} else if(status == Task::ReturnStatus::Failure) {
			std::vector<std::string> list = std::get<1>(b);
			for(auto& t : list) {
				tasks.at(t).launch();
			}
		}
		
	} catch( const std::out_of_range& e) {}
}

bool TaskManager::tasksRunning() {
	for(auto& task_entry : tasks) {
		if(task_entry.second.getState() == Task::State::Running) return true;
	}
	return false;
}

std::string TaskManager::listTasks() {
	std::string output = "Tasks\n--------\n";
	for(auto& task_entry : tasks) {
		output += " ";
		switch(task_entry.second.getState()) {
			case Task::State::Running:
				output += "*";
				break;
			case Task::State::Done:
				output += "^";
				break;
			case Task::State::Stopped:
				output += "-";
				break;
			default:
				output += "?";
				break;
		}
		output += " | " + task_entry.second.getFullName() + "\n";
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
			if(task_entry.second.getState() == Task::State::Running) output += task_entry.first + "\n";
		}
	} else if(which == "onStart") {
		for(auto& tag : tags_start) {
			output += tag + "\n";
		}
	} else if(which == "onBlock") {
		for(auto& tag : tags_block) {
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

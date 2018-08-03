#include "ThreadManager.hpp"

#include <iostream>

#include "plog/Log.h"

using namespace th_man;

void ThreadManager::load(NamedClass& parent, Threadable& functor, const std::string& display_name, const th_man::RunLevel run_level) {
	if(functor.tryInit()) { // if not loaded
		std::thread t(std::ref(functor));
		if(t.joinable()) t.detach();

		thread_map.emplace(&functor, std::make_unique<ThreadPack>(parent, display_name, run_level, t));
	}
}

th_man::ThreadStatus ThreadManager::status(Threadable& th) {
	try {
		std::unique_ptr<ThreadPack>& th_p = thread_map.at(&th);

		if(th.isWorking()) {
			return ThreadStatus::Working;
		}

	} catch(std::out_of_range& e) {
		return ThreadStatus::NotLoaded;
	}
	return ThreadStatus::Paused;
}

void ThreadManager::resume(Threadable& th) {
	th.nextStep();
}

void ThreadManager::unload(Threadable& th) {
	// safe to drop unique_ptr<ThreadPack> (containing thread object) according to https://en.cppreference.com/w/cpp/thread/thread/~thread
	if(th.queueCleanUp()) thread_map.erase(&th); // only release the handle if the thread actually intends to stop	
}

void ThreadManager::unloadAllFromParent(const NamedClass& parent) {
	for(auto it = thread_map.begin(); it != thread_map.end(); ++it) {
		std::unique_ptr<ThreadPack>& th_p = it->second;
		if(&(th_p->parent) == &parent) unload(*(it->first));
	}
}

std::string ThreadManager::listThreads(void) {
	std::string output = "Threads\n--------\n";
	for(auto it = thread_map.begin(); it != thread_map.end(); ++it) {
		ThreadStatus th_status = status(*(it->first));
		switch(th_status) {
			case ThreadStatus::Working:
				output += " > ";
				break;
			case ThreadStatus::Paused:
				output += " ||";
				break;
			default:
				output += " ? ";
				LOG_WARNING << "ThreadStatus '" << (int)th_status << "' not supported.";
				break;
		}
		output += " (";
		ThreadPack& info = *(it->second);
		switch(info.run_level) {
			case RunLevel::Critical:
				output += "C";
				break;
			case RunLevel::Worker:
				output += "W";
				break;
			default:
				output += "?";
				LOG_WARNING << "RunLevel '" << (int)info.run_level << "' not supported.";
				break;
		}
		if(it->first->isPersistent()) output += "*"; else output += " ";
		output += ") | " + info.name + " @ " + info.parent.getInstanceName() + "\n\n";
	}
	return output;
}

int ThreadManager::threadCount(void) {
	return thread_map.size();
}

int ThreadManager::threadCount(RunLevel level) {
	int cnt = 0;
	for(auto it = thread_map.begin(); it != thread_map.end(); ++it) {
		if(it->second->run_level == level) ++cnt;
	}
	return cnt;
}
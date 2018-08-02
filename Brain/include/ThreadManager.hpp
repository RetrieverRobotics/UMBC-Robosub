#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include <unordered_map>
#include <functional>
#include <thread>
#include <string>

#include "NamedClass.hpp"
#include "Threadable.hpp"

/*
ThreadManager keeps track of Threadables launched by Tasks.

Each Threadable has a RunLevel, which currently includes Worker and Critical.
Worker threads tend to have a specific short-lived function, while Critical
threads are expected to have a very long lifetime and will not be stopped often.

Each Threadable also has an implicit ThreadStatus inherent to its current
behavior.

A Threadable is designed to run as a state machine. This avoids data races as a
Task merely has to check if the Threadable is currently processing data, and
avoids constant recomputation even if the calling Task does not need the data
thereby making best use of available compute time. For specifics on internals,
see Threadable.

To start a thread, a Threadable is passed to load() which adds an entry in a map
under the given name and initializes the Threadable.
A Task may at any time call resume (even while the thread is working), thereby
queueing the next step.
*/
namespace th_man {
	enum class RunLevel {
		Critical, Worker
	};
	enum class ThreadStatus {
		NotLoaded, Working, Paused
	};
}
class ThreadManager {
public:
	void load(NamedClass& parent, Threadable& functor, const std::string& display_name, const th_man::RunLevel);

	th_man::ThreadStatus status(Threadable& th);

	void resume(Threadable& th);

	void unload(Threadable& th);
	void unloadAllFromParent(const NamedClass& parent);

	std::string listThreads(void);
	int threadCount(void);
	int threadCount(th_man::RunLevel);

private:
	struct ThreadPack {
		ThreadPack(const NamedClass& _parent, const std::string& _name, th_man::RunLevel level, std::thread& _th)
			: parent(_parent), name(_name), run_level(level), th(std::move(_th)) {}

		std::thread th;
		const NamedClass& parent;
		const th_man::RunLevel run_level;
		const std::string name;
	};

	std::unordered_map<Threadable*, std::unique_ptr<ThreadPack> > thread_map;
};

#endif
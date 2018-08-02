#ifndef THREADABLE_H
#define THREADABLE_H

#include <string>
#include <atomic>

#include "NamedClass.hpp"

/*
As mentioned in ThreadManager, a Threadable is a state machine. It is also
a Functor, in that it overloads operator(). When launched in a thread, 
operator() is called once. In order to maintain the thread between steps,
a while loop is included in operator(). The state variables are atomic as it
is feasible that they might be modified from several different threads.

On the first step, the init function is called. From then on, step() is called.
When the thread is unloaded, cleanUp() is called. Note that a Threadable may not
run cleanUp() immediately when ThreadManager::unload is called as a step can not
be interrupted - a step must complete before cleanUp can be called. This is why,
the function is queueCleanUp() - to indicate that cleanUp will happens when the
thread gets around to checking if it should.
*/

class Threadable {
public:
	Threadable();
	virtual ~Threadable() {}

	void operator()(void);

	bool tryInit(void) {
		if(!init_complete.load()) {
			init();
			init_complete.store(true);
			return true;
		}
		return false;
	}

	void queueCleanUp(void) {
		clean_up_next.store(true);
	}

	void nextStep(void) {
		allow_step.store(true);
	}

	bool isWorking(void) {
		return working.load();
	}

protected:
	virtual void init(void) {}
	virtual void step(void) {}
	virtual void cleanUp(void) {}

private:
	std::atomic<bool> init_complete;
	std::atomic<bool> allow_step;
	std::atomic<bool> working;
	std::atomic<bool> clean_up_next;

	void sleepThread(int ms);
};

#endif
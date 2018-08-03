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

NOTE: It is possible to run blocking functions in a Threadable without interrupting
main thread execution however inadvisable as this will prevent Task::unload
from having predictable behavior. Exceptions are things like reading from cin
which have to be blocking - however it then becomes risky to try and cleanUp
that thread. To that end I am experimenting with a 'persistent' property. queueCleanUp()
will have no effect on a thread that has 'persistent' set.
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

	bool queueCleanUp(void) {
		if(!persistent) {
			clean_up_next.store(true);
			return true;
		}
		return false;
	}

	void nextStep(void) {
		allow_step.store(true);
	}

	bool isWorking(void) {
		return working.load();
	}
	bool isPersistent(void) {
		return persistent;
	}

protected:
	virtual void init(void) {}
	virtual void step(void) {}
	virtual void cleanUp(void) {}

	bool persistent;
	void makePersistent(void) {
		persistent = true;
	}

	void sleepThread(int ms);

private:
	std::atomic<bool> init_complete;
	std::atomic<bool> allow_step;
	std::atomic<bool> working;
	std::atomic<bool> clean_up_next;
};

#endif
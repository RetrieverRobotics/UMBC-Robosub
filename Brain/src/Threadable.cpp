#include "Threadable.hpp"

#include <thread>
#include <chrono>

Threadable::Threadable() : allow_step(true), working(false), clean_up_next(false), init_complete(false), persistent(false) {}

void Threadable::operator()(void) {
	if(init_complete) {
		while(true) {
			if(clean_up_next.load()) {
				cleanUp();

				clean_up_next.store(false);
				working.store(false);
				allow_step.store(true);
				init_complete.store(false);

				break;

			} else {
				if(allow_step) {
					allow_step.store(false);

					working.store(true);
					step();
					working.store(false);
				} else {
					sleepThread(0);
				}
			}
		}
	}
}

void Threadable::sleepThread(int ms) {
	if(ms > 0) std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
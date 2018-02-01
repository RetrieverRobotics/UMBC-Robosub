/*
Robosub Controller
First attempt at a manager for overall sub behavior

NOT INTENDED TO COMPILE

for the qualifier gate:
	needs to be able to descend and hold a depth
	hold a heading (initial heading, then adjusted)
	identify features of an image and modify heading
		two orange rectangles, vertical
		the fact that they are near a horizontal black rectangle
...
*/

class SubController {
public:
	SubController();
	~SubController();

	void register(uint8_t (*f)(InputState* in, TargetState* out));

	void step(InputState* in, TargetState* out) {
		if(current_task != NULL) {
			uint8_t task_done = current_task(in, out);
			// if non-zero...
			if(task_done) {
				// set current_task equal to the next function pointer
				// in tasks or to NULL if there isn't another
			}
		}	
	}

	uint8_t isDone() {
		if(current_task == NULL) {
			return true;
		}
		return false;
	}

private:
	// a dynamic list to store all the function pointers
	// to be run in the order they were added
	Vector<...> tasks;

	// a pointer to the current task function
	... current_task;

};

struct InputState {
	// frame pointers to next frame in whatever video streams we have
	// full orientation vector
	// depth data
	// point cloud if possible from active sonar
	// pinger location or null from DSP processor
	// manipulator state
};
struct TargetState {
	// target orientation vector
	// target movement vector
	// target depth
	// manipulator state
	// torpedo launchers
};

// layout of a task function
uint8_t SomeTask(InputState* in, TargetState* out) {
	// do whatever SomeTask does
	// return 0 if it wants to be called again
	// or non-zero if it's done, value depends on next task to be called
}
uint8_t StopTask(InputState* in, TargetState* out) {
	// another task
	// stop all the motors, make things safe, etc
}

int main(void) {
	SubController controller = new SubController();

	controller.register(SomeTask); // takes a function pointer
	controller.register(StopTask);

	// separated for readability reasons
	DataModel current_state = new InputState();
	DataModel target_state = new TargetState();

	while(true) {
		// fetch the next video frames, ping locator heading, depth,
		// orientation, current state of manipulators, etc
		current_state.___ = ....

		// pass this model of the current sub state to the controller
		// which will then pass this to the current task
		// controller returns a new target state (heading, depth, manipulators)
		controller.step(&current_state, &target_state);
		
		// send motor and manipulator commands to uC based on the target_state	
		// send telemetry to ground station if this is enabled

		// log the current and target state

		if(controller.isDone()) {
			break;
		}
	}


	return 0;
}

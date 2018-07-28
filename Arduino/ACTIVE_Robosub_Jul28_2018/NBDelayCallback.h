#ifndef NB_DELAY_CALLBACK_H
#define NB_DELAY_CALLBACK

#include <Arduino.h>

class NBDelayCallback { // only implements milliseconds for now
public:
	NBDelayCallback(int _length, void (*_callback)(void), bool _enabled = false)
		: start(0), length(_length), callback(_callback), enabled(_enabled) {
		touch();
	}

	NBDelayCallback() : NBDelayCallback(500, NULL) {}

	void touch(bool force_reset = false) {
		if(isComplete() || force_reset) {
			start = millis();
		}
	}

	bool isComplete() {
		return ( millis() > start + length ); // if (now) is later than (start + length)
	}

	void update() {
		if(enabled && isComplete()) {
			if(callback) callback();
			touch();
		}
	}

	void setEnabled(bool _enabled = true) {
		enabled = _enabled;
	}
private:
	unsigned long start;
	unsigned int length;
	void (*callback)(void);

	bool enabled;
};

#endif
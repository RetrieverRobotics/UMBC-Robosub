/**********************************************************************************************
 * Arduino PID Library - Version 1.1.1
 * by Brett Beauregard <br3ttb@gmail.com> brettbeauregard.com
 * Edited by Connor Strang
 *
 * This Library is licensed under a GPLv3 License
 **********************************************************************************************/

#if ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include "PID_v1_strang.h"

extern String boot_log;

PID::PID(double out_min, double out_max, int period, Direction dir,
	PropOn on_what,
	double Kp, double Ki, double Kd) : setpoint(0), input(0), output(0), period_ms(period)
{

	mode = Mode::Manual;

	setOutputLimits(out_min, out_max);

	setTunings(Kp, Ki, Kd, on_what);
	setDirection(dir);

	prev_ms = millis() - period_ms;
}
PID::PID() : PID(-100, 100, 50) {}

bool PID::compute(double set, double in) {
	setSetpoint(set);
	setInput(in);
	return compute();
}

bool PID::compute() {
	if(mode != Mode::Automatic) return false;

	unsigned long now = millis();
	unsigned long delta_time = (now - prev_ms);

	if(delta_time >= period_ms) {
		double out_tmp = 0;

		// Compute all the working error variables
		double error = setpoint - input;
		double d_input = input - prev_input;
		output_sum += Ki * error;

		// apply the proportional term based on the prop_on option
		if(prop_on == PropOn::POnError) {
			out_tmp = Kp * error;
		} else if(prop_on == PropOn::POnMeasure) {
			output_sum -= Kp * d_input;
		}

		output_sum = constrain(output_sum, out_min, out_max);

		// Finish computing output
		out_tmp += output_sum - Kd * d_input;

		output = constrain(out_tmp, out_min, out_max);

		// store state
		prev_input = input;
		prev_ms = now;
		return true;
	}
	return false;
}

void PID::setTunings(double _Kp, double _Ki, double _Kd, PropOn on_what) {
	prop_on = on_what;
	setTunings(_Kp, _Ki, _Kd);

}

void PID::setTunings(double _Kp, double _Ki, double _Kd){
	if ( _Kp < 0 || _Ki < 0 || _Kd < 0 ) return;

	disp_Kp = _Kp; disp_Ki = _Ki; disp_Kd = _Kd;

	double period_sec = ((double)period_ms)/1000;
	Kp = _Kp;
	Ki = _Ki * period_sec;
	Kd = _Kd / period_sec;

	if(direction == Direction::Reverse) {
		Kp *= -1;
		Ki *= -1;
		Kd *= -1;
	}
}

void PID::setPeriod(int _period_ms) {
	if (_period_ms > 0) {
		double ratio  = (double)_period_ms / (double)period_ms;
		Ki *= ratio;
		Kd /= ratio;
		period_ms = (unsigned long)_period_ms;
	}
}

void PID::setOutputLimits(double _out_min, double _out_max) {
	if(_out_min >= _out_max) return;

	out_min = _out_min;
	out_max = _out_max;

	if(mode == Mode::Automatic){
		output = constrain(output, out_min, out_max);
		output_sum = constrain(output_sum, out_min, out_max);
	}
}

void PID::setMode(Mode new_mode, bool on_manual_zero_output) {
	if(new_mode != mode) {
		// if currently manual and going to automatic, initialize
		if(mode == Mode::Manual && new_mode == Mode::Automatic) {
			init();
		} else if(on_manual_zero_output) {
			// prevents a bug where output will jump upon reentering automatic
			output = 0;
		}

		mode = new_mode;
	}
}

void PID::init() {
	output_sum = output;
	prev_input = input;
	output_sum = constrain(output_sum, out_min, out_max);
}

void PID::setDirection(Direction dir) {
	// don't bother if they're the same
	if(dir != direction) {
		if(mode) {
			Kp *= -1;
			Ki *= -1;
			Kd *= -1;
		}
		direction = dir;
	}
}

double PID::getKp() { return disp_Kp; }
double PID::getKi() { return disp_Ki; }
double PID::getKd() { return disp_Kd; }

PID::Mode PID::getMode() { return mode; }
PID::Direction PID::getDirection() { return direction; }


void PID::setSetpoint(double val) {
	setpoint = val;
}
void PID::setInput(double val) {
	input = val;
}
bool PID::setOutput(double val) {
	if(mode == Mode::Manual) {
		output = val;
		return true;
	}
	return false;
}

double PID::getSetpoint() { return setpoint; }
double PID::getInput() { return input; }
double PID::getOutput() { return output; }


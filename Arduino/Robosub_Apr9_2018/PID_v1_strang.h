#ifndef PID_V1_STRANG_H
#define PID_V1_STRANG_H
#define LIBRARY_VERSION	1.0

class PID {
public:

	enum Mode {
		Manual, Automatic
	};
	// action mode of controller: Direct increases output when error is positive, Reverse decreases it
	enum Direction {
		Direct, Reverse
	};
	enum PropOn { // Kp * [measurement || error]
		POnMeasure, POnError
	};

	PID(double out_min, double out_max, int period_ms, Direction dir = Direct,
		PropOn on_what = PropOn::POnError,
		double Kp = 0, double Ki = 0, double Kd = 0);
	PID();

	void setMode(Mode new_mode, bool on_manual_zero_output = true);

	bool compute(double set, double in);
	bool compute();

	void setOutputLimits(double _out_min = 0, double _out_max = 255);

	void setTunings(double _Kp, double _Ki, double _Kd, PropOn on_what);         	  
	void setTunings(double _Kp, double _Ki, double _Kd);

	void setDirection(Direction dir);
	void setPeriod(int _period_ms = 100);

	double getKp();
	double getKi();
	double getKd();
	Mode getMode();
	Direction getDirection();

	void setSetpoint(double);
	void setInput(double);
	bool setOutput(double);

	double getSetpoint();
	double getInput();
	double getOutput();

private:
	void init();

	double setpoint;
	double input;
	double output;

	double output_sum, prev_input;
	double out_min, out_max;

	// user-entered tuning constants
	double disp_Kp;
	double disp_Ki;
	double disp_Kd;

	// tuning constants after having been corrected for sample time
	double Kp;
	double Ki;
	double Kd;

	Direction direction;
	PropOn prop_on;

	unsigned long period_ms;
	unsigned long prev_ms;

	Mode mode;
};

#endif


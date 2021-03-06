//https://forum.pjrc.com/threads/40813-Teensy-3-5-3-6-Breakouts-Available-Now-Discount-for-PJRC-Forum-Members

// Further development of this program can be found in Robosub_Jul28_2018
// Text Banners were generated using pyfiglet with the fonts clr6x6 and clr5x6 (inside functions)

// Builds successfully with Arduino 1.8.0 and Teensyduino 1.34
// Target is a Teensy 3.5

// This version has not been tested with a BNO055, although it will compile. Minor adjustments may be necessary
// to the EulerVector3 inputs to get correct behavior.

#include <ArduinoSTL.h>
#include <map>
#include <Wire.h>

#include "string_utils.h"

// IMU
// uncomment a single selection - multiple IMUs are not supported
//#define IMU_BNO055
#define IMU_MPU6050

#ifdef IMU_BNO055 // adafruit breakout, vcc pin towards front, away from center
	#include <Adafruit_BNO055.h>
	Adafruit_BNO055 bno055 = Adafruit_BNO055(55);

#elif defined(IMU_MPU6050) // generic breakout, vcc pin towards front, pin header away from center
	#include <MPU6050_tockn.h>
	MPU6050 mpu6050(Wire);

#endif

// brushless thruster ESC wrapper
#include "BLThruster.h"

// maps are slower than direct access, however they lend themselves to being accessed from an interpreter
std::map<String, BLThruster> thrusters = {
	{ "vert_fl", BLThruster(3) },
	{ "vert_bl", BLThruster(4) },
	{ "vert_fr", BLThruster(6) },
	{ "vert_br", BLThruster(7) },
	{ "thrust_l", BLThruster(2) },
	{ "thrust_r", BLThruster(5) }
};

/*
Location Map (Top):
				[Front]
		 vert_fl    vert_fr
  thrust_l			    thrust_r
		 vert_bl    vert_br
				 [Back]
*/

// Depth Sensor
#include "MS5803.h"

MS5803 ms5803 = MS5803(0x77, 5);

// PID (in a wrapper class to handle the variables)
#include "PID_v1_strang.h"

// std::map doesn't maintain insertion order -> use a vector with the same names, iterate the vector and use that to access
// elements in map in correct order. This can also be used to disable printing of certain elements by simply not inserting it.
std::vector<String> pid_order = { "yaw", "pitch", "roll", "pressure" };
std::map<String, PID> pid = {
	{ "yaw", PID(-50, 50, 50, PID::Direction::Reverse) },
	{ "pitch", PID(-80, 80, 50, PID::Direction::Reverse) },
	{ "roll", PID(-30, 30, 50, PID::Direction::Reverse) },
	{ "pressure", PID(-80, 80, 50, PID::Direction::Reverse) }
};

std::map<String, float> pid_target = {
	{ "yaw", 0 },        // degrees
	{ "pitch", 0 },       // degrees
	{ "roll", 0 },        // degrees
	{ "pressure", 1010 } // millibar, ~1 atm
};

// lonely variable that sets the base forward/backwards velocity before yaw pid output is mixed
float thrust_base = 0;

// Pilot Mode behavior variables
std::map<String, float> pilot_vars = {
	{ "jump_thrust", 25 },
	{ "step_thrust", 20 },
	{ "jump_yaw", 30 },
	{ "step_yaw", 5 },
	{ "step_pressure", 10 },
	{ "lim_base", 80 }
};

std::map<String, float> test_vars {
	{ "power", 20 }
};

// global struct to hold sensor data in one place
#include "EulerVector3.h"

struct sensor_data_struct {
	float water_pressure, water_temp;
	EulerVector3 e_orientation; // (e)uler
} sensor_data;

// logging stuff
#include "NBDelayCallback.h"

// forward declare functions passed as function pointers
void logSensors(void);
void logImuCal(void);
void logThrusters(void);
void logPID(void);

std::map<String, NBDelayCallback> log_triggers = {
	{ "sensors", NBDelayCallback(200, logSensors) },
	{ "imu_cal", NBDelayCallback(200, logImuCal) },
	{ "thr", NBDelayCallback(200, logThrusters) },
	{ "pid", NBDelayCallback(200, logPID) },
};

#include "setup_functions.h" // include this after all sensor definitions


//		 ####  #####  #####  #   #  ####
//		#      #        #    #   #  #   #
//		 ###   ####     #    #   #  ####
//		    #  #        #    #   #  #
//		####   #####    #     ###   #

void setup() {
	Serial.begin(115200); // Teensy always run 12Mbit / sec
	while(!Serial.dtr()) { delay(10); }; // wait for active serial connection to begin boot process

	pinMode(13, OUTPUT);
	digitalWrite(13, HIGH); // turn on onboard led to indicate that system is up

	setupPeripherals(); // function defined in setup_functions.h that handles sensor setup

	Serial.println("BOOT Finished.");

	// easy way to initialize everything to a safe state and prevent premature activation
	EStop();
}

// throwaway string that's always in scope to build messages to be printed with Serial.println()
String msg;

namespace mode {
	enum Interpreter {
		Config, Pilot, Pilot_OnChar, Test
	};
	enum Operation {
		Enabled, Disabled
	};
}

mode::Operation op_mode = mode::Disabled;
mode::Interpreter i_mode = mode::Config;


//		#       ###    ###   ####
//		#      #   #  #   #  #   #
//		#      #   #  #   #  ####
//		#      #   #  #   #  #
//		#####   ###    ###   #

void loop() {
	// input
	readSensors();
	readSerial();

	// data analysis and computation
	updatePIDControllers();

	// output
	updateThrusters();

	// logging
	bgLog();
}

void readSensors() {
	ms5803.update(MS5803_OSR_256); // 2-20 mS of delay, depending on sampling rate
	sensor_data.water_pressure = ms5803.getPressure();
	sensor_data.water_temp = ms5803.getTemperature();

#ifdef IMU_BNO055
	imu::Vector<3> euler = bno055.getVector(Adafruit_BNO055::VECTOR_EULER);
	sensor_data.e_orientation.set(euler.x(), euler.y(), euler.z());

#elif defined(IMU_MPU6050)
	mpu6050.update();
	// yaw and roll appear to be inverted compared to bno055
	sensor_data.e_orientation.set(-1*mpu6050.getAngleZ(), mpu6050.getAngleX(), -1*mpu6050.getAngleY());
#endif
}

#define MAX_CHARS_PER_LOOP (20)
#define CONFIG_ARG_SEP (' ')
String cmd_str;
void readSerial() {
	uint16_t char_count = 0;

	while(Serial.available() > 0 && char_count < MAX_CHARS_PER_LOOP) {
		char c = Serial.read();
		++char_count;

		if(i_mode == mode::Pilot_OnChar) {
			parseCommand(c);

		} else {
			if(c == '\n') {
				parseCommand(cmd_str);
				cmd_str = "";
			} else {
				cmd_str += c;
			}
		}	
	}
}

double angleWrapShortcut(double angle) {
	// it may be possible to improve this by first finding the piece that fits within a 360 degree disk
	// and then running the shortcut
	if(angle > 180) angle -= 360; else if(angle < -180) angle += 360;
	return angle;
}
void updatePIDControllers() {
	// In general, the PID controller setpoints are held at zero and an error between target and measurement
	// fed to the input. This keeps the numbers fed into the controllers predictable and low, regardless
	// of the ranges of the sensor measurements, which makes the tuning constants easier to predict.

	// For IMUs that report 0-360, a calculation can be done to minimize the angle the robot has to travel through
#if defined(BNO055) // || defined(XXX) // or any other imu that reports in 0-360 degrees
	pid["yaw"].setInput(angleWrapShortcut(pid_target["yaw"] - sensor_data.e_orientation.yaw()));
	pid["pitch"].setInput(angleWrapShortcut(pid_target["pitch"] - sensor_data.e_orientation.pitch()));
	pid["roll"].setInput(angleWrapShortcut(pid_target["roll"] - sensor_data.e_orientation.roll()));

#else
	pid["yaw"].setInput(pid_target["yaw"] - sensor_data.e_orientation.yaw());
	pid["pitch"].setInput(pid_target["pitch"] - sensor_data.e_orientation.pitch());
	pid["roll"].setInput(pid_target["roll"] - sensor_data.e_orientation.roll());
#endif

	pid["pressure"].setInput(pid_target["pressure"] - sensor_data.water_pressure);

	// run compute() on all pid controllers
	for(auto it = pid.begin(); it != pid.end(); ++it) {
		it->second.compute();
	}
}

void updateThrusters() {
	// see instantiations at top for location map

	// yaw correction mixed on top of a base forwards / backwards velocity
	// There is currently no way to measure velocity -> use a constant power level restricted to a low enough 
	// value that there is still room for the motors to react to yaw changes
	thrusters["thrust_l"].setPower(thrust_base + pid["yaw"].getOutput());
	thrusters["thrust_r"].setPower(thrust_base - pid["yaw"].getOutput());

	// mix outputs of pressure, pitch, and roll into vertical thrusters
	// this simple approach works reasonably well as long as the sub is intended to be flat (ie ~0 targets for pitch and roll)
	thrusters["vert_fl"].setPower(pid["pressure"].getOutput() - pid["pitch"].getOutput() + pid["roll"].getOutput());
	thrusters["vert_bl"].setPower(pid["pressure"].getOutput() + pid["pitch"].getOutput() + pid["roll"].getOutput());
	thrusters["vert_fr"].setPower(pid["pressure"].getOutput() - pid["pitch"].getOutput() - pid["roll"].getOutput());
	thrusters["vert_br"].setPower(pid["pressure"].getOutput() + pid["pitch"].getOutput() - pid["roll"].getOutput());
}

void startPilot() {
	// lock depth and yaw (keep existing settings for pitch and roll which have sane defaults)
	parseCommand(String("pid") + CONFIG_ARG_SEP + "pressure" + CONFIG_ARG_SEP + "lock");
	parseCommand(String("pid") + CONFIG_ARG_SEP + "yaw" + CONFIG_ARG_SEP + "lock");

	// enable all PID controllers
	for(auto it = pid.begin(); it != pid.end(); ++it) {
		it->second.setMode(PID::Mode::Automatic);
	}
}

void makeSafe() {
	// thrusters
	thrust_base = 0;

	for(auto thr_it = thrusters.begin(); thr_it != thrusters.end(); ++thr_it) {
		thr_it->second.setPower(0);
	}

	// pid controllers
	for(auto pid_it = pid.begin(); pid_it != pid.end(); ++pid_it) {
		pid_it->second.setMode(PID::Mode::Manual);
	}
}

void EStop() {
	makeSafe();
	op_mode = mode::Disabled;

	Serial.println("CMD Emergency stop activated. Send 'SAFE' to reset.");
}


//		 ###    ###   #   #  #   #    #    #   #  ####          ####    ###    ###    ####
//		#   #  #   #  ## ##  ## ##   # #   ##  #  #   #         #   #  #   #  #   #  #
//		#      #   #  # # #  # # #  #   #  # # #  #   #         #   #  #   #  #       ###
//		#   #  #   #  # # #  # # #  #####  #  ##  #   #         #   #  #   #  #   #      #
//		 ###    ###   #   #  #   #  #   #  #   #  ####          ####    ###    ###   ####


/*
Command Documention:
	*  ?  represents a string tag
	*  _  represents a number
	*  /  indicates that any of the surrounding options are acceptable

op_mode == Disabled
	SAFE ~ changes op_mode to mode::Enabled and switches i_mode to Config

op_mode == Enabled
	ESTOP ~ runs EStop() which changes op_mode to mode::Disabled and runs makeSafe()

	i_mode == Config
		* Note, the separator character (':' shown here) is flexible and is set by CONFIG_ARG_SEP
		up near the readSerial() method under loop() It is currently set to ' ' (space) as this
		is much easier to type quickly than a colon.

		pid:?:lock   ~ store relevant sensor measurement as target
		pid:?:_      ~ set target
		pid:?:+/-_   ~ change target by
		pid:?:read   ~ print current target

		pid:?:tune:_,_,_  ~ set tuning of a single controller
		pid:?:tune:read   ~ get tuning of a single controller
		pid:tune:read     ~ print tunings of all controllers

		pid:?:start/stop ~ starts or stops a single controller
		pid:start/stop   ~ starts or stops all controllers
		
		pilot:?:_     ~ set value of a variable in pilot_vars
		pilot:?:read  ~ print single variable
		pilot:read    ~ print all variables

		test:?:_      ~ identical configuration to pilot
		test:?:read
		test:read

		log:?:start/stop  ~ start or stop the logging of a set of information
		log:stop          ~ stop all logging

		mode:pilot/pilot_  ~ enter pilot mode; 'pilot' is \n-based, 'pilot_' is char-based
		mode:test          ~ enter thruster testing mode

		stop/exit/quit/q/x  ~ runs makeSafe()

	i_mode == Pilot || Pilot_OnChar [op_mode == Enabled]
		* values prefixed by 'step' are intended to be less than values prefixed by 'jump'
		* by default, a new line must be sent between commands which is supported by any terminal
		however the OnChar variation will process each character as it arrives and ignore the newline
		* in the keymap, letters may be shown next to each other when they have similar functions

		keymap:
		qQ  u      i
		       jJ  k  Ll
		    m      ,

		q ~ return to i_mode = Config, sub will continue whatever it was doing (ie holding depth, moving etc)
		Q ~ return to i_mode = Config, sub will be made safe

		u ~ ascend by step_pressure
		m ~ descend by step_pressure

		j ~ turn left by jump_yaw
		J ~ turn left by step_yaw
		l ~ turn right by jump_yaw
		L ~ turn right by step_yaw

		i [first press]        ~ forwards, set thrust_base to jump_thrust
		i [subsequent presses] ~ add step_thrust to thrust_base, clipping to lim_base
		, [first press]        ~ backwards, set thrust_base to -1*jump_thrust
		, [subsequent presses] ~ subtract step_thrust from thrust_base, clipping to lim_base

		k ~ stop, set thrust_base to 0

	i_mode == Test
		_some_string_  ~ searches for _some_string_ in name of all registered thrusters and sets matches to test_vars["power"]
		stop/x  ~ turns off all thrusters
		quit/exit  ~ call makeSafe() and return to i_mode = Config
*/


//		####     #    ####    ####  #####          ###    ###   #   #  #   #    #    #   #  ####
//		#   #   # #   #   #  #      #             #   #  #   #  ## ##  ## ##   # #   ##  #  #   #
//		####   #   #  ####    ###   ####          #      #   #  # # #  # # #  #   #  # # #  #   #
//		#      #####  # #        #  #             #   #  #   #  # # #  # # #  #####  #  ##  #   #
//		#      #   #  #  ##  ####   #####          ###    ###   #   #  #   #  #   #  #   #  ####
//		                                   #####


#define ERR_BAD_SYNTAX ("ERROR Bad syntax")

std::vector<String> args;
void parseCommand(String cmd) {
	String msg;

	switch(op_mode) {
		case mode::Disabled:
			if(cmd.equals("SAFE")) {
				op_mode = mode::Enabled;
				i_mode = mode::Config;
				Serial.println("INFO System Enabled. Interpreter mode -> Config");
			}
			break;

		case mode::Enabled:
			if(cmd.equals("ESTOP")) EStop();


// 	#  #   ##   ###   ####         ###   ##   #  #  ####  ###    ###
// 	####  #  #  #  #  #           #     #  #  ## #  #      #    #
// 	####  #  #  #  #  ###         #     #  #  # ##  ###    #    # ##
// 	#  #  #  #  #  #  #           #     #  #  #  #  #      #    #  #
// 	#  #   ##   ###   ####         ###   ##   #  #  #     ###    ###

			switch(i_mode) {
				case mode::Config:
				{
					str_util::split(cmd, CONFIG_ARG_SEP, args);
					uint8_t num_args = args.size();

					if(num_args > 0) {
						if(args[0].equals("pid")) { // pid:
							if(num_args == 2) {
								if(args[1].equals("stop")) { // pid:stop
									for(auto it = pid.begin(); it != pid.end(); ++it) {
										it->second.setMode(PID::Mode::Manual);
									}
									Serial.println("CMD config.pid.stop -> All PID controllers stopped.");

								} else if(args[1].equals("start")) { // pid:start
									for(auto it = pid.begin(); it != pid.end(); ++it) {
										it->second.setMode(PID::Mode::Automatic);
									}
									Serial.println("CMD config.pid.start -> All PID controllers started.");
								}

							} else if(num_args == 3) {
								if(pid.count(args[1]) > 0) { // ensure pid tag is valid
									if(args[2].equals("start")) { // pid:?:start
										pid[args[1]].setMode(PID::Mode::Automatic);
										msg = String("CMD config.pid.") + args[1] + ".start -> " + args[1] + " controller started."; Serial.println(msg);

									} else if(args[2].equals("stop")) { // pid:?:stop
										pid[args[1]].setMode(PID::Mode::Manual);
										msg = String("CMD config.pid.") + args[1] + ".stop -> " + args[1] + " controller stopped."; Serial.println(msg);
									
									} else if(args[2].equals("lock")) { // pid:?:lock
										// pretty much the only one that has to be filled manually
										bool print_success = true;
										float& target = pid_target[args[1]];
										if(args[1].equals("yaw")) {
											target = sensor_data.e_orientation.yaw();
										} else if(args[1].equals("pitch")) {
											target = sensor_data.e_orientation.pitch();
										} else if(args[1].equals("roll")) {
											target = sensor_data.e_orientation.roll();
										} else if(args[1].equals("pressure")) {
											target = sensor_data.water_pressure;
										} else {
											msg = String("ERROR config.pid.") + args[1] + ".lock -> " + args[1] + " not registered in pid_target[].";
											Serial.println(msg);
											print_success = false;
										}
										if(print_success) msg = String("CMD config.pid.") + args[1] + ".lock -> " + target; Serial.println(msg);
									
									} else if(args[2].equals("read")) { // pid:?:read
										msg = String("CMD config.pid.") + args[1] + ".read: " + pid_target[args[1]]; Serial.println(msg);

									} else { // pid:?:+/-_ or pid:?:_
										float val = args[2].toFloat();
										char sign = args[2].charAt(0);
										float& target = pid_target[args[1]];
										if(sign == '+' || sign == '-') target += val; else target = val;
										msg = String("CMD config.pid.") + args[1] + " -> " + target; Serial.println(msg);
									}
								} else if(args[1].equals("tune") && args[2].equals("read")) { // pid:tune:read
									Serial.println("CMD config.pid.tune.read:");
									for(auto it = pid_order.begin(); it != pid_order.end(); ++it) {
										PID& tmp = pid[*it];
										msg = String("   ") + *it + ": p = " + tmp.getKp() + ", i = " + tmp.getKi() + ", d = " + tmp.getKd();
										Serial.println(msg);
									}
								}
							} else if(num_args == 4 && args[2].equals("tune")) {
								if(pid.count(args[1]) > 0) {
									if(args[3].equals("read")) { // pid:?:tune:read
										PID& tmp = pid[args[1]];
										msg = String("CMD config.pid.") + args[1] + ".tune.read: p = " + tmp.getKp() + ", i = " + tmp.getKi() + ", d = " + tmp.getKd();
										Serial.println(msg);

									} else if(args[3].indexOf(',') > -1) { // probably pid:?:tune:_,_,_
										std::vector<String> tunings;
										str_util::split(args[3], ',', tunings);
										if(tunings.size() == 3) { // pid:?:tune:_,_,_
											PID& tmp = pid[args[1]];
											tmp.setTunings(tunings[0].toFloat(), tunings[1].toFloat(), tunings[2].toFloat());
											msg = String("CMD config.pid.") + args[1] + ".tune -> p = " + tmp.getKp() + ", i = " + tmp.getKi() + ", d = " + tmp.getKd();
											Serial.println(msg);
										}
									}
								}
							}

						} else if(args[0].equals("pilot")) {
							if(num_args == 2) {
								if(args[1].equals("read")) { // pilot:read
									msg = "CMD config.pilot.read ->\n";
									for(auto it = pilot_vars.begin(); it != pilot_vars.end(); ++it) {
										msg += String("   ") + it->first + ": " + it->second + "\n";
									}
									Serial.println(msg);
								}
							} else if(num_args == 3 && pilot_vars.count(args[1]) > 0) {
								if(args[2].equals("read")) { // pilot:?:read
									msg = String("CMD config.pilot.") + args[1] + ".read: " + pilot_vars[args[1]]; Serial.println(msg);

								} else { // pilot:?:_
									pilot_vars[args[1]] = constrain(args[2].toFloat(), 0, 100);
									msg = String("CMD config.pilot.") + args[1] + " -> " + pilot_vars[args[1]]; Serial.println(msg);
								}
							}

						} else if(args[0].equals("test")) {
							if(num_args == 2) {
								if(args[1].equals("read")) { // test:read
									msg = "CMD config.test.read ->\n";
									for(auto it = test_vars.begin(); it != test_vars.end(); ++it) {
										msg += String("   ") + it->first + ": " + it->second + "\n";
									}
									Serial.println(msg);
								}
							} else if(num_args == 3 && test_vars.count(args[1]) > 0) {
								if(args[2].equals("read")) { // test:?:read
									msg = String("CMD config.test.") + args[1] + ".read: " + test_vars[args[1]]; Serial.println(msg);

								} else { // test:?:_
									test_vars[args[1]] = constrain(args[2].toFloat(), 0, 100);
									msg = String("CMD config.test.") + args[1] + " -> " + test_vars[args[1]]; Serial.println(msg);
								}
							}

						} else if(args[0].equals("log")) { // log:
							if(num_args == 2) {
								if(args[1].equals("stop")) { // log:stop
									for(auto it = log_triggers.begin(); it != log_triggers.end(); ++it) {
										it->second.setEnabled(false);
									}
									Serial.println("CMD config.log.stop -> All loggers disabled.");
								}
							} else if(num_args == 3 && log_triggers.count(args[1]) > 0) {
								int val = ( args[2].equals("start") ? 1 : ( args[2].equals("stop") ? 0 : -1 ) ); // ternary for the win!
								if(val > -1) {
									log_triggers[args[1]].setEnabled( (bool)val ); // log:?:start/stop
									msg = String("CMD config.log.") + args[1] + "." + args[2] + " -> Logging for " + args[1] + (val > 0 ? " enabled." : " disabled.");
									Serial.println(msg);
								}
							}

						} else if(args[0].equals("mode")) { // mode:
							if(num_args == 2) {
								if(args[1].equals("pilot")) { // mode:pilot
									i_mode = mode::Pilot;
									startPilot();
									Serial.println("CMD config.mode -> Pilot - Parse on \\n.");
								} else if(args[1].equals("pilot_")) { // mode:pilot_
									i_mode = mode::Pilot_OnChar;
									startPilot();
									Serial.println("CMD config.mode -> Pilot - Parse each char.");
								} else if(args[1].equals("test")) { // mode:test
									i_mode = mode::Test;
									makeSafe();
									Serial.println("CMD config.mode -> Test.");
								}
							}

						} else if(args[0].equals("stop") || args[0].equals("exit") || args[0].equals("quit") || args[0].equals("q") || args[0].equals("x")) {
							makeSafe();
							Serial.println("CMD stop -> Made safe.");
						}
					}
				}
					break;


//		#  #   ##   ###   ####        ###   ###   #      ##  #####
//		####  #  #  #  #  #           #  #   #    #     #  #   #
//		####  #  #  #  #  ###         ###    #    #     #  #   #
//		#  #  #  #  #  #  #           #      #    #     #  #   #
//		#  #   ##   ###   ####        #     ###   ####   ##    #

				case mode::Pilot_OnChar: // take advantage of fall-through to select either Pilot or Pilot_OnChar
					Serial.println(); // send a newline so the response message doesn't end up appended to the command char
				case mode::Pilot:
				{
					if(cmd.equals("i")) { // forwards
						if(thrust_base > 0) {
							if(abs(thrust_base) < pilot_vars["lim_base"]) thrust_base += pilot_vars["step_thrust"];
						} else {
							thrust_base = pilot_vars["jump_thrust"];
						}
						thrust_base = constrain(thrust_base, 0, pilot_vars["lim_base"]);
						Serial.print("CMD pilot.Forward @ "); Serial.println(thrust_base);

					} else if(cmd.equals(",")) { // backwards
						if(thrust_base < 0) {
							if(abs(thrust_base) < pilot_vars["lim_base"]) thrust_base -= pilot_vars["step_thrust"];
						} else {
							thrust_base = -1*pilot_vars["jump_thrust"];
						}
						thrust_base = constrain(thrust_base, -1*pilot_vars["lim_base"], 0);
						Serial.print("CMD pilot.Reverse @ "); Serial.println(thrust_base);

					} else if(cmd.equals("k")) { // halt forward/backward motion
						thrust_base = 0;
						Serial.println("CMD pilot.Halt");

					} else if(cmd.equals("j")) { // left big
						pid_target["yaw"] += pilot_vars["jump_yaw"];
						Serial.print("CMD pilot.Left -> "); Serial.println(pid_target["yaw"]);

					} else if(cmd.equals("J")) { // left small
						pid_target["yaw"] += pilot_vars["step_yaw"];
						Serial.print("CMD pilot.Left -> "); Serial.println(pid_target["yaw"]);

					} else if(cmd.equals("l")) { // right big
						pid_target["yaw"] -= pilot_vars["jump_yaw"];
						Serial.print("CMD pilot.Right -> "); Serial.println(pid_target["yaw"]);

					} else if(cmd.equals("L")) { // right small
						pid_target["yaw"] += pilot_vars["step_yaw"];
						Serial.print("CMD pilot.Right -> "); Serial.println(pid_target["yaw"]);

					} else if(cmd.equals("u")){ // ascend
						pid_target["pressure"] -= pilot_vars["step_pressure"];
						Serial.print("CMD pilot.Ascend -> "); Serial.println(pid_target["pressure"]);

					} else if(cmd.equals("m")){ // descend
						pid_target["pressure"] += pilot_vars["step_pressure"];
						Serial.print("CMD pilot.Descend -> "); Serial.println(pid_target["pressure"]);

					} else if(cmd.equals("q")) { // exit mode
						i_mode = mode::Config;
						Serial.println("CMD pilot.Quit -> Returning to Config mode. Retaining behavior.");

					} else if(cmd.equals("Q")) {
						i_mode = mode::Config;
						makeSafe();
						Serial.println("CMD pilot.QuitAndSafe -> Returning to Config mode. Making safe.");

					}
				}
					break;


// 	#  #   ##   ###   ####       #####  ####   ### #####
// 	####  #  #  #  #  #            #    #     #      #
// 	####  #  #  #  #  ###          #    ###    ##    #
// 	#  #  #  #  #  #  #            #    #        #   #
// 	#  #   ##   ###   ####         #    ####  ###    #

				case mode::Test:
				{
					if(cmd.equals("list")) {
						msg = String("CMD test.list: [ ");
						for(auto it = thrusters.begin(); it != thrusters.end(); ++it) {
							msg += it->first + " ";
						}
						msg += " ]"; Serial.println(msg);

					} else if(cmd.equals("x") || cmd.equals("stop")) {
						for(auto it = thrusters.begin(); it != thrusters.end(); ++it) {
							it->second.setPower(0);
						}
						Serial.println("CMD test.stop -> Set power of all thrusters to 0.");

					} else if(cmd.equals("quit") || cmd.equals("exit")) {
						i_mode = mode::Config;
						makeSafe();
						Serial.println("CMD test.Quit -> Config.");

					} else if(!cmd.equals("")) {
						String list = "";
						for(auto it = thrusters.begin(); it != thrusters.end(); ++it) {
							if(it->first.indexOf(cmd) > -1) {
								it->second.setPower(test_vars["power"]);
								list += it->first + " ";
							}
						}
						if(!list.equals("")) {
							msg = String("CMD test.set -> [ ") + list + " ] to " + test_vars["power"];
							Serial.println(msg);
						}
					}
				}
					break;
			}
			
			break;

	}

}


//		#       ###    ####   ####   ###   #   #   ####
//		#      #   #  #      #        #    ##  #  #
//		#      #   #  #  ##  #  ##    #    # # #  #  ##
//		#      #   #  #   #  #   #    #    #  ##  #   #
//		#####   ###    ####   ####   ###   #   #   ####


void bgLog() {
	for(auto it = log_triggers.begin(); it != log_triggers.end(); ++it) {
		it->second.update();
	}
}

void logSensors(void) {
	Serial.println("INFO log.sensors...");
	msg = "INFO Pressure (mbar): " + String(sensor_data.water_pressure); Serial.println(msg);
	msg = "INFO Temperature (C): " + String(sensor_data.water_temp); Serial.println(msg);
	msg = "INFO Orientation (degrees): Yaw = " + String(sensor_data.e_orientation.roll())
		+ " Pitch = " + String(sensor_data.e_orientation.pitch())
		+ " Roll = " + String(sensor_data.e_orientation.roll());
	Serial.println(msg); Serial.println();
}

void logPID(void) {
	Serial.println("INFO log.pid...");
	for(auto it = pid_order.begin(); it != pid_order.end(); ++it) {
		PID& tmp = pid[*it];
		float& tmp_val = pid_target[*it];
		msg = String("INFO ") + *it + ": target = " + tmp_val + " | in = " + tmp.getInput() + " out = " + tmp.getOutput();
		Serial.println(msg);
	}
	Serial.println();
}

void logImuCal(void) {
	Serial.println("INFO log.imu_cal...");
#ifdef IMU_BNO055
	uint8_t sys, gyro, accel, mag;
	bno055.getCalibration(&sys, &gyro, &accel, &mag); // this blocks the entire program if it fails to get the data
	msg = String("INFO Imu Cal: Sys = ") + sys + ", Gyro = " + gyro + ", Accel = "
		+ accel + ", Mag = " + mag;
	Serial.println(msg);

#elif defined(IMU_MPU6050)
	Serial.println("INFO Imu Cal: Not implemented for MPU6050.");

#endif
	Serial.println();
}

void logThrusters(void) {
	Serial.println("INFO log.thr...");
	msg = String("INFO Thrusters: L") + thrusters["thrust_l"].getPower() + " R" + thrusters["thrust_r"].getPower() + " | ";
	msg += String("FL") + thrusters["vert_fl"].getPower() + " BL" + thrusters["vert_bl"].getPower()
	+ " BR" + thrusters["vert_br"].getPower() + " FR" + thrusters["vert_fr"].getPower();
	Serial.println(msg); Serial.println();
}

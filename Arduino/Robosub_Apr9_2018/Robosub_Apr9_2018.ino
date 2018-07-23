//https://forum.pjrc.com/threads/40813-Teensy-3-5-3-6-Breakouts-Available-Now-Discount-for-PJRC-Forum-Members


#include <ArduinoSTL.h>

// IMU
#include <Adafruit_BNO055.h>

Adafruit_BNO055 bno = Adafruit_BNO055(55);

// motor controller wrapper
#include "BLThruster.h"

BLThruster vert_fl(3), vert_bl(4), vert_fr(6), vert_br(7), thrust_l(2), thrust_r(5);

// TFT
#include <font_Arial.h>
#include <font_ArialBold.h>
#include <ILI9341_t3.h>

#define TFT_DC (9)
#define TFT_CS (10)

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);

// Depth Sensor
#include "MS5803.h"

MS5803 depth_sensor = MS5803(0x77, 5);

#include "string_utils.h"
#include "setup_functions.h"

// PID
#include <PID_v1.h>

double yaw_set, yaw_in, yaw_out; // yaw_set and yaw_in in degrees, yaw_out in motor power
double yaw_Kp = 0, yaw_Ki = 0, yaw_Kd = 0;
PID yaw_controller(&yaw_in, &yaw_out, &yaw_set, yaw_Kp, yaw_Ki, yaw_Kd, REVERSE);
float target_heading = 0;

double depth_set, depth_in, depth_out; // depth_set and depth_in in millibars, depth_out in motor power
double depth_Kp = 0, depth_Ki = 0, depth_Kd = 0;
PID depth_controller(&depth_in, &depth_out, &depth_set, depth_Kp, depth_Ki, depth_Kd, REVERSE);
float target_pressure = 1000; // millibars, ~1 atm

double pitch_set, pitch_in, pitch_out;
double pitch_Kp = 0, pitch_Ki = 0, pitch_Kd = 0;
PID pitch_controller(&pitch_in, &pitch_out, &pitch_set, pitch_Kp, pitch_Ki, pitch_Kd, REVERSE);
float target_pitch = 0;
float pitch_multiplier = 1;

double roll_set, roll_in, roll_out;
double roll_Kp = 0, roll_Ki = 0, roll_Kd = 0;
PID roll_controller(&roll_in, &roll_out, &roll_set, roll_Kp, roll_Ki, roll_Kd, REVERSE);
float target_roll = 0;
float roll_multiplier = 1;

const float PILOT_STEP_YAW = 45;
const float PILOT_STEP_PRESSURE = 10;
const float PILOT_STEP_THRUST = 30;
const float PILOT_BASE_LIM = 80;

float thrust_base = 0;
float pilot_jump_thrust = 45;

//
struct sensor_data_struct {
  float water_pressure, water_temp;
  imu::Vector<3> e_orientation; // (e)uler
} sensor_data;

// logging stuff
bool log_sensors = false;
uint32_t log_sensors_prev_millis = 0;
uint16_t log_sensors_delay = 200;

bool log_imu_cal = false;
uint32_t log_imu_cal_prev_millis = 0;
uint16_t log_imu_cal_delay = 200;
uint8_t sys, gyro, accel, mag;

bool log_thr = false;
uint32_t log_thr_prev_millis = 0;
uint16_t log_thr_delay = 200;

bool log_nav = false;
uint32_t log_nav_prev_millis = 0;
uint16_t log_nav_delay = 200;

void setup() {

  Serial.begin(115200); // Teensy always run 12Mbit / sec
  while(!Serial.dtr()) { delay(10); };
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);

  setupPeripherals(); // IMU, TFT, MS5803 (pressure)

  yaw_controller.SetMode(MANUAL);
  yaw_controller.SetOutputLimits(-50,50);
  yaw_controller.SetSampleTime(50);
  depth_controller.SetMode(MANUAL);
  depth_controller.SetOutputLimits(-100,100);
  depth_controller.SetSampleTime(50);
  pitch_controller.SetMode(MANUAL);
  pitch_controller.SetOutputLimits(-30,30);
  pitch_controller.SetSampleTime(50);
  roll_controller.SetMode(MANUAL);
  roll_controller.SetOutputLimits(-30,30);
  roll_controller.SetSampleTime(50);

  yaw_set = 0;
  depth_set = 0;
  pitch_set = 0;
  roll_set = 0;

  Serial.println("BOOT Finished.");
}

String cmd, msg;
float tmp_val = 0;

#define MODE_CTRL_AUTO (0)
#define MODE_CTRL_PILOT (1)
uint8_t ctrl_mode = MODE_CTRL_AUTO;

#define MAX_CHARS_PER_LOOP (20)

void loop() {
  // read sensors
  depth_sensor.update(MS5803_OSR_256); // 2-20 mS of delay, depending on sampling rate
  sensor_data.water_pressure = depth_sensor.getPressure();
  sensor_data.water_temp = depth_sensor.getTemperature();

  sensor_data.e_orientation = bno.getVector(Adafruit_BNO055::VECTOR_EULER);

  log();

  // get serial input
  uint16_t char_count = 0;
  while(Serial.available() > 0 && char_count < MAX_CHARS_PER_LOOP) {
    char c = Serial.read();
    ++char_count;
    if(c == '\n') {
      parseCommand(cmd);
      cmd = "";
    } else {
      cmd += c;
    }
  }

//  if(ctrl_mode == MODE_CTRL_AUTO) {

    // update motion controllers with new inputs
    yaw_controller.SetTunings(yaw_Kp, yaw_Ki, yaw_Kd);
    yaw_in = target_heading - sensor_data.e_orientation.x(); // compute an error, yaw_set will always be 0
    if(yaw_in < -180) yaw_in += 360; else if(yaw_in > 180) yaw_in -= 360; // handle wrapping

    pitch_controller.SetTunings(pitch_Kp, pitch_Ki, pitch_Kd);
    pitch_in = target_pitch - sensor_data.e_orientation.y();
    if(pitch_in < -180) pitch_in += 360; else if(pitch_in > 180) pitch_in -= 360;

    roll_controller.SetTunings(roll_Kp, roll_Ki, roll_Kd);
    roll_in = target_roll - sensor_data.e_orientation.z();
    if(roll_in < -180) roll_in += 360; else if(roll_in > 180) roll_in -= 360;

    depth_controller.SetTunings(depth_Kp, depth_Ki, depth_Kd);
    depth_in = target_pressure - sensor_data.water_pressure; // similar to above, input is error

    // allow PID to compute
    yaw_controller.Compute();
    depth_controller.Compute();
    pitch_controller.Compute();
    roll_controller.Compute();

    // update motor outputs
    thrust_l.setPower(thrust_base + yaw_out);
    thrust_r.setPower(thrust_base + yaw_out*-1);

    // mix outputs of depth, pitch, and roll into vertical motors
    // this simple approach works reasonably well as long as the sub is intended to be flat (ie 0* targets for pitch and roll)
    // the multipliers were included to make it easy to determine which direction the motors had to act - I'll leave it for the moment
    vert_fl.setPower(depth_out - (pitch_multiplier*pitch_out) + (roll_multiplier*roll_out));
    vert_bl.setPower(depth_out + (pitch_multiplier*pitch_out) + (roll_multiplier*roll_out));
    vert_fr.setPower(depth_out - (pitch_multiplier*pitch_out) - (roll_multiplier*roll_out));
    vert_br.setPower(depth_out + (pitch_multiplier*pitch_out) - (roll_multiplier*roll_out));
//  }

  delay(10); // 10 here + 20 depth read + bno read + serial parse <= 50 mS PID compute, hopefully
}

/*
 * pid:x:start/stop       3  where x is yaw/depth/pitch, 1 enables, 0 disables
 * pid:stop         2  disable all loops, doesn't stop motors
 * pid:x:dp,di,dd  3  set all three values in one command, comma-seperated
 * pid:read
 *
 * thr:stop  turn off all thrusters !!! this won't do anything useful unless pid is also disabled
 * thr:pilot   enter pilot mode, send a 'q' to exit, still line-based, so press enter to make stuff happen
 *  q    i
 *    j  k  l
 *       ,
 *
 * log:sensors/imu_cal/thr:nav:start/stop
 *
 * stop 1
 * //translate:x/y:duration
 * //rotate:z:delta_angle
 */

#define ERR_BAD_SYNTAX ("ERROR Bad syntax")

// for commands that need to wait for additional info/confirm, use a state machine, can't afford to block
void parseCommand(String cmd) {
  String msg = "";
  std::vector<String> args;
  str_util::split(cmd, ':', args);
  uint8_t num_args = args.size();
  if(ctrl_mode == MODE_CTRL_AUTO) {
    if(num_args > 0) {
      if(args[0].equals("pid")) {
        if(num_args == 2) {
          if(args[1].equals("read")) {
            msg = "CMD pid.read ->\n";
            msg += String("\tpid.yaw: ") + yaw_Kp + ", " + yaw_Ki + ", " + yaw_Kd + "\n";
            msg += String("\tpid.depth: ") + depth_Kp + ", " + depth_Ki + ", " + depth_Kd + "\n";
            msg += String("\tpid.pitch: ") + pitch_Kp + ", " + pitch_Ki + ", " + pitch_Kd + "\n";
            msg += String("\tpid.roll: ") + roll_Kp + ", " + roll_Ki + ", " + roll_Kd;
            Serial.println(msg);

          } else {
            // disable all controllers
            if(args[1].equals("stop")) {
              yaw_controller.SetMode(MANUAL);
              depth_controller.SetMode(MANUAL);
              pitch_controller.SetMode(MANUAL);
              roll_controller.SetMode(MANUAL);
              msg = Serial.println("CMD pid.all -> disabled");
            }
          }
        } else if(num_args == 3) {
          if(args[2].indexOf(',') > -1) {
            // set tuning parameters of a single controller
            std::vector<String> tunings;
            str_util::split(args[2], ',', tunings);
            if(tunings.size() == 3) {
              if(args[1].equals("yaw")) {
                yaw_Kp = tunings[0].toFloat();
                yaw_Ki = tunings[1].toFloat();
                yaw_Kd = tunings[2].toFloat();
                msg = String("CMD pid.yaw -> p = ") + yaw_Kp + ", i = " + yaw_Ki + ", d = " + yaw_Kd; Serial.println(msg);

              } else if(args[1].equals("depth")) {
                depth_Kp = tunings[0].toFloat();
                depth_Ki = tunings[1].toFloat();
                depth_Kd = tunings[2].toFloat();
                msg = String("CMD pid.depth -> p = ") + depth_Kp + ", i = " + depth_Ki + ", d = " + depth_Kd; Serial.println(msg);

              } else if(args[1].equals("pitch")) {
                pitch_Kp = tunings[0].toFloat();
                pitch_Ki = tunings[1].toFloat();
                pitch_Kd = tunings[2].toFloat();
                msg = String("CMD pid.pitch -> p = ") + pitch_Kp + ", i = " + pitch_Ki + ", d = " + pitch_Kd; Serial.println(msg);
              } else if(args[1].equals("roll")) {
                roll_Kp = tunings[0].toFloat();
                roll_Ki = tunings[1].toFloat();
                roll_Kd = tunings[2].toFloat();
                msg = String("CMD pid.roll -> p = ") + roll_Kp + ", i = " + roll_Ki + ", d = " + roll_Kd; Serial.println(msg);
              }
            }

          } else {
            // change mode on a per-controller basis
            int8_t mode = -1;
            if(args[2].equals("start")) mode = AUTOMATIC; else if(args[2].equals("stop")) mode = MANUAL;
            if(mode > -1) {
              if(args[1].equals("yaw")) {
                yaw_controller.SetMode(mode);
                msg = String("CMD pid.yaw -> ") + (mode > 0 ? "enabled" : "disabled"); Serial.println(msg);

              } else if(args[1].equals("depth")) {
                depth_controller.SetMode(mode);
                msg = String("CMD pid.depth -> ") + (mode > 0 ? "enabled" : "disabled"); Serial.println(msg);

              } else if(args[1].equals("pitch")) {
                pitch_controller.SetMode(mode);
                msg = String("CMD pid.pitch -> ") + (mode > 0 ? "enabled" : "disabled"); Serial.println(msg);

              } else if(args[1].equals("roll")) {
                roll_controller.SetMode(mode);
                msg = String("CMD pid.rol -> ") + (mode > 0 ? "enabled" : "disabled"); Serial.println(msg);
              }
            }
          }
        } else {
          Serial.println(ERR_BAD_SYNTAX);
        }

      } else if(args[0].equals("thr")) {
        if(num_args == 2) {
          if(args[1].equals("pilot")) {
            startPilot();
            Serial.println("CMD thr -> Entering pilot mode...");

          } else if(args[1].equals("stop")) {
            thrust_l.setPower(0);
            thrust_r.setPower(0);
            vert_fl.setPower(0);
            vert_bl.setPower(0);
            vert_fr.setPower(0);
            vert_br.setPower(0);
            Serial.println("CMD thr -> Stopped all motors");
          }

        } else {
          Serial.println(ERR_BAD_SYNTAX);
        }

      } else if(args[0].equals("config")) {
        if(num_args == 3) {
          if(args[1].equals("yaw")) {
            if(args[2].equals("lock")) {
              target_heading = sensor_data.e_orientation.x();
              msg = "CMD config.yaw -> Acquired heading lock: " + String(target_heading); Serial.println(msg);

            } else {
              float new_val = args[2].toFloat();
              char sign = args[2].charAt(0);
              if(sign == '+' || sign == '-') target_heading += new_val; else target_heading = new_val;

              msg = "CMD config.yaw -> Target heading set to " + String(target_heading); Serial.println(msg);
            }
          } else if(args[1].equals("depth")) {
            if(args[2].equals("lock")) {
              target_pressure = sensor_data.water_pressure;
              msg = "CMD config.depth -> Acquired depth lock: " + String(target_pressure) + " mbar"; Serial.println(msg);

            } else {
              float new_val = args[2].toFloat();
              char sign = args[2].charAt(0);
              if(sign == '+' || sign == '-') target_pressure += new_val; else target_pressure = new_val;
              msg = "CMD config.depth -> Target pressure set to " + String(target_pressure) + " mbar"; Serial.println(msg);
            }
          } else if(args[1].equals("pitch")) {
            if(args[2].equals("lock")) {
              target_pitch = sensor_data.e_orientation.y();
              msg = "CMD config.pitch -> Acquired pitch lock: " + String(target_pitch); Serial.println(msg);

            } else if(args[2].equals("invert")) {
              pitch_multiplier *= -1;
              msg = "CMD config.pitch -> Inverted pitch multiplier, new value: " + String(pitch_multiplier); Serial.println(msg);

            } else {
              float new_val = args[2].toFloat();
              char sign = args[2].charAt(0);
              if(sign == '+' || sign == '-') target_pitch += new_val; else target_pitch = new_val;
              msg = "CMD config.pitch -> Target pitch set to " + String(target_pitch); Serial.println(msg);
            }
          } else if(args[1].equals("roll")) {
            if(args[2].equals("lock")) {
              target_roll = sensor_data.e_orientation.z();
              msg = "CMD config.roll -> Acquired roll lock: " + String(target_roll); Serial.println(msg);

            }  else if(args[2].equals("invert")) {
              roll_multiplier *= -1;
              msg = "CMD config.roll -> Inverted roll multiplier, new value: " + String(roll_multiplier); Serial.println(msg);

            } else {
              float new_val = args[2].toFloat();
              char sign = args[2].charAt(0);
              if(sign == '+' || sign == '-') target_roll += new_val; else target_roll = new_val;
              msg = "CMD config.roll -> Target roll set to " + String(target_roll); Serial.println(msg);
            }
          }
        } else {
          Serial.println(ERR_BAD_SYNTAX);
        }

      } else if(args[0].equals("log")) {
        if(num_args == 3) {
          bool val = false;
          if(args[2].equals("start")) val = true;
          if(args[1].equals("sensors")) {
            log_sensors = val;
            msg = String("CMD log.sensors -> ") + (val > 0 ? "enabled" : "disabled"); Serial.println(msg);

          } else if(args[1].equals("imu_cal")) {
            log_imu_cal = val;
            msg = String("CMD log.imu_cal -> ") + (val > 0 ? "enabled" : "disabled"); Serial.println(msg);

          } else if(args[1].equals("thr")) {
            log_thr = val;
            msg = String("CMD log.thr -> ") + (val > 0 ? "enabled" : "disabled"); Serial.println(msg);

          } else if(args[1].equals("nav")) {
            log_nav = val;
            msg = String("CMD log.nav -> ") + (val > 0 ? "enabled" : "disabled"); Serial.println(msg);
          }
        } else {
          Serial.println(ERR_BAD_SYNTAX);
        }

      } else if(args[0].equals("stop") || args[0].equals("quit") || args[0].equals("exit") || args[0].equals("x") || args[0].equals("X")) {
        EStop();

      } else {
        Serial.println(ERR_BAD_SYNTAX);
      }
    }
  } else if(ctrl_mode == MODE_CTRL_PILOT) {
    if(num_args > 0) {
      if(args[0].equals("i")) {
        if(thrust_base > 0) {
          if(abs(thrust_base) < PILOT_BASE_LIM) thrust_base += PILOT_STEP_THRUST;
        } else {
          thrust_base = pilot_jump_thrust;
        }
        thrust_base = constrain(thrust_base, 0, PILOT_BASE_LIM);
        Serial.print("CMD pilot.Forward @ "); Serial.println(thrust_base);
      } else if(args[0].equals(",")) {
        if(thrust_base < 0) {
          if(abs(thrust_base) < PILOT_BASE_LIM) thrust_base -= PILOT_STEP_THRUST;
        } else {
          thrust_base = -1*pilot_jump_thrust;
        }
        thrust_base = constrain(thrust_base, -1*PILOT_BASE_LIM, 0);
        Serial.print("CMD pilot.Reverse @ "); Serial.println(thrust_base);
      } else if(args[0].equals("k")) {
        thrust_base = 0;
        Serial.println("CMD pilot.Stop");
      } else if(args[0].equals("j")) {
        target_heading += PILOT_STEP_YAW;
        Serial.print("CMD pilot.Left -> "); Serial.println(target_heading);
      } else if(args[0].equals("l")) {
        target_heading -= PILOT_STEP_YAW;
        Serial.print("CMD pilot.Right -> "); Serial.println(target_heading);
      } else if(args[0].equals("u")){
        target_pressure -= PILOT_STEP_PRESSURE;
        Serial.print("CMD pilot.Ascend -> "); Serial.println(target_pressure);
      } else if(args[0].equals("m")){
        target_pressure += PILOT_STEP_PRESSURE;
        Serial.print("CMD pilot.Descend -> "); Serial.println(target_pressure);
      } else if(args[0].equals("q")) {
        ctrl_mode = MODE_CTRL_AUTO;
        Serial.println("CMD pilot -> Exiting pilot mode...");
      } else {
        Serial.println(ERR_BAD_SYNTAX);
      }
    }
  }
}

void startPilot() {
  // lock depth
  parseCommand("config:depth:lock");
  // 0 the outputs
  yaw_out = depth_out = pitch_out = roll_out = 0;
  // set automatic mode on the controllers
  yaw_controller.SetMode(AUTOMATIC);
  depth_controller.SetMode(AUTOMATIC);
  pitch_controller.SetMode(AUTOMATIC);
  roll_controller.SetMode(AUTOMATIC);

  ctrl_mode = MODE_CTRL_PILOT;
}

void EStop() {
  // motors
  thrust_l.setPower(0);
  thrust_r.setPower(0);
  vert_fl.setPower(0);
  vert_bl.setPower(0);
  vert_fr.setPower(0);
  vert_br.setPower(0);
  // disable pid controllers
  yaw_controller.SetMode(MANUAL);
  depth_controller.SetMode(MANUAL);
  pitch_controller.SetMode(MANUAL);
  roll_controller.SetMode(MANUAL);
  // zero pid outputs
  yaw_out = 0;
  depth_out = 0;
  pitch_out = 0;
  roll_out = 0;

  Serial.println("CMD Emergency stop activated");
}

void log() {
  uint32_t now = millis();
  bool something_logged = false;
  if(log_sensors) {
    if(now > log_sensors_prev_millis + log_sensors_delay) {
      log_sensors_prev_millis = now; something_logged = true;
      msg = "INFO Pressure (mbar): " + String(sensor_data.water_pressure); Serial.println(msg);
      msg = "INFO Temperature (C): " + String(sensor_data.water_temp); Serial.println(msg);
      msg = "INFO Orientation (degrees): X = " + String(sensor_data.e_orientation.x())
        + " Y = " + String(sensor_data.e_orientation.y())
        + " Z = " + String(sensor_data.e_orientation.z()); Serial.println(msg);
    }
  }

  if(log_nav) {
    if(now > log_nav_prev_millis + log_nav_delay) {
      log_nav_prev_millis = now; something_logged = true;
      msg = String("INFO nav: yaw = ") + sensor_data.e_orientation.x() + " target = " + target_heading; Serial.println(msg);
      msg = String("INFO yaw pid: yaw_set = ") + yaw_set + " yaw_in = " + yaw_in + " yaw_out = " + yaw_out; Serial.println(msg);
      msg = String("INFO depth pid: depth_set = ") + depth_set + " depth_in = " + depth_in + " depth_out = " + depth_out; Serial.println(msg);
      msg = String("INFO pitch pid: pitch_set = ") + pitch_set + " pitch_in = " + pitch_in + " pitch_out = " + pitch_out; Serial.println(msg);
      msg = String("INFO roll pid: roll_set = ") + roll_set + " roll_in = " + roll_in + " roll_out = " + roll_out; Serial.println(msg);
    }
  }

  if(log_imu_cal) {
    if(now > log_imu_cal_prev_millis + log_imu_cal_delay) {
      log_imu_cal_prev_millis = now; something_logged = true;
      bno.getCalibration(&sys, &gyro, &accel, &mag); // this blocks the entire program if it fails to get the data
      msg = String("INFO Imu: Sys = ") + sys + ", Gyro = " + gyro + ", Accel = "
        + accel + ", Mag = " + mag; Serial.println(msg);
    }
  }

  if(log_thr) {
    if(now > log_thr_prev_millis + log_thr_delay) {
      log_thr_prev_millis = now; something_logged = true;
      msg = String("INFO Thrusters: L") + thrust_l.getPower() + " R" + thrust_r.getPower() + " | ";
      msg += String("FL") + vert_fl.getPower() + " BL" + vert_bl.getPower() + " BR" + vert_br.getPower() + " FR" + vert_fr.getPower();
      Serial.println(msg);
    }
  }

  if(something_logged) Serial.println(); // less useful that I had hoped since they don't necessarily print at the same time
}

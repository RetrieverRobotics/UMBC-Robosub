#include <PID_v1.h>

#include <Adafruit_BNO055.h>

#include <font_Arial.h>
#include <font_ArialBold.h>
#include <ILI9341_t3.h>

#include <Servo.h>

// wraps the handling of a Blue Robotics
// brushless motor esc as a Servo
// maybe also wrap a pid controller, reversal, etc
class BLThruster {
  private:
  Servo esc; // esc = electronic speed controller
  int pwm_pin;
  boolean reversed;

  int power;

  const static int ESC_MIN;
  const static int ESC_MAX;
  const static int ESC_HALF;
  const static int ESC_CENTER;
  const static int ESC_RANGE;

  void update() {
    // pid if implemented, update esc pwm
    int actual_power = (reversed ? -1*power : power);
    int us = map(actual_power, -100, 100, BLThruster::ESC_MIN, BLThruster::ESC_MAX);
    esc.writeMicroseconds(us);
  }
  
  public:
  BLThruster(int _pwm_pin, boolean _reversed = false) {
    pwm_pin = _pwm_pin;
    esc.attach(pwm_pin);
    // blue robotics esc has forward reverse capability,
    // so write a value in the middle of range to turn it off
    esc.writeMicroseconds(BLThruster::ESC_CENTER);

    reversed = _reversed;
  }

  void setReverse(bool val) {
    reversed = val;
  }

  void setPower(int _power) {
    // not applying reversal here b/c a call to getPower()
    // might return a confusing result
    power = _power;
    update();
  }

  int getPower(void) {
    return power;
  }
};
// these are static (shared between instances),
// which makes the initialization weird
const int BLThruster::ESC_MIN = 1100;
const int BLThruster::ESC_MAX = 1900;
const int BLThruster::ESC_HALF = (BLThruster::ESC_MAX - BLThruster::ESC_MIN)/2;
const int BLThruster::ESC_CENTER = BLThruster::ESC_MIN + BLThruster::ESC_HALF;
const int BLThruster::ESC_RANGE = BLThruster::ESC_MAX - BLThruster::ESC_HALF;

#define TFT_DC (9)
#define TFT_CS (10)

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);

// light sensor: http://www.everlight.com/file/ProductFile/ALS-PDIC144-6C-L378.pdf
// sensor(+) => 3.3V
// sensor(-) => 100k resistor, analog_read
// 100k resistor => GND
#define SENSOR_LIGHT (15)

Adafruit_BNO055 bno = Adafruit_BNO055(55);

int light_trigger = -1;

uint8_t sys, gyro, accel, mag;

// pin defs correct
// wiring configured such that (+) is down for vertical, (+) is forward for thrust
BLThruster vert_fl(3), vert_bl(4), vert_fr(6), vert_br(7), thrust_l(2), thrust_r(5);

double setpoint, input, output;
double Kp = 0.5, Ki = 0, Kd = 0.1;
PID heading_controller(&input, &output, &setpoint, Kp, Ki, Kd, DIRECT);

void setup() {
  Serial.begin(115200);
  Serial.println("Orientation Sensor Test");

  while(!bno.begin(Adafruit_BNO055::OPERATION_MODE_NDOF)) {
    Serial.println("Sensor not detected - find an appropriate wall.");
    delay(3000);
  }
  delay(1000);

  bno.setExtCrystalUse(true);

  sys = gyro = accel = mag = 0;

  restoreOffsets();

  // setup tft
  tft.begin();
  tft.fillScreen(ILI9341_WHITE);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(2);
  
  tft.setRotation(2);
  tft.setCursor(0,5);

  // calibrate light sensor
  tft.println(" Light cal...");
  int light_max = 0;
  for(int i = 0; i < 50; i++) {
    int tmp = getLightMean();
    if(tmp > light_max) light_max = tmp;
  }
  tft.print(" Max: ");
  tft.println(light_max);
  // literally a magic number, dependent on sample frequency and length of calibration period
  light_trigger = light_max + 200;
  tft.println(" Done.");
  delay(1000);

  
  delay(1000);
  
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
}

imu::Vector<3> euler;

unsigned int pwr = 0;
unsigned int HEADING_MAX_ERROR = 5; // +- degrees
unsigned int TASK_START_DELAY = 10*1000;
unsigned int TASK_1_DUR = 10*1000;
unsigned int TASK_2_DUR = 60*1000;

void loop() {
  int program = getUserInput("Select Program", 5, 0);
  delay(1000);
  String str_confirm = "Confirm " + String(program);
  if(program > -1 && getUserInput(str_confirm, 1, 5000) > -1) {
    switch(program) {
      case 0: // IMU CAL
      {
        tft.setCursor(0, 5);
        tft.println(" IMU cal...");
        int cnt = 0;
        // at 50 mS delay, 100 counts is 5 seconds solid (cumulative)
        while(cnt < 100) {
          bno.getCalibration(&sys, &gyro, &accel, &mag);
          tft.fillRect(0, 0, 240, 40, ILI9341_BLACK);
          tft.setCursor(0, 5);
          tft.print(" ");
          tft.print(sys);
          tft.print(" : "); 
          tft.print(gyro);
          tft.print(" ");
          tft.print(accel);
          tft.print(" ");
          tft.println(mag);
          tft.print(" ");
          tft.println(cnt);
          // *** CALIBRATION REQUIREMENTS ***
          if(sys >= 1 && gyro == 3 && mag >= 1) ++cnt;
          // ***
          delay(50);
        }
        tft.println(" Done.");
        tft.println(" Continuing in 3s");
        delay(3000);
      }
        break;
      case 1: // DISPLAY ORIENTATION
      {
        long millis_start = millis();
        while(millis() - millis_start < TASK_1_DUR) {
          euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
          tft.fillRect(0, 0, 80, 60, ILI9341_BLACK);
          tft.setCursor(0, 5);
          tft.print(" x:");
          tft.println((int)euler.x(), DEC);
          tft.print(" y:");
          tft.println((int)euler.y(), DEC);
          tft.print(" z:");
          tft.println((int)euler.z(), DEC);
          delay(20);
        }
        
      }
        break;
      case 2: // LOCK HEADING
      {
        tft.setCursor(0, 5);
        String msg = "Heading Lock in " + String(TASK_START_DELAY/1000);
        tft.println(msg);
        delay(TASK_START_DELAY); // !!!
        long millis_start = millis();

        euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
        int heading = euler.x();

        setpoint = 0; // error is calculated relative to lock point, so 0 is best error
        input = 0;
        output = 0;
        heading_controller.SetOutputLimits(-100, 100);
        heading_controller.SetSampleTime(50); // 50 mS between computes
        heading_controller.SetMode(AUTOMATIC);

        tft.fillScreen(ILI9341_RED);

        //while(true) {
        while(millis() - millis_start < TASK_2_DUR) {
          euler = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
          int yaw = euler.x();
          int heading_error = 0;

          // calculate error
          heading_error = heading - yaw;
          // wrap it -> +-180
          if(heading_error < -180) {
            heading_error += 360;
          } else if(heading_error > 180) {
            heading_error -= 360;
          }
          
          input = heading_error;

          heading_controller.Compute();

          Serial.print("error:");
          Serial.println(heading_error);
          Serial.print("output:");
          Serial.println(output);

          thrust_l.setPower(output);
          thrust_r.setPower(-1*output);
          
          Serial.print("L: ");
          Serial.print(thrust_l.getPower());
          Serial.print(" R: ");
          Serial.println(thrust_r.getPower());

          delay(25);
          
        }
        heading_controller.SetMode(MANUAL);
        // stop motors at end of task
        thrust_l.setPower(0);
        thrust_r.setPower(0);
        tft.println(" Done.");
      }
        break;
      case 3: // SET MOTOR POWER LEVEL
      {
        // get user preference for new power
        int num_levels = 4;
        int pwr_levels[] = { 0, 7, 25, 50 };
        bool success = false;
        String msg = "PWR";
        for(int i = 0 ; i < num_levels; i++) {
          msg += " " + String(pwr_levels[i]);
        }
        int pwr_index = getUserInput(msg, num_levels, 0);
        if(pwr_index > -1) { // kinda has to be - input doesn't time out, but still
          delay(1000);
          String msg = "Confirm " + String(pwr_levels[pwr_index]);
          // confirm selection, then set power
          if(getUserInput(msg, 1, 3000) > -1) {
            pwr = pwr_levels[pwr_index];
            success = true;
          } 
        }
        tft.setCursor(0,5);
        tft.println( (success ? " Value changed." : " Canceled.") );
      }
        break;
      case 4:
      {
        // pid parameter tunings
        double delta_opts[] = { -1, -0.1, -0.01, 0.01, 0.1, 1 };
        delay(1000);
        int delta_opt = getUserInput("Select delta", 6, 5*1000);
        
        if(delta_opt > -1) {
          delay(1000);
          int multiplier = getUserInput("Select mult", 10, 5*1000);

          if(multiplier > 0) { // multiplying by zero wouldn't do anything...
            delay(1000);
            int param = getUserInput("Select param", 3, 5*1000);
          
            if(param > -1) {
              delay(1000);
              
              float val = multiplier*delta_opts[delta_opt];
              
              String msg = "Confirm: " + String(val) + " ?";
              if(getUserInput(msg, 1, 5*1000) > -1) {
                switch(param) {
                  case 0:
                    Kp += val;
                    //Kp = constrain(Kp, 0, 5);
                    break;
                  case 1:
                    Ki += val;
                    //Ki = constrain(Ki, 0, 20);
                    break;
                  case 2:
                    Kd += val;
                    //Kd = constrain(Kd, 0, 20);
                    break;
                }
                tft.setCursor(0, 5);
                tft.print(" ");
                tft.print(Kp);
                tft.print(" ");
                tft.print(Ki);
                tft.print(" ");
                tft.print(Kd);
                heading_controller.SetTunings(Kp, Ki, Kd);
  
                delay(3000);
              }
            }
          }
        }
      }
        break;
    }

    delay(3000);
  }
}

// number of options - each option adds 1 second
// timeout in milliseconds
int getUserInput(String info, int num_opts, int timeout) {
  if(num_opts <= 0) return -1;
  long millis_start = millis();
  int ticks = -1;
  int exit_condition = -1;
  while(true) {
    long sys_time = millis();
    if(timeout > 0 && sys_time - millis_start > timeout && ticks == -1) break;
    
    // check tick
    int val = getLightMean();
    if(val > light_trigger) {
      Serial.println(val - light_trigger);
      ticks++;

      // if exceeded max time (200mS * 5 = 1S / opt), return failure
      if(ticks >= num_opts * 5) break;
    } else if(ticks > -1) {
      exit_condition = 0;
      break;
    }

    tft.setTextSize(2);
    tft.setTextColor(ILI9341_BLACK);
    tft.fillRect(0, 0, 240, 100, ILI9341_WHITE);

    // show info bar
    tft.setCursor(0, 5);
    tft.print(" ");
    tft.println(info);
    
    // show timeout counter
    if(timeout > 0) {
      if(ticks == -1) {
        tft.println(timeout - (sys_time - millis_start));
      } else {
        tft.println("waiting...");
      }
    }
    
    // show tick count
    tft.print(" ");
    tft.println(ticks);
    
    // show available options
    tft.print(" ");
    for(int i = 0; i < num_opts; i++) {
      if(ticks > -1 && ticks/5 == i) tft.setTextColor(ILI9341_GREEN); else tft.setTextColor(ILI9341_RED);
      tft.print(i);
      tft.print("   ");
    }
  }

  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);

  if(exit_condition == 0) {
    return ticks/5;
  } else {
    return -1;
  }
}

// slower reads -> higher (more stable?) voltage
// measurement device reading fast lowers voltage at sensor b/c tiny current
// 20,10 is most stable yet -> 200 ms period
int getLightMean() {
  int sum = 0;
  const byte NUM_SAMPLES = 100;
  const byte SAMPLE_DELAY = 2; // milliseconds
  for(int i = 0; i < NUM_SAMPLES; i++) {
    sum += analogRead(SENSOR_LIGHT);
    delay(SAMPLE_DELAY);
  }
  return sum / NUM_SAMPLES;
}

void printCalibData() {
  uint8_t sys, gyro, accel, mag;
  sys = gyro = accel = mag = 0;
  bno.getCalibration(&sys, &gyro, &accel, &mag);

  Serial.print("S:");
  Serial.print(sys, DEC);
  Serial.print(" G:");
  Serial.print(gyro, DEC);
  Serial.print(" A:");
  Serial.print(accel, DEC);
  Serial.print(" M:");
  Serial.println(mag, DEC);
}

// doesn't seem to do much good
void restoreOffsets() {
  adafruit_bno055_offsets_t offsets;
  
  offsets.accel_offset_x = 65523;
  offsets.accel_offset_y = 65484;
  offsets.accel_offset_z = 22;
  
  offsets.gyro_offset_x = 65535;
  offsets.gyro_offset_y = 65535;
  offsets.gyro_offset_z = 0;
  
  offsets.mag_offset_x = 65150;
  offsets.mag_offset_y = 65287;
  offsets.mag_offset_z = 305;
  
  offsets.accel_radius = 1000;
  offsets.mag_radius = 724;

  bno.setSensorOffsets(offsets);
}


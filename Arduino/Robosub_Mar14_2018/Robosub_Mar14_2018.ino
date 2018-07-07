#include <Adafruit_BNO055.h>

#include <font_Arial.h>
#include <font_ArialBold.h>
#include <ILI9341_t3.h>

#include "BLThruster.h"

#include "TaskManager.h"

#define TFT_DC (9)
#define TFT_CS (10)

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);

#define SENSOR_LIGHT (15)

Adafruit_BNO055 bno = Adafruit_BNO055(55);

int light_trigger = -1;

uint8_t sys, gyro, accel, mag;

// pin defs correct
// wiring configured such that (+) is down for vertical, (+) is forward for thrust
BLThruster vert_fl(3), vert_bl(4), vert_fr(6), vert_br(7), thrust_l(2), thrust_r(5);




TaskManager manager;

#define TASK_IDLE (0)
#define TASK_WORKING (5)
#define TASK_DONE (10)

Task T_read_light;

int read_light_sum = 0;
int read_light_count = 0;
int read_light_num_samples = 50;
uint8_t read_light_state = TASK_IDLE;
void read_light_update(void) {
  if(read_light_count < read_light_num_samples) {
    read_light_sum += analogRead(SENSOR_LIGHT);
    ++read_light_count;
  } else {
    read_light_state = TASK_DONE;
    task_pause(&T_read_light);
  }
}
void read_light_reset() {
  read_light_sum = 0;
  read_light_count = 0;
  read_light_state = TASK_IDLE;
}
void read_light_start() {
  read_light_reset();
  read_light_state = TASK_WORKING;
  task_continue(&T_read_light);
}
int read_light_getMean(void) {
  if(read_light_state == TASK_DONE) {
    read_light_state = TASK_IDLE;
    return read_light_sum / read_light_num_samples;
  } else {
    return -1;
  }
}

Task T_get_input;

void get_input_update(void) {

}


void setup() {
  Serial.begin(115200);
  while(!Serial);

  manager_init(&manager);

  task_init(&T_read_light, read_light_update, 1, TASK_PAUSED);
  task_init(&T_get_input, get_input_update, 5, TASK_PAUSED);

  manager_register_task(&manager, &T_read_light);
  manager_register_task(&manager, &T_get_input);

  Serial.println("Ready.\n");

/*
  Serial.println("Orientation Sensor Test");

  while(!bno.begin(Adafruit_BNO055::OPERATION_MODE_NDOF)) {
    Serial.println("Sensor not detected - find an appropriate wall.");
    delay(3000);
  }
  delay(1000);

  bno.setExtCrystalUse(true);

  sys = gyro = accel = mag = 0;
*/
  
  calibrateLightSensor();

  
}

void loop() {
  manager_update(&manager);


}




void calibrateLightSensor() {
  Serial.println("Calibrating light sensor : detecting maximum value...");
  int light_max = 0;
  // current settings use 50 mS blocks, nominally non-blocking... but that's why we have loops!
  for(int i = 0; i < 100; i++) { // 5 seconds
    read_light_start();
    while(read_light_state == TASK_WORKING) {
      manager_update(&manager);
    }
    int tmp = read_light_getMean();
    if(tmp > light_max) light_max = tmp;
  }
  Serial.print("Max: ");
  Serial.println(light_max);
  int buffer = 200;
  light_trigger = light_max + buffer;
  Serial.print("Trigger value: ");
  Serial.println(light_trigger);
  Serial.println("Done light calibration.");
}

// number of options - each option adds 1 second
// timeout in milliseconds
// number of options - each option adds 1 second
// timeout in milliseconds
int getUserInputBlocking(String info, int num_opts, int timeout) {
  if(num_opts <= 0) return -1;
  long millis_start = millis();
  int ticks = -1;
  int exit_condition = -1;
  while(true) {
    long sys_time = millis();
    if(timeout > 0 && sys_time - millis_start > timeout && ticks == -1) break;
    
    // check tick
    int val = getLightMeanBlocking();
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

// use fast sample rate <= 2mS to push voltage down at low
// light levels -> more useable range
// use higher sample count for more stable reading
int getLightMeanBlocking() {
  int sum = 0;
  // at 20-30, the mean is highly variable depending on how often function
  // is called, 50 is a little more stable: 10mS and 100mS between calls both yielded
  // a max around 200 (in a given lighting situation).
  // Over time, this value crept to 250 or so. Use at least 100 for buffer.
  uint16_t num_samples = 50; 
  uint16_t sample_delay = 1;
  for(int i = 0; i < num_samples; i++) {
    sum += analogRead(SENSOR_LIGHT);
    delay(sample_delay);
  }
  return sum / num_samples;
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


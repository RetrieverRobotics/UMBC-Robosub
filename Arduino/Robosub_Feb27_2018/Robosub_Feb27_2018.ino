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
    power = (reversed ? -1*power : power);
    int us = map(power, -100, 100, BLThruster::ESC_MIN, BLThruster::ESC_MAX);
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

#define SENSOR_LIGHT (15)

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);

#define NUM_MOTORS (6)

// pin defs correct
// wiring configured such that (+) is down for vertical, (+) is forward for thrust
BLThruster vert_fl(3), vert_bl(4), vert_fr(6), vert_br(7), thrust_l(2), thrust_r(5);

String motor_names[] = { "V_FL", "V_BL", "V_FR", "V_BR", "TH_L", "TH_R" };
BLThruster *motors[] = { &vert_fl, &vert_bl, &vert_fr, &vert_br, &thrust_l, &thrust_r };

String task_names[6] = {"", ""};
void (*tasks[6])();

int light_trigger = -1;

void setup() {
  Serial.begin(115200);

  tasks[0] = down25;
  task_names[0] = "down 25%";
  tasks[1] = down50;
  task_names[1] = "down 50%";
  tasks[2] = down100;
  task_names[2] = "(dis) down 100%";
  
  tasks[3] = thrust_test25;
  task_names[3] = "thrust 25%";
  tasks[4] = thrust_test50;
  task_names[4] = "thrust 50%";
  tasks[5] = thrust_test100;
  task_names[5] = "thrust 100%";
  
  // setup tft
  tft.begin();
  tft.fillScreen(ILI9341_WHITE);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(2);
  
  tft.setRotation(2);
  
  // calibrate light sensor
  tft.println("Starting calibration\n(5s)...");
  int light_max = 0;
  for(int i = 0; i < 50; i++) {
    int tmp = getLightMean();
    if(tmp > light_max) light_max = tmp;
  }
  tft.print("Max: ");
  tft.println(light_max);
  // literally a magic number, dependent on sample frequency and length of calibration period
  light_trigger = light_max + 200;
  tft.println("Done.");
  delay(1000);
  
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
}

void loop() {
  int opt = getUserInput("Select task:", 6, 0); // blacks out the background when finished

  if(opt > -1) {
    tft.setCursor(0, 5);
    tft.println("Selected");
    tft.println(task_names[opt]);
    delay(1000);
    
    int confirm = getUserInput("Confirm:", 1, 3000);
    if(confirm > -1) {
      tft.fillRect(0, 0, 240, 100, ILI9341_BLACK);
      tft.setCursor(0, 5);
      tft.println("Running ");
      tft.print("[");
      tft.print(task_names[opt]);
      tft.println("]");
      tft.println("in 30 secs...");
      delay(30*1000);
      tft.println("Started task");
      
      tasks[opt]();
      
      tft.println("Done.");
      delay(1000);
    }
  }
}

void down(int power, int duration) {
  vert_fl.setPower(power);
  vert_bl.setPower(power);
  vert_fr.setPower(power);
  vert_br.setPower(power);
  delay(duration);
  vert_fl.setPower(0);
  vert_bl.setPower(0);
  vert_fr.setPower(0);
  vert_br.setPower(0);
}

// to be used as function pointers
void down25() {
  down(25, 7000);
}

void down50() {
  down(50, 4000);
}

void down100() {
  //down(100, 2000);
}

void thrust(int power, int duration) {
  thrust_l.setPower(power);
  thrust_r.setPower(power);
  delay(duration);
  thrust_l.setPower(0);
  thrust_r.setPower(0);
}

// to be used as function pointers
void thrust_test25() {
  thrust(25, 7000);
  delay(2000);
  thrust(-25, 7000);
}

void thrust_test50() {
  thrust(50, 4000);
  delay(2000);
  thrust(-50, 4000);
}

void thrust_test100() {
  thrust(100, 2000);
  delay(2000);
  thrust(-100, 2000);
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

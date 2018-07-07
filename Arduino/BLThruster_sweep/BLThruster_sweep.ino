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
  const static int ESC_RANGE;

  void update() {
    // pid if implemented, update esc pwm
    power = (reversed ? -1*power : power);
    int us = map(power, -100, 100, -1*BLThruster::ESC_RANGE, BLThruster::ESC_RANGE);
    esc.writeMicroseconds(us);
  }
  
  public:
  BLThruster(int _pwm_pin, boolean _reversed = false) {
    pwm_pin = _pwm_pin;
    esc.attach(pwm_pin);
    // blue robotics esc has forward reverse capability,
    // so write a value in the middle of range to turn it off
    esc.writeMicroseconds(BLThruster::ESC_HALF);

    reversed = _reversed;
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
const int BLThruster::ESC_RANGE = BLThruster::ESC_MAX - BLThruster::ESC_HALF;

BLThruster VertFL(2);
BLThruster VertFR(3);
BLThruster VertBL(4);
BLThruster VertBR(5);
BLThruster HorL(6);
BLThruster HorR(7);

#define NUM_THRUSTERS (6)

BLThruster *thrusters[NUM_THRUSTERS];

#define START_PIN (14)

void setup() {
  Serial.begin(19200);

  pinMode(START_PIN, INPUT_PULLUP);
  pinMode(13, OUTPUT);

  thrusters[0] = &VertFL;
  thrusters[1] = &VertFR;
  thrusters[2] = &VertBL;
  thrusters[3] = &VertBR;
  thrusters[4] = &HorL;
  thrusters[5] = &HorR;
}

#define POWER_MAX (20)
#define POWER_MIN (-1*POWER_MAX)
#define DURATION_HALF_TO_FULL (5000)
#define INTERVAL (DURATION_HALF_TO_FULL/POWER_MAX)

void loop() {
  if(!digitalRead(START_PIN)) {
    for(int i = 0; i < NUM_THRUSTERS; i++) {
      // thrusters initialized to zero at program start
      // so 0 -> max -> min -> 0

      for(int p = 0; p < POWER_MAX; p++) {
        thrusters[i]->setPower(p);  
        Serial.print(i);
        Serial.print(": ");
        Serial.println(p);
        delay(INTERVAL);
      }
      for(int p = POWER_MAX; p > POWER_MIN; p--) {
        thrusters[i]->setPower(p);
        Serial.print(i);
        Serial.print(": ");
        Serial.println(p);
        delay(INTERVAL);
      }
      for(int p = POWER_MIN; p < 0; p++) {
        thrusters[i]->setPower(p);
        Serial.print(i);
        Serial.print(": ");
        Serial.println(p);
        delay(INTERVAL);
      }
    }
  }
}

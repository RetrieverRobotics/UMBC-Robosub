#include <Arduino.h>
#include <Servo.h>

// wraps the handling of a Blue Robotics
// brushless motor esc as a Servo
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
    power = _power;

    // update esc pwm
    int actual_power = (reversed ? -1*power : power);
    int us = map(actual_power, -100, 100, BLThruster::ESC_MIN, BLThruster::ESC_MAX);
    esc.writeMicroseconds(us);
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

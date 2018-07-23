#include "BLThruster.h"

// these are static (shared between instances),
// which makes the initialization weird
const int BLThruster::ESC_MIN = 1100;
const int BLThruster::ESC_MAX = 1900;
const int BLThruster::ESC_HALF = (BLThruster::ESC_MAX - BLThruster::ESC_MIN)/2;
const int BLThruster::ESC_CENTER = BLThruster::ESC_MIN + BLThruster::ESC_HALF;
const int BLThruster::ESC_RANGE = BLThruster::ESC_MAX - BLThruster::ESC_HALF;

// NO SERIAL CALLS IN CONSTRUCTORS BEFORE SERIAL HAS BEEN INITIALIZED
// me... no! I wasn't yelling.
BLThruster::BLThruster(uint8_t _pwm_pin, bool _reversed) {
  pwm_pin = _pwm_pin;
  esc.attach(pwm_pin);
  // blue robotics esc has forward reverse capability,
  // so write a value in the middle of range to turn it off
  esc.writeMicroseconds(BLThruster::ESC_CENTER);

  reversed = _reversed;
}

void BLThruster::setReverse(bool _reversed) {
  reversed = _reversed;
}

void BLThruster::setPower(int8_t _power) {
  //power = _power;
  power = constrain(_power, -100, 100);

  // update esc pwm
  int8_t actual_power = (reversed ? -1*power : power);
  uint16_t us = map(actual_power, -100, 100, BLThruster::ESC_MIN, BLThruster::ESC_MAX);
  esc.writeMicroseconds(us);
}

int8_t BLThruster::getPower(void) {
  return power;
}

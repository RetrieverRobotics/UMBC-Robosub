#include <Arduino.h>
#include <Servo.h>

// wraps the handling of a Blue Robotics
// brushless motor esc as a Servo
class BLThruster {
public:
  BLThruster(uint8_t, bool = false);
  // default constructor needed in order to use [] type access for STL containers
  BLThruster();

  void setReverse(bool);

  void setPower(int8_t);

  int8_t getPower(void);

  const static int PWR_MIN;
  const static int PWR_MAX;
  const static int PWR_CENTER;

private:
  Servo esc; // esc = electronic speed controller
  int pwm_pin;
  bool reversed;

  int power;

  const static int ESC_MIN;
  const static int ESC_MAX;
  const static int ESC_HALF;
  const static int ESC_CENTER;
  const static int ESC_RANGE;
};

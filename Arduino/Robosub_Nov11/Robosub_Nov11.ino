#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

Adafruit_BNO055 bno = Adafruit_BNO055();
//adafruit_bno055_offsets_t calibration; //11 values that can all by set to 65535

imu::Vector<3> data;
float heading = 0.0;

void setup() {
  pinMode(7, INPUT_PULLUP);
  Serial.begin(115200);
  if(!bno.begin()) {
    Serial.println("No BNO055 detected.");
    while(1);
  }

  bno.setExtCrystalUse(true);

  bno.setMode(Adafruit_BNO055::OPERATION_MODE_NDOF);

  
  uint8_t s, g, a, m;
  bno.getCalibration(&s, &g, &a, &m);
  while(true) {
    bno.getCalibration(&s, &g, &a, &m);
    //if(s+g+m >= 9) break;
    Serial.print("s:");
    Serial.print(s);
    Serial.print(" g:");
    Serial.print(g);
    Serial.print(" a:");
    Serial.print(a);
    Serial.print(" m:");
    Serial.print(m);
    
    Serial.println();
    delay(200);

    if(!digitalRead(7)) break;
  }
  

  Serial.println("Checking heading...");
  delay(1000);
  data = bno.getVector(Adafruit_BNO055::VECTOR_EULER);
  heading = data.x();
  Serial.println("Initial heading acquired.");
  delay(1000);
}

float error_threshold = 3; // number of degrees error before applying corrections

void loop() {
  // Possible vector values can be:
  // - VECTOR_ACCELERMOETER - m/s^2
  // - VECTOR_MAGNETOMETER  - uT
  // - VECTOR_GYROSCOPE     - rad/s
  // - VECTOR_EULER         - degrees
  // - VECTOR_LINEARACCEL   - m/s^2
  // - VECTOR_GRAVITY       - m/s^2

  data = bno.getVector(Adafruit_BNO055::VECTOR_EULER);

  Serial.print("x:");
  Serial.print(data.x());
  Serial.print(" y:");
  Serial.print(data.y());
  Serial.print(" z:");
  Serial.println(data.z());

  if(Serial.available() > 0) {
    
  }
  
  
  // max fusion rate is 100 Hz, meaning min 10 ms rate
  delay(100);

  // note: without calibration, the yaw angle appears stable as long
  // as the sensor stays more or less flat.
}

/*
   360-0
270     90
    180
*/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>

/*
 * 2^16 - 1
 * 
 * 
 * Cal set 1 @ Nov. 15, 2017
 * (NDOF) 
 * A: 65523 65478 22
 * G: 65535 65535 0
 * M: 65146 65286 305
 * A_rad: 1000
 * M_rad: 735
 * 
 * Cal set 1 @ Nov. 15, 2017
 * (IMUPLUS)
 * A: 65523 65488 17
 * G: 0 65535 0
 * M: 65163 65299 299
 * A_rad: 1000
 * M_rad: 718
 * 
 * Cal set 2 @ Nov. 15, 2017
 * (NDOF)
 * A: 65523 65491 22
 * G: 65535 65535 1
 * M: 65157 65288 306
 * A_rad: 1000
 * M_rad: 712
 * 
 * Cal set 2 @ Nov. 15, 2017
 * (IMUPLUS)
 * A: 65526 65494 19
 * G: 65535 65535 0
 * M: 65165 65271 299
 * A_rad: 100
 * M_rad: 724
 */

Adafruit_BNO055 bno = Adafruit_BNO055(55);

void displayCalStatus(void)
{
    /* Get the four calibration values (0..3) */
    /* Any sensor data reporting 0 should be ignored, */
    /* 3 means 'fully calibrated" */
    uint8_t system, gyro, accel, mag;
    system = gyro = accel = mag = 0;
    bno.getCalibration(&system, &gyro, &accel, &mag);

    /* Display the individual values */
    Serial.print("Sys:");
    Serial.print(system, DEC);
    Serial.print(" G:");
    Serial.print(gyro, DEC);
    Serial.print(" A:");
    Serial.print(accel, DEC);
    Serial.print(" M:");
    Serial.print(mag, DEC);
    Serial.println();
}

void displaySensorOffsets(void)
{
    adafruit_bno055_offsets_t offsets;
    bno.getSensorOffsets(offsets);
    
    Serial.print("Accelerometer: ");
    Serial.print(offsets.accel_offset_x); Serial.print(" ");
    Serial.print(offsets.accel_offset_y); Serial.print(" ");
    Serial.print(offsets.accel_offset_z); Serial.print(" ");

    Serial.print("\nGyro: ");
    Serial.print(offsets.gyro_offset_x); Serial.print(" ");
    Serial.print(offsets.gyro_offset_y); Serial.print(" ");
    Serial.print(offsets.gyro_offset_z); Serial.print(" ");

    Serial.print("\nMag: ");
    Serial.print(offsets.mag_offset_x); Serial.print(" ");
    Serial.print(offsets.mag_offset_y); Serial.print(" ");
    Serial.print(offsets.mag_offset_z); Serial.print(" ");

    Serial.print("\nAccel Radius: ");
    Serial.print(offsets.accel_radius);

    Serial.print("\nMag Radius: ");
    Serial.print(offsets.mag_radius);
    Serial.println();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  if(!bno.begin()) {
    Serial.println("No BNO055 detected.");
    while(1);
  }

  bno.setExtCrystalUse(true);
  bno.setMode(Adafruit_BNO055::OPERATION_MODE_NDOF);
}

void loop() {
  // put your main code here, to run repeatedly:

  displayCalStatus();

  if(Serial.available() > 0) {
    displaySensorOffsets();
    while(1);
  }
  delay(300);
}

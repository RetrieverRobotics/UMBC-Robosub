
bool IMU_SETUP_GOOD = false;
void setupIMU() {
#ifdef IMU_BNO055
  Serial.println("BOOT Initializing IMU...");
  unsigned long init_begin = millis();
  unsigned int timeout = 5000;
  int cnt = 1;
  while(millis() - init_begin < timeout) {
    if(bno055.begin(Adafruit_BNO055::OPERATION_MODE_NDOF)) {
      IMU_SETUP_GOOD = true;
      break;
    }
    String msg = "BOOT\tIMU Init - Attempt " + String(cnt) + " failed.";
    Serial.println(msg);
    ++cnt;
    delay(500);
  }
  if(IMU_SETUP_GOOD) {
    Serial.println(F("BOOT\tIMU found."));
    uint8_t sys, gyro, accel, mag;
    bno055.getCalibration(&sys, &gyro, &accel, &mag); // this blocks the entire program if it fails to get the data
    String msg = String("INFO Imu: Sys = ") + sys + ", Gyro = " + gyro + ", Accel = " + accel + ", Mag = " + mag; Serial.println(msg);
  } else {
    Serial.println(F("BOOT\tIMU initialization failed. Continuing... even though it's pointless."));
  }

  bno055.setExtCrystalUse(true);

#elif defined(IMU_MPU6050)
  Serial.println("BOOT Initializing IMU...");
  Wire.begin();
  mpu6050.begin();
  Serial.println("BOOT IMU initialized if connected correctly.");
  Serial.println("BOOT Beginning IMU Calibration - DO NOT MOVE");
  mpu6050.calcGyroOffsets(false); // do not write to console
  String msg = "BOOT\tIMU gyro calibration complete: " + String(mpu6050.getGyroXoffset()) + ", "
    + String(mpu6050.getGyroYoffset()) + ", " + String(mpu6050.getGyroZoffset());
  Serial.println(msg);

/*
cal log:
-13.29, 1.76, -0.73
-13.14, 1.77, -0.71
-13.25, 1.65, -0.69

*/

#endif
}


void setupPeripherals() {
  setupIMU();
  ms5803.begin();
}



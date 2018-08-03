
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
    String msg = String("//BOOT Imu: Sys = ") + sys + ", Gyro = " + gyro + ", Accel = " + accel + ", Mag = " + mag; Serial.println(msg);
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

#elif defined(IMU_MPU9250)
  Serial.println("BOOT Initializing IMU...");
  Wire.begin();

  byte c = mpu9250.readByte(MPU9250_ADDRESS, WHO_AM_I_MPU9250);
  if(c == 0x73) {
    Serial.println("BOOT IMU Self test (lower is better)...");
    Serial.print("BOOT\t");
    mpu9250.MPU9250SelfTest(mpu9250.SelfTest);
    for(int i = 0; i < 6; ++i) {
      Serial.print(mpu9250.SelfTest[i], 1);
      Serial.print(" ");
    }
    Serial.println();

    Serial.println("BOOT Starting IMU Cal! DO NOT MOVE");
    mpu9250.calibrateMPU9250(mpu9250.gyroBias, mpu9250.accelBias);
    Serial.println("BOOT\tIMU Cal complete.");

    mpu9250.initMPU9250();
    Serial.println("BOOT IMU init complete - ACC/GYR/TEMP");

    char d = mpu9250.readByte(AK8963_ADDRESS, WHO_AM_I_AK8963);
    if(d == 0x48) {
      mpu9250.initAK8963(mpu9250.magCalibration);
      Serial.println("BOOT Reading Mag Cal...");
      Serial.print("BOOT\t");
      for(int i = 0; i < 3; i++) {
        Serial.print(mpu9250.magCalibration[i], 2);
        Serial.print(" ");
      }
      Serial.println();
    } else Serial.println("BOOT Cannot find AK8963 magnetometer.");

  } else {
    Serial.print("BOOT Cannot find MPU9250 @ 0x73. Instead found: ");
    Serial.println(c, HEX);
  }

#endif
}


void setupPeripherals() {
  setupIMU();
  ms5803.begin();

  pinMode(PIN_ADC_KILL, INPUT);
}



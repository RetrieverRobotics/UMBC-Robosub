
bool IMU_SETUP_GOOD = false;
void setupIMU() {
  Serial.println("BOOT Initializing IMU...");
  unsigned long init_begin = millis();
  unsigned int timeout = 5000;
  int cnt = 1;
  while(millis() - init_begin < timeout) {
    if(bno.begin(Adafruit_BNO055::OPERATION_MODE_NDOF)) {
      IMU_SETUP_GOOD = true;
      break;
    }
    String msg = "BOOT\tAttempt " + String(cnt) + " failed.";
    Serial.print(msg);
    ++cnt;
    delay(500);
  }
  if(IMU_SETUP_GOOD) {
    Serial.println(F("BOOT\tIMU found."));
  } else {
    Serial.println(F("BOOT\tIMU initialization failed. Continuing... even though it's pointless."));
  }

  bno.setExtCrystalUse(true);
}

bool TFT_SETUP_GOOD = true; // can't really verify this...
void setupTFT() {
  Serial.println(F("BOOT Initializing TFT display..."));
  tft.begin();
  tft.fillScreen(ILI9341_WHITE);
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(2);

  tft.setRotation(2);

  Serial.println(F("BOOT\tTFT setup complete (assuming it's attached)"));
}


void setupPeripherals() {
  setupIMU();
  setupTFT();
  depth_sensor.begin();
}



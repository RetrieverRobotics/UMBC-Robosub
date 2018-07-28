#include "MS5803.h"

const uint8_t MS5803::OSR_flags[] = {
  0x00, 0x02, 0x04, 0x06, 0x08
};
const uint8_t MS5803::OSR_delays[] = {
  1, 2, 3, 5, 10
};

MS5803::MS5803(uint16_t _addr, uint8_t _model = 5) {
  // address is either 0x76 or 0x77, depending on CSB pin state
  addr = _addr;
  
  // 1, 2, 5, 14
  // for first order, powers of 2 vary
  // for second order, the equations used vary slightly
  // model = _model;
  // if(!(model == 1 || model == 2 || model == 5 || model == 14)) model = 5;
  model = 5;
}

void MS5803::begin() {
  Serial.println("BOOT Initializing MS5803 pressure sensor...");

  Wire.begin();

  Wire.beginTransmission(addr);
  Wire.write(MS5803_RESET);
  Wire.endTransmission();
  delay(5); // make this too short and some of the coefficients aren't read correctly

  Serial.print(F("BOOT\tcoefficients: "));
  for(int i = 0; i < 6; i++) {
    Wire.beginTransmission(addr);
    Wire.write(MS5803_COEF_READ + (2*i));
    Wire.endTransmission();

    Wire.requestFrom(addr, 2);
    if(Wire.available() == 2) { for(uint8_t i = 0; i < 2; i++) data[i] = Wire.read(); }

    coeffs[i] = (data[0] * 256) + data[1];
    Serial.print(coeffs[i]);
    Serial.print(" ");
  }
  Serial.println();
}

// one is inclined to take the simplicity of this for granted
// that is, if they're not writing the library... XD
void MS5803::update(uint8_t osr) {
  // credit to Luke Miller for the neurotic casting to prevent rollover
  
  // begin uncompensated pressure conversion
  Wire.beginTransmission(addr);
  Wire.write(MS5803_D1 | OSR_flags[osr]);
  Wire.endTransmission();
  // wait for conversion to complete
  delay(OSR_delays[osr]);
  // call ADC read
  Wire.beginTransmission(addr);
  Wire.write(MS5803_ADC_READ);
  Wire.endTransmission();
  // read 24 bits
  Wire.requestFrom(addr, 3);
  if(Wire.available() == 3) { for(uint8_t i = 0; i < 3; i++) data[i] = Wire.read(); }
  // pack bits into 32-bit int
  D1 = (data[0] * 65536) + (data[1] * 256) + data[2];

  // begin uncompensated temperature conversion
  Wire.beginTransmission(addr);
  Wire.write(MS5803_D2 | OSR_flags[osr]);
  Wire.endTransmission();
  delay(OSR_delays[osr]);
  // call ADC read
  Wire.beginTransmission(addr);
  Wire.write(MS5803_ADC_READ);
  Wire.endTransmission();
  // read 24 bits
  Wire.requestFrom(addr, 3);
  if(Wire.available() == 3) { for(uint8_t i = 0; i < 3; i++) data[i] = Wire.read(); }
  // pack bits into 32-bit int
  D2 = (data[0] * 65536) + (data[1] * 256) + data[2];

  // calculate temperature
  dT = (int32_t)D2 - ((int32_t)coeffs[MS5803_COEF_TREF] * 256); // 2^8
  TEMP = (int32_t) ( 2000 + ((int64_t)dT * coeffs[MS5803_COEF_TEMPSENS]) / 8388608 ); // 2^23

  switch(model) {
    case 1:
      break;
    case 2:
      break;
    case 5:
      // calculate a few compensation values
      OFF = ((int64_t)coeffs[MS5803_COEF_OFF] * 262144) + ( (coeffs[MS5803_COEF_TCO] * (int64_t)dT) / 32); // 2^18, 2^5
      SENS = ((int64_t)coeffs[MS5803_COEF_SENS] * 131072) + ( (coeffs[MS5803_COEF_TCS] * (int64_t)dT) / 128); // 2^17, 2^7
    
      // 2ND ORDER TEMP COMPENSATION, adjust before calculating compensated pressure
      if(TEMP < 2000) { // low temp
        int64_t tmp = 0;
        TEMP2 = (int32_t)( 3 * ((int64_t)dT * dT) / 8589934592 ); // 2^33
        tmp = pow( TEMP - 2000, 2 );
        OFF2 = 3 * tmp / 16;
        SENS2 = 7 * tmp;
    
        if(TEMP < -1500) { // very low temp
          tmp = pow( TEMP + 1500, 2 );
          SENS2 += 3 * tmp;
        }
      } else {
        TEMP2 = OFF2 = SENS2 = 0;
      }
    
      TEMP -= TEMP2;
      OFF -= OFF2;
      SENS -= SENS2;
      
      break;
    case 14:
      break;
  }

  // calculate temperature-compensated pressure
  P = ( (D1 * SENS / 2097152) - OFF ) / 32768; // 2^21, 2^15
}

float MS5803::getPressure() {
  return (float)P / 100.0; // millibars, 0.01 resolution
}
int32_t MS5803::getPressureI() {
  return P;
}

float MS5803::getTemperature() {
  return (float)TEMP / 100.0; // degrees C, 0.01 resolution
}
int32_t MS5803::getTemperatureI() {
  return TEMP;
}



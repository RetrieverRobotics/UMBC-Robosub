// http://www.mouser.com/ds/2/418/MS5803-02BA-736541.pdf
// https://www.mouser.com/ds/2/418/NG_DS_MS5803-05BA_B-1130145.pdf
// https://thecavepearlproject.org/2014/03/27/adding-a-ms5803-02-high-resolution-pressure-sensor/
// http://lukemiller.org/index.php/2014/04/arduino-code-for-ms5803-pressure-sensors/

#include <Arduino.h>
#include <Wire.h>

// PROM read, addresses 0 through E
// first two bytes reserved for manufacturer
// bytes middle 12 bytes read as 16-bit pairs, compensation coefficients written by factory
// last two bytes are CRC
#define MS5803_PROM_READ (0xA0)
#define MS5803_COEF_READ (MS5803_PROM_READ+2)

// coefficients array aliases
#define MS5803_COEF_SENS     (0)
#define MS5803_COEF_OFF      (1)
#define MS5803_COEF_TCS      (2)
#define MS5803_COEF_TCO      (3)
#define MS5803_COEF_TREF     (4)
#define MS5803_COEF_TEMPSENS (5)

// oversampling setup data indexes (note the array below)
// allows OSR to be changed during runtime
// also notes max conversion times for a given OSR
#define MS5803_OSR_256  (0) // 1 ms
#define MS5803_OSR_512  (1) // 2 ms
#define MS5803_OSR_1024 (2) // 3 ms
#define MS5803_OSR_2048 (3) // 5 ms
#define MS5803_OSR_4096 (4) // 10 ms

// reset ADC
#define MS5803_RESET (0x1E)

// data conversion commands
#define MS5803_D1 (0x40)
#define MS5803_D2 (0x50)

#define MS5803_ADC_READ (0x00)

class MS5803 {
public:
  MS5803(uint16_t, uint8_t);

  void begin();
  void update(uint8_t);
  float getPressure();
  int32_t getPressureI();
  float getTemperature();
  int32_t getTemperatureI();
  
private:
  const static uint8_t OSR_flags[];
  const static uint8_t OSR_delays[];

  uint8_t model;
  uint16_t addr;

  // millibar
  // const uint16_t P_min = 0, P_max = 6000;

  // degree C
  // const int16_t Temp_min = -40, Temp_max = 85, Temp_ref = 20;

  uint32_t D1 = 0, D2 = 0;

  int32_t dT = 0, TEMP = 0;

  int64_t OFF = 0, SENS = 0;
  int32_t P = 0;

  // for 2nd order compensation
  int32_t TEMP2 = 0, OFF2 = 0, SENS2 = 0;

  uint16_t coeffs[6];
  uint8_t data[3];
};

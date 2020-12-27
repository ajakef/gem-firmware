//*************
// File written by Adafruit and modified by JFA
// Function definitions are in logger.cpp
//*************
/**************************************************************************/
/*!

@file Adafruit_ADS1015.h
@author K. Townsend (Adafruit Industries)
@license BSD (see license.txt)

This is a library for the Adafruit ADS1015 breakout board
----> https://www.adafruit.com/products/???

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

@section HISTORY

v1.0 - First release
v1.1 - Added ADS1115 support - W. Earl
*/
/**************************************************************************/
#ifndef ADS1015_H
#define ADS1015_H

#include "Arduino.h"
#include <Wire.h>

//I2C ADDRESS/BITS
#define ADS1015_ADDRESS (0x48) // 1001 000 (ADDR = GND)

//CONVERSION DELAY (in mS)
#define ADS1015_CONVERSIONDELAY (1)
#define ADS1115_CONVERSIONDELAY (8)

//POINTER REGISTER
#define ADS1015_REG_POINTER_CONVERT (0x00)
#define ADS1015_REG_POINTER_CONFIG (0x01)

//CONFIG REGISTER
#define ADS1015_REG_CONFIG_OS_SINGLE (0x8000) // Write: Set to start a single-conversion
#define ADS1015_REG_CONFIG_MUX_DIFF_0_1 (0x0000) // Differential P = AIN0, N = AIN1 (default)

#define ADS1015_REG_CONFIG_PGA_6_144V (0x0000) // +/-6.144V range = Gain 2/3
#define ADS1015_REG_CONFIG_PGA_4_096V (0x0200) // +/-4.096V range = Gain 1
#define ADS1015_REG_CONFIG_PGA_2_048V (0x0400) // +/-2.048V range = Gain 2 (default)
#define ADS1015_REG_CONFIG_PGA_1_024V (0x0600) // +/-1.024V range = Gain 4
#define ADS1015_REG_CONFIG_PGA_0_512V (0x0800) // +/-0.512V range = Gain 8
#define ADS1015_REG_CONFIG_PGA_0_256V (0x0A00) // +/-0.256V range = Gain 16
#define ADS1015_REG_CONFIG_MODE_SINGLE (0x0100) // Power-down single-shot mode (default)

// ADS1115 Data Rates--JFA 2015/6/20
#define ADS1115_REG_CONFIG_DR_8SPS (0x0000) // 8 sps for ADS1115
#define ADS1115_REG_CONFIG_DR_16SPS (0x0020) // 16 sps for ADS1115
#define ADS1115_REG_CONFIG_DR_32SPS (0x0040) // 32 sps for ADS1115
#define ADS1115_REG_CONFIG_DR_64SPS (0x0060) // 64 sps for ADS1115
#define ADS1115_REG_CONFIG_DR_128SPS (0x0080) // 128 sps for ADS1115
#define ADS1115_REG_CONFIG_DR_250SPS (0x00A0) // 250 sps for ADS1115
#define ADS1115_REG_CONFIG_DR_475SPS (0x00C0) // 475 sps for ADS1115
#define ADS1115_REG_CONFIG_DR_860SPS (0x00E0) // 860 sps for ADS1115
///////////

#define ADS1015_REG_CONFIG_CMODE_TRAD (0x0000) // Traditional comparator with hysteresis (default)
#define ADS1015_REG_CONFIG_CPOL_ACTVLOW (0x0000) // ALERT/RDY pin is low when active (default)
#define ADS1015_REG_CONFIG_CLAT_NONLAT (0x0000) // Non-latching comparator (default)
#define ADS1015_REG_CONFIG_CQUE_NONE (0x0003) // Disable the comparator and put ALERT/RDY in high state (default)

/*=========================================================================*/

typedef enum
{
  GAIN_TWOTHIRDS = ADS1015_REG_CONFIG_PGA_6_144V,
  GAIN_ONE = ADS1015_REG_CONFIG_PGA_4_096V,
  GAIN_TWO = ADS1015_REG_CONFIG_PGA_2_048V,
  GAIN_FOUR = ADS1015_REG_CONFIG_PGA_1_024V,
  GAIN_EIGHT = ADS1015_REG_CONFIG_PGA_0_512V,
  GAIN_SIXTEEN = ADS1015_REG_CONFIG_PGA_0_256V
} adsGain_t;

class Adafruit_ADS1015
{
protected:
   // Instance-specific properties
   uint8_t m_i2cAddress;
   uint8_t m_conversionDelay;
   uint8_t m_bitShift;
   adsGain_t m_gain;

 public:
  Adafruit_ADS1015(uint8_t i2cAddress = ADS1015_ADDRESS);
  void begin(void);
  void request_Differential_0_1();
  int16_t read_ADC(int8_t *error);
  void setGain(adsGain_t gain);

 private:
};

// Derive from ADS1105 & override construction to set properties
class Adafruit_ADS1115 : public Adafruit_ADS1015
{
 public:
  Adafruit_ADS1115(uint8_t i2cAddress = ADS1015_ADDRESS);

 private:
};
#endif

// Modified from the original Adafruit version--Jake Anderson, 2017
/**************************************************************************/
/*!
@file Adafruit_ADS1015.cpp
@author K.Townsend (Adafruit Industries)
@license BSD (see license.txt)

Driver for the ADS1015/ADS1115 ADC

This is a library for the Adafruit MPL115A2 breakout
----> https://www.adafruit.com/products/???

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

@section HISTORY

v1.0 - First release
*/
/**************************************************************************/
#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <Wire.h>
#include "Adafruit_ADS1015.h"

/**************************************************************************/
/*!
@brief Abstract away platform differences in Arduino wire library
*/
/**************************************************************************/
static uint8_t i2cread(void) {
#if ARDUINO >= 100
  return Wire.read();
#else
  return Wire.receive();
#endif
}
/**************************************************************************/
/*!
@brief Abstract away platform differences in Arduino wire library
*/
/**************************************************************************/
static void i2cwrite(uint8_t x) {
#if ARDUINO >= 100
  Wire.write((uint8_t)x);
#else
  Wire.send(x);
#endif
}

/**************************************************************************/
/*!
@brief Instantiates a new ADS1015 class w/appropriate properties
*/
/**************************************************************************/

Adafruit_ADS1015::Adafruit_ADS1015(uint8_t i2cAddress)
{
   m_i2cAddress = i2cAddress;
   m_conversionDelay = ADS1015_CONVERSIONDELAY;
   m_bitShift = 4;
   m_gain = GAIN_TWOTHIRDS; /* +/- 6.144V range (limited to VDD +0.3V max!) */
}

/**************************************************************************/
/*!
@brief Instantiates a new ADS1115 class w/appropriate properties
*/
/**************************************************************************/
Adafruit_ADS1115::Adafruit_ADS1115(uint8_t i2cAddress){
   m_i2cAddress = i2cAddress;
   m_conversionDelay = ADS1115_CONVERSIONDELAY;
   m_bitShift = 0;
   m_gain = GAIN_TWOTHIRDS; /* +/- 6.144V range (limited to VDD +0.3V max!) */
}

//////////////////
void Adafruit_ADS1015::begin() {
  Wire.begin();
  Wire.setClock(400000L);
}
//////////////////////
void Adafruit_ADS1015::setGain(adsGain_t gain){
  m_gain = gain;
}
//////////////////////////
void Adafruit_ADS1015::request_Differential_0_1_JFA(){
    // Start with default values
  uint16_t config = ADS1015_REG_CONFIG_CQUE_NONE | // Disable the comparator (default val)
                    ADS1015_REG_CONFIG_CLAT_NONLAT | // Non-latching (default val)
                    ADS1015_REG_CONFIG_CPOL_ACTVLOW | // Alert/Rdy active low (default val)
                    ADS1015_REG_CONFIG_CMODE_TRAD | // Traditional comparator (default val)
                    ADS1115_REG_CONFIG_DR_475SPS | // 475 sps is the best sample rate setting when true sample rate is actually 400 sps
                    ADS1015_REG_CONFIG_MODE_SINGLE | // Single-shot mode (default)
                    m_gain | // Set PGA/voltage range
                    ADS1015_REG_CONFIG_MUX_DIFF_0_1 | // Set channels: AIN0 = P, AIN1 = N
                    ADS1015_REG_CONFIG_OS_SINGLE;  // Set 'start single-conversion' bit
  // Write config register to the ADC
  Wire.beginTransmission(m_i2cAddress);
  Wire.write((uint8_t)ADS1015_REG_POINTER_CONFIG);
  Wire.write((uint8_t)(config>>8));
  Wire.write((uint8_t)(config & 0xFF));
  Wire.endTransmission();
}

int16_t Adafruit_ADS1015::read_Differential_0_1_JFA(int8_t *error){
  Wire.beginTransmission(m_i2cAddress);
  Wire.write(ADS1015_REG_POINTER_CONVERT);
  Wire.endTransmission();
  *error += (2 != Wire.requestFrom(m_i2cAddress, (uint8_t)2));
  //int16_t res = ((Wire.read() << 8) | Wire.read());
  return (int16_t)((Wire.read() << 8) | Wire.read());
}

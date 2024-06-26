// use LOGGER_H to ensure that definitions are not run twice. This must be the first line run in logger.h.
#ifndef LOGGER_H
#define LOGGER_H

// Main User-Configurable Options
#define DT 2500 // microseconds
#define META_CYC 100 // interval (in samples) between metadata reports
#define FILE_LENGTH_DEFAULT 240 // minutes--must be multiple of 10 minutes
#define LOW_BATT_THRESHOLD_STOP_MV 2800
#define LOW_BATT_THRESHOLD_GO_MV 3000

// GPS_TESTING block to facilitate GPS tests (don't have to wait 15 minutes for results)
#define GPS_TESTING 0
#if GPS_TESTING
  #define GPS_QUOTA_DEFAULT 10 // number of good GPS samples that must be logged before GPS is put on standby
  #define GPS_CYC 1500 // interval in samples between GPS turning on and logging until quota is reached) 15 minutes
  #define GPS_CYC_DEFAULT 1 // minutes, for the config
  #define GPS_RESET_THRESHOLD 20 // reset the GPS if it records 20 PPS pulses without a good NMEA string
  #define LONG_GPS_CYC_LENGTH 10 // GPS fixes in 15 minutes. The almanac takes 12.5 minutes to load, so this should be adequate.
#else
  #define GPS_QUOTA_DEFAULT 20 // number of good GPS samples that must be logged before GPS is put on standby
  #define GPS_CYC 90000 // interval in samples between GPS turning on and logging until quota is reached) 15 minutes
  #define GPS_CYC_DEFAULT 15 // minutes, for the config
  #define GPS_RESET_THRESHOLD 20 // reset the GPS if it records 20 PPS pulses without a good NMEA string
  #define LONG_GPS_CYC_LENGTH 1800 // GPS fixes in 15 minutes. The almanac takes 12.5 minutes to load, so this should be adequate.
#endif

#define BUFFERLENGTH 80 // GPS buffer length (characters, bytes)
#define ADC_ERROR_THRESHOLD 255 // raise error 3 if the ADC fails this many times in a row
#define MILLIS_ROLLOVER 8192 // the milliseconds written to the data file roll over this often (reduces file size)

/*FIFO is the buffer where samples go between the high-priority digitization/filter loop and the low-priority SD write.
Each sample takes up 4 bytes. Normally, the FIFO only contains a few samples, but it occasionally uses many more. If it
fills, it triggers an overrun, and subsequent samples are lost until the FIFO has space again. So, the FIFO must be long 
enough to make sure it doesn't fill, but short enough to fit within available SRAM. If program crashes mysteriously after
adding new code, it could be a memory error: try decreasing FIFO_SIZE_BYTES.*/
#define FIFO_SIZE_BYTES 300  
#define FIFO_DIM FIFO_SIZE_BYTES/4

// Serial data rate (for communicating with GPS): use 57600
// this is set up to easily switch to 115200, but the Gem doesn't receive any good serial data from the GPS at 115200
#define FAST_BAUD_RATE 57600 
#if FAST_BAUD_RATE == 57600
  #define PMTK_SET_BAUD_FAST PMTK_SET_BAUD_57600
#else
  #define PMTK_SET_BAUD_FAST PMTK_SET_BAUD_115200
#endif

// include dependencies
#include <Arduino.h>
#include "src/SdFat/SdFat.h"
#include "src/NilRTOS/NilFIFO.h"
#include "src/ADS1115/ADS1115.h"
#include <avr/wdt.h>
#include "version.h"
#include <EEPROM.h>

// Digital Pinout
// D0, D1: Serial port for communicating with GPS and computer
#define PPS 2 // pulse-per-second from GPS (D2 supports interrupts)
// D3: unused
#define LED 4 // ACQ LED, blue
#define ERRORLED 5 // ERR LED, red
#define SWITCH 6 // Acquisition start/stop switch
// D7, D8, D9: unused
// D10-D13: SD card interface (SPI)

// Analog Pinout
#define TEMP 0 // thermometer
#define BATVOLT 1 // battery voltage
// A2, A3: user can connect to miscellaneous analog sensors
// A4, A5: Analog-Digital Converter interface (I2C)

// GPS Commands (from Adafruit GPS library)
#define PMTK_COLD_START "$PMTK103*30" // do not use existing time/location/almanac/ephemeris data. Consider using this on first power-up.
#define PMTK_SET_NMEA_UPDATE_1HZ  "$PMTK220,1000*1F"
#define PMTK_SET_BAUD_115200 "$PMTK251,115200*1F" ///< 115200 bps
#define PMTK_SET_BAUD_57600 "$PMTK251,57600*2C"
#define PMTK_SET_BAUD_9600 "$PMTK251,9600*17"
#define PMTK_SET_NMEA_OUTPUT_RMCONLY "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29"// turn on only the second sentence (GPRMC)
#define PMTK_SET_NMEA_OUTPUT_RMCGGA "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"// turn on GPRMC and GGA
#define PMTK_SET_NMEA_OUTPUT_ALLDATA "$PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28"// turn on ALL THE DATA
#define PMTK_SET_NMEA_OUTPUT_OFF "$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"// turn off output
#define PMTK_STANDBY "$PMTK161,0*28"// standby command & boot successful message
#define PMTK_AWAKE "$PMTK010,002*2D"
#define PMTK_FACTORY_RESET "$PMTK104*37"

// Coefficients for FIR filter: sum to 2^15, so divide by 2^15 at the end (first confining to +/-2^15).
#define C0 21L
#define C1 -21L
#define C2 -104L
#define C3 -241L
#define C4 -383L
#define C5 -406L
#define C6 -146L
#define C7 524L
#define C8 1617L
#define C9 2986L
#define CA 4341L
#define CB 5341L
#define CC 5709L

// Type for RMC info
struct RMC {
  int8_t hour;
  int8_t minute;
  float second;
  float lat;
  float lon;
  uint8_t year;
  uint8_t month;
  uint8_t day;
  uint16_t millisParsed;
};

// Type for configuration info
struct GemConfig {
  uint8_t gps_mode; // 1 cycled, 2 on, 3 off
  uint8_t gps_cycle; // interval (minutes)
  uint8_t gps_quota; // number of strings
  uint8_t adc_range; // 0 for +/-0.256 V (e.g., infrasound--default), 1 for +/-0.512 V
  uint8_t led_shutoff; // 0 (never turn off), 1-255 (number of minutes to leave on)
  uint8_t serial_output; // 0 to send all data to SD only; 1 to send all data to SD and pressure data only to Serial too
};

// Type for a data record.
struct Record_t {
  int16_t pressure;
  uint16_t time;
};

// Function declarations:
void printdata(Record_t* p, SdFile* file, volatile float* pps_millis, GemConfig *config, int16_t *last_pressure, int16_t *last_time, uint16_t *sample_count_stream);
void printmeta(SdFile* file, NilStatsFIFO<Record_t, FIFO_DIM>* fifo, uint16_t* maxWriteTime, uint8_t* GPS_flag, float* AVCC);
void printRMC(RMC* G, SdFile* file, volatile float* pps_millis, uint8_t* long_gps_cyc);
void FindFirstFile(char fname[13], SdFat* sd, SdFile* file, int16_t* SN);
void IncrementFilename(char fname[13]);
void logstatus(int8_t logging[2], float *AVCC);
void OpenNewFile(SdFat* sd, char filename[13], SdFile* file, GemConfig* config, int16_t* last_pressure, int16_t* last_time);
void BlinkLED(uint32_t* sample_count, uint8_t* GPS_on, uint16_t* GPS_count);
void EndFile(SdFile* file);
void EndLogging(uint16_t* maxLatency, NilStatsFIFO<Record_t, FIFO_DIM>* fifo, uint32_t* sample_count);
void GPS_startup(GemConfig* config);
void error(int8_t code);
void begin_WDT(void);
uint8_t ParseRMC(char *nmea, RMC *G);
int16_t SincFilt(int16_t buf1[4], int16_t buf2[4], int16_t buf3[4], int16_t buf4[4], int16_t buf5[4], int16_t buf6[4], int16_t* reading);
uint8_t parseHex(char c);
void ReadConfig(SdFile *file, char *buffer, uint8_t *buffidx, GemConfig *config);
int32_t ReadConfigLine(SdFile *file, char *buffer, uint8_t *buffidx);
float set_AVCC(uint16_t* SN);
void GPS_standby();
void GPS_awake();

// This approximate "millisecond" function is used for compatibility with the micros() used for the pps; they roll over at the same time.
//#define gem_millis() round(micros()/1024.0) 
uint16_t gem_millis(); //saves a lot of flash to implement this as a function instead of as a #define
void file_print_lf(SdFile* file);

// miscellaneous GPS checks, which may be redefined as methods of RMC class in the future
bool checkRMC_fresh(volatile float *pps_millis);
bool checkRMC_yearmissing(RMC* G);
bool checkRMC_yearwrong(RMC* G);
bool checkRMC_latlon(RMC* G);
bool checkRMC_secfloat(RMC* G);
bool checkRMC_badtime(RMC* G);

#endif // ifndef LOGGER_H; this must be after all the non-commented lines in logger.h

// Miscellaneous GPS instructions that are not used here but could potentially be implemented by a user (thanks, Adafruit!)

// to generate your own sentences, check out the MTK command datasheet and use a checksum calculator
// such as the awesome http://www.hhhh.org/wiml/proj/nmeaxor.html
//#define PMTK_SET_NMEA_UPDATE_5HZ  "$PMTK220,200*2C"
//#define PMTK_SET_NMEA_UPDATE_10HZ "$PMTK220,100*2F"
//#define PMTK_API_SET_FIX_CTL_1HZ  "$PMTK300,1000,0,0,0,0*1C"// Position fix update rate commands.
//#define PMTK_API_SET_FIX_CTL_5HZ  "$PMTK300,200,0,0,0,0*2F"// Can't fix position faster than 5 times a second!
//#define PMTK_LOCUS_STARTLOG  "$PMTK185,0*22"
//#define PMTK_LOCUS_STOPLOG "$PMTK185,1*23"
//#define PMTK_LOCUS_STARTSTOPACK "$PMTK001,185,3*3C"
//#define PMTK_LOCUS_QUERY_STATUS "$PMTK183*38"
//#define PMTK_LOCUS_ERASE_FLASH "$PMTK184,1*22"
//#define LOCUS_OVERLAP 0
//#define LOCUS_FULLSTOP 1
//#define PMTK_STANDBY_SUCCESS "$PMTK001,161,3*36"  // Not needed currently
//#define PMTK_Q_RELEASE "$PMTK605*31"// ask for the release and version
//#define PGCMD_ANTENNA "$PGCMD,33,1*6C" // request for updates on antenna status
//#define PGCMD_NOANTENNA "$PGCMD,33,0*6D"
//#define PMTK_ENABLE_SBAS "$PMTK313,1*2E" // "satellite-based augmentation system". Systems are available for several regions/continents
//#define PMTK_ENABLE_WAAS "$PMTK301,2*2E"

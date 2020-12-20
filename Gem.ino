/* Pinout:
0,1: Serial
2: PPS interrupt
3: Open
4: ACQ LED (blue)
5: Error LED (red)
6: Logging status switch
7, 8, 9: Open 
10-13: SD
A0: Temperature
A1: Battery Voltage
*/
#include "version.h"
#include <Wire.h>
#include <SdFat.h> // 512-byte buffer
#include <NilTimer1.h> 
#include "Adafruit_ADS1015.h"
#include <EEPROM.h>
#include "logger.h"

// Declare FIFO with overrun and minimum free space statistics.
NilStatsFIFO<Record_t, FIFO_DIM> fifo; 

GemConfig config; // configuration info
Adafruit_ADS1115 adc; // should be small--under 10 bytes
char buffer[BUFFERLENGTH]; // buffer for reading GPS strings
char c;
uint8_t buffidx = 0;
uint8_t GPS_flag = 0; // indicates that a GPS string is ready to write
uint8_t GPS_on = 0; // indicates whether the GPS is on, or on standby
uint8_t firstfile;
SdFat sd; // SD file system
SdFile file; // Log file
char filename[13] = "CONFIG.TXT"; 
RMC G; // structure to hold GPS string info

volatile uint8_t pps = 0;
volatile uint16_t pps_millis = 0;
uint8_t pps_count = 0;
uint8_t pps_print = 0;
int8_t logging[2] = {0, 0}; // if necessary, could consolidate 'sampling' and 'firstfile' into logging by using codes for logging[0] instead of just 0/1
int8_t sampling = 0;
uint32_t sample_count = 0;  // distinct from samp_index--used in data writing 
uint8_t GPS_count = 0; // number of pps pulses in a GPS-on period, so that we know when to turn off the GPS // 2015/4/1 JFA
int8_t cons_overruns = 0; // count of consecutive overruns for determining an error
uint8_t AdcError = 0;
uint16_t maxWriteTime = 0;
uint16_t writeStartTime;
int16_t SN = 0;
float AVCC = 3.1; // version-dependent; see setup()

uint8_t FIR_count;
int16_t FIR_buffer_1[4] = {0, 0, 0, 0};
int16_t FIR_buffer_2[4] = {0, 0, 0, 0};
int16_t FIR_buffer_3[4] = {0, 0, 0, 0};
int16_t FIR_buffer_4[4] = {0, 0, 0, 0};
int16_t FIR_buffer_5[4] = {0, 0, 0, 0};
int16_t FIR_buffer_6[4] = {0, 0, 0, 0};
int16_t ADC_F;
uint32_t mmm=0;
uint16_t tsamp[4] = {0, 0, 0, 0};
int16_t reading;
int16_t last_sample = 0;
////////////////////////////////////////////////////

// Highest priority thread: read ADC and send to FIFO
// Declare a stack with 16 bytes beyond context switch and interrupt needs.
// The highest priority thread requires less stack than other threads.
NIL_WORKING_AREA(waThread1, 16);
NIL_THREAD(Thread1, arg) { // Declare thread function for thread 1.
  while(1){
    while(!sampling){
      nilThdSleepMilliseconds(100); // ms
    }
    // Start timer 1 with a period of DT.
    nilTimer1Start(DT);
    tsamp[0] = millis()-30; tsamp[1] = tsamp[0]+10; tsamp[2] = tsamp[1]+10; tsamp[3] = tsamp[2]+10;
    FIR_count = 0;

    while(sampling){
      // Sleep until it's time for next data point.
      if (!nilTimer1Wait()) {
        digitalWrite(ERRORLED, HIGH);
        fifo.countOverrun();// Took too long so count an overrun.
        continue;
      }
      // increment the count and read a sample
      FIR_count++;
      if(FIR_count > 3){ // changed from FIR_count==4 to try to fix ERR_A--JFA 2016-03-07
        FIR_count = 0;
      }
      reading = adc.read_Differential_0_1_JFA(&AdcError); // read the current sample; increment AdcError if problem
      adc.request_Differential_0_1_JFA(); // request the next sample; will be read next iteration
      
      mmm = micros();
      // run the filter if this is a decimation sample.  
      if(FIR_count == 0){
        ADC_F = SincFilt(FIR_buffer_1, FIR_buffer_2, FIR_buffer_3, FIR_buffer_4, 
                        FIR_buffer_5, FIR_buffer_6, &reading); // filter has to be done before updating buffers
      }

      mmm = micros() - mmm;
      // shift the data between buffers and add the new reading
      FIR_buffer_1[FIR_count] = FIR_buffer_2[FIR_count];
      FIR_buffer_2[FIR_count] = FIR_buffer_3[FIR_count];
      FIR_buffer_3[FIR_count] = FIR_buffer_4[FIR_count];
      FIR_buffer_4[FIR_count] = FIR_buffer_5[FIR_count];
      FIR_buffer_5[FIR_count] = FIR_buffer_6[FIR_count];
      FIR_buffer_6[FIR_count] = reading;
      
      if(FIR_count != 0) continue;
      
      // if we're at this point, it's a decimation round (FIR_count == 0).
      Record_t* p = fifo.waitFree(TIME_IMMEDIATE); // Get a free fifo buffer.
  
      // Skip the point if no buffer is available, fifo will count overrun.
      if(!p){
        cons_overruns++;
        if(cons_overruns >= 100){
          // Overrun error: could be interpreted as SD card writer failing to keep up with incoming data.
          // This can happen when the card writer gets wet.
          error(4); 
        }
        continue;
      }
      cons_overruns = 0;
      
      p->time = tsamp[0]; 
      p->pressure = ADC_F; // sinc-filtered ADC centered on 0.01 s ago
      fifo.signalData(); // Signal SD write thread that new data is ready.
      tsamp[0] = tsamp[1];
      tsamp[1] = tsamp[2];
      tsamp[2] = tsamp[3]; 
      tsamp[3] = millis(); // tsamp must be calculated here so that it is matched with the sample centered here    

      // check for ADC errors--this count is reset at the start of each file
      if(AdcError >= ADC_ERROR_THRESHOLD){
        error(3); // error reading ADC--check power, signal connections; could be broken. 
      }
      
      GPS_flag = GPS_flag || pps;
      pps_print = pps_print || pps;
      pps = 0;
    }
    nilTimer1Stop(); // if we're here, logging is off so stop the timer.
    // Done, go to sleep.
  }
}
//------------------------------------------------------------------------------
// List the threads in this table between NIL_THREADS_TABLE_BEGIN and NIL_THREADS_TABLE_END.
// When multiple threads are defined, list them in order of priority.
NIL_THREADS_TABLE_BEGIN()
NIL_THREADS_TABLE_ENTRY(NULL, Thread1, NULL, waThread1, sizeof(waThread1))
NIL_THREADS_TABLE_END()
//------------------------------------------------------------------------------
void setup() {
  // read serial number
  SN += (EEPROM.read(0) - '0')*100;
  SN += (EEPROM.read(1) - '0')*10;
  SN += (EEPROM.read(2) - '0')*1;
  
  // If serial number is known and less than 38 (first Gem 0.9), use DEFAULT (unsafe unless known to be v<0.9).
  // Otherwise, use EXTERNAL (always safe, but analog readings don't work for v<0.9)
  if(SN > 0 && SN < 38){
    analogReference(DEFAULT);
    AVCC = 3.35;
  }else{
    analogReference(EXTERNAL);
    AVCC = 3.1;
  }
  
  delay(100);   
  // set up IO pins: PPS, LEDs, switch, SD, and analog
  pinMode(PPS, INPUT);
  attachInterrupt(0, PPS_INT, RISING); // set up PPS pin interrupt (log PPS when PPS pin is pulled high)
  pinMode(LED, OUTPUT);
  pinMode(ERRORLED, OUTPUT);
  pinMode(SWITCH, INPUT);
  pinMode(SS, OUTPUT); // SS is chipselect--10
  
  // In case user has computer connected by Serial, give instructions on how to set serial number
  Serial.begin(57600);
  Serial.println();
  Serial.print(F("Firmware ")); Serial.println(F(FIRMWARE_VERSION));
  Serial.println();
  Serial.print(F("Gem Serial Number: "));
  Serial.println(SN);

  Serial.println(F("To program new serial number 'XXX', enter 'SN:XXX'"));
  Serial.println(F("For Lab Mode (streaming ascii data over Serial; no GPS), enter 'lab'"));
  Serial.println(F("For Binary Mode (streaming binary data over Serial; no GPS), enter 'bin'"));
  delay(200);
  // Configure the GPS
  GPS_startup(&config); 
  
  // start external ADC (ADS1115)
  adc.begin();
  // Start Real-Time Operating System
  nilSysBegin(); // this line must be in setup
}
//------------------------------------------------------------------------------
// Write data to the SD in loop() (the "idle thread").
void loop() {
  // each iteration of loop represents one switch-on/switch-off cycle  
  buffidx = 0;
  GPS_flag = 0;
  GPS_on = 0;
  logging[0] = 0; logging[1] = 0;
  sample_count = 0;
  GPS_count = 0;
  pps_count = 0;
  pps_print = 0;
  pps = 0;
  cons_overruns= 0;
  AdcError = 0;
  maxWriteTime = 0;
  // wait for ACQ switch to be turned on. 
  while(!logging[0]){ // wait here until switch is turned on
    logstatus(logging);

    // Meanwhile, check to see if user has supplied new serial number via Serial connection.
    if(Serial.available() > 2){
      for(int i = 0; i < 3; i++){
        buffer[i] = Serial.read();
      }
//      if(Serial.read() == 'S' && Serial.read() == 'N' && Serial.read() == ':'){ // If "SN:" prefix is detected, write the new serial number to EEPROM.
      if(buffer[0] == 'S' && buffer[1] == 'N' && buffer[2] == ':'){ // If "SN:" prefix is detected, write the new serial number to EEPROM.
        SN = 0;
        for(int i = 0; i < 3; i++){
          while(!Serial.available()){}
          EEPROM.write(i, (char) Serial.read());
          SN += round(pow(10, 2-i)) * (EEPROM.read(i)-'0');
        }
        Serial.print(F("New Serial Number: "));
        Serial.println(SN);
      }else if(buffer[0] == 'l' && buffer[1] == 'a' && buffer[2] == 'b'){ 
        // check to see if user has requested a lab test (output to serial in addition to sd)
        config.serial_output = 1; // send pressure data over Serial
        config.gps_mode = 3; // GPS off
        Serial.println(F("Lab test mode: no GPS, ascii data over Serial"));
      }else if(buffer[0] == 'b' && buffer[1] == 'i' && buffer[2] == 'n'){
        config.serial_output = 2; // send pressure data over Serial
        config.gps_mode = 3; // GPS off
        Serial.println(F("Lab test mode: no GPS, binary data over Serial"));
        
      }
    }
  }

  // At this point, the logging switch is set to on and we're about to start recording data. Prepare for data logging:

  digitalWrite(LED, LOW); digitalWrite(ERRORLED, LOW); // make sure LEDs are off

  // Initialize SD and create or open and truncate the data file.
  if (!sd.begin(SS)){ // SS is chipselect: defined in SdFat library
    error(1); // throw an error if we can't set up the SD connection.
  }

  // Set up Gem configuration
  // redefine filename for config file (groan)
  filename[0]='C';filename[1]='O';filename[2]='N';filename[3]='F';filename[4]='I';filename[5]='G';
  filename[6]='.';filename[7]='T';filename[8]='X';filename[9]='T';filename[10]='\0';filename[11]='\0';filename[12]='\0';
  if(sd.exists(filename)){ // have to do this instead of sd.exists(F("CONFIG.TXT")) for some reason
    if(!file.open(filename)){
      Serial.println(F("Error reading config file"));
      error(2); // throw an error if the config file can't be opened
    }
    Serial.println(F("Reading config file"));
    ReadConfig(&file, buffer, &buffidx, &config);
    file.close();
    // acknowledge config file read with red-blue blink
    digitalWrite(LED, HIGH);
    digitalWrite(ERRORLED, HIGH);
    delay(100);
    digitalWrite(LED, LOW);
    digitalWrite(ERRORLED, LOW);

  }else{ // if no config file, use defaults
    Serial.println(F("Config file does not exist; using defaults"));
    if(config.gps_mode != 3){ // if set (in startup), preserve it
      config.gps_mode = 1;
      GPS_count = 0;
      pps_count = 0;
      pps_print = 0;
      pps = 0;
   }
    config.gps_cycle = GPS_CYC_DEFAULT; // minutes
    config.gps_quota = GPS_QUOTA_DEFAULT; 
    config.adc_range = 0;
    config.led_shutoff = 0; // never disable LEDs
    if(config.serial_output != 1 && config.serial_output != 2){ //if set (in startup), preserve it
      config.serial_output = 0; // do not send pressure data over serial
    }
  }
  Serial.println(F("Configuration:"));
  Serial.print(F("GPS Mode (1-Cycled, 2-On, 3-Off): ")); Serial.println(config.gps_mode);
  Serial.print(F("GPS Cycle Duration (min.): ")); Serial.println(config.gps_cycle);
  Serial.print(F("GPS Quota (Fixes): ")); Serial.println(config.gps_quota);
  Serial.print(F("LED Shutoff (minutes): ")); Serial.println(config.led_shutoff);
  Serial.print(F("Serial Output (0-off, 1-on): ")); Serial.println(config.serial_output);
  Serial.print(F("ADC Range: ")); Serial.println(config.adc_range);

  // set the PGA gain. The INA118 clips at +0.55V, so no sense turning gain below 8
  if(config.adc_range == 2){ // Don't actually use this setting for Gems with INA118 (v0.98-v1.0)
    adc.setGain(GAIN_FOUR);  
  }else if(config.adc_range == 1){ // This is always ok with the amp, but consider the sensor's linearity
    adc.setGain(GAIN_EIGHT);  
  }else{
    adc.setGain(GAIN_SIXTEEN); // default, appropriate for most infrasound
  }  

  // Open the first output data file
  FindFirstFile(filename, &sd, &file, &SN); // find the first filename of the form "FILEXXXX.#SN" that doesn't already exist
  OpenNewFile(&sd, filename, &file, &config, &last_sample); // for some reason, this takes a long time (~4s), but for the first file only.
  sampling = 1; // tell the high-priority thread to start sampling the ADC
  firstfile = 1; // this is the first file since acquisition starts

  // Turn the GPS off if needed
  if(config.gps_mode == 3){
    Serial.println(F(PMTK_STANDBY));
  }

  while(logging[0]){ // stay in this sub-loop until the logging switch is turned off
    //////////////////////////////////
    // listen for a GPS string. When logging first begins, the GPS is on, but GPS_on is false. 
    // This keeps the logger from parsing the GPS string during the start-file bottleneck, but lets
    // it search for a fix immediately, saving time.
    while(GPS_on && (Serial.available() > 0)){ 
      if(pps_print){
        file.print(F("P,")); file.println(pps_millis % MILLIS_ROLLOVER);
        pps_print = 0;
        if(config.led_shutoff == 0 || (firstfile == 1 && (sample_count/6000) < config.led_shutoff)){
          digitalWrite(ERRORLED, HIGH); // blink red when the PPS arrives
        }
        pps_count++;
      }else{
        digitalWrite(ERRORLED, LOW);
      }
      c = Serial.read();
      //file.println(c); // debugging line: if GPS isn't working, print characters directly to file. this should normally be commented out to keep files uncluttered.
      if((!GPS_flag) || (c == '\r')){ // if GPS_flag is off or c is a carriage return, ignore it
        continue;
      }else if((buffidx == 0) && (c != '$')){ // do nothing with the character if the string hasn't started yet
        continue;
      }
      if(c == '\n'){ // newline is the end of the string--parse it, write it to disk, and reset everything
        uint8_t fail = ParseRMC(buffer, &G);
        if(fail == 0){  // if it's a good RMC string, this statement will run
          printRMC(&G, &file, &pps_millis); 
          GPS_count++; // only increment GPS_count if it's a good string
          file.print(F("R,0,")); file.println(buffer);// 2016-07-03
        }else{ // 2016-07-03
          file.print(F("R,")); file.print(fail); file.print(F(",")); file.println(buffer);// 2016-07-03
        }// 2016-07-03
        // done with this string: reset all the GPS-related variables
        for(buffidx = 0; buffidx < BUFFERLENGTH; buffidx++) buffer[buffidx] = 0; // clear buffer
        GPS_flag = 0;
        buffidx = 0;
        continue;
      }
      // if we're here, we care about the character and it isn't the end of the string, so add it to the buffer and increment buffidx.
      buffer[buffidx++] = c;
      // safety feature: if the buffer reaches its max length, stop listening and reset everything
      if(buffidx >= (BUFFERLENGTH-1)){
        file.print("E,"); file.println(buffer);// 2016-07-03
        for(buffidx = 0; buffidx < BUFFERLENGTH; buffidx++) buffer[buffidx] = 0;
        buffidx = 0;
        GPS_flag = 0;
      }
    }
    // done listening for GPS string
    //////////////////////////////////////
    // Write all data in the FIFO to the disk.
    writeStartTime = millis();// remember start time for writing this sample
    Record_t* p = fifo.waitData(TIME_IMMEDIATE);// Check for an available data record in the FIFO.
    if(!p) continue;// Continue if no available data records in the FIFO.
    printdata(p, &file, &pps_millis, &config, &last_sample);  // Print data to file
    fifo.signalFree(); // Signal the read thread that the record is free.

    // done writing to disk.  now, just a bunch of timekeeping stuff.

    // Check to see if pps_count is too high (GPS strings not being logged). If yes, restart GPS.
    if( (pps_count - GPS_count) > GPS_RESET_THRESHOLD ){
      GPS_startup(&config);
      file.println(F("E,GPS_startup"));
      pps_count = 0;
    }
  
    // Turn on GPS at 1000th sample in cycle (1000 to reduce bottleneck at file beginning).

    if(config.gps_mode != 3 && (sample_count % (config.gps_cycle * 60L * 100L)) == 1000){    // config.gps_cycle is in minutes. Note that if this is 0, anything % 0 is undefined, so need to make sure it's >0
      delay(10);
      Serial.println(F(PMTK_AWAKE));
      GPS_on = 1;
    }
    
    // Check to see if enough GPS strings have been collected this cycle, and put the GPS on standby if yes.
    if(config.gps_mode != 2 && GPS_count >= config.gps_quota){ // if gps mode is cycled or off, and GPS_count > quota, turn the gps off.
      Serial.println(F(PMTK_STANDBY));
      GPS_count = 0;
      pps_count = 0;
      GPS_on = 0;
    }
   
    // Print metadata and sync every N s (N/2 mod N to avoid reduce load at start-file bottleneck)
    if((sample_count % META_CYC) == (META_CYC/2)){
      printmeta(&file, &fifo, &maxWriteTime, &GPS_on, &AVCC); 
      file.sync(); // sync with every metadata line
      if(GPS_on == 0){
        digitalWrite(ERRORLED, LOW); // just in case light is on (from overrun, e.g.) turn it off --JFA 2015/03/06
      }
    }
    
    // switch files once we've recorded the sample limit
    if(sample_count % (FILE_LENGTH_DEFAULT * 60L * 100L) == 0 && sample_count > 0){ // switch files if config.file_length reached--factor of 10 needed
      EndFile(&file); // close the old file
      IncrementFilename(filename); // move to the next filename (e.g., FILE0001.TXT to FILE0002.TXT. Note that this does not make sure that the new file name is not already taken!)
      OpenNewFile(&sd, filename, &file, &config, &last_sample); // open the new file
      sample_count = 0;
      AdcError = 0; // in the next raw format, AdcError should be tracked in metadata, and maybe reset every metadata write.
      firstfile = 0; // this is no longer the first file since acquisition started
    }
    
    // miscellaneous stuff
    logstatus(logging); // check to see whether the logging switch is still on
    sample_count++; // increment the sample count
    if(config.led_shutoff == 0 || (firstfile == 1 && sample_count < (6000L * config.led_shutoff))){
      BlinkLED(&sample_count, &GPS_on, &GPS_count); // blink the ACQ LED
    }
    writeStartTime = millis() - writeStartTime; // Track max write time
    if (writeStartTime > maxWriteTime){
      maxWriteTime = writeStartTime; // update maxWriteTime (milliseconds): this is the longest time a write-cycle took in this metadata period.
    }
  }
  // Done, close the file and print stats.
  sampling = 0;
  EndFile(&file);
  EndLogging(&maxWriteTime, &fifo, &sample_count);
}
//////////////////////////////////////
void PPS_INT(){
  pps = 1;
  pps_millis = millis();
}
//////////////////////////////////////
void error(int8_t code){
  Serial.print(F("Error code ")); Serial.println(code);
  file.print(F("E,3")); file.sync();
  while(logging[0]){
    logstatus(logging);
    for(uint8_t i = 0; i < code; i++){
      digitalWrite(ERRORLED, HIGH);
      delay(20);
      digitalWrite(ERRORLED, LOW);
      delay(313);
    }
    delay(1000);
  }
}
// Error types:
// 1: sd.begin
// 2: sd.open
// 3: ADS1115 communication
// 4: overruns--can be due to bad SD connection

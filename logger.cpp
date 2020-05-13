#include "logger.h"
#include <EEPROM.h>
#include "version.h"

void printdata(Record_t* p, SdFile* file, volatile uint16_t* pps_millis, GemConfig *config, int16_t* last_sample){
  // print data to the serial connection if needed
  if(config->serial_output == 1){
    Serial.println(p->pressure); // ascii data stream
  }else if(config->serial_output == 2){
    Serial.write((int8_t)(p->pressure / 256)); // binary data stream
    Serial.write((uint8_t)(p->pressure % 256)); // binary data stream
  }
  // write a data line
  //if(config->compression == 1){
    file->print(F("D"));
    file->printField(p->time % MILLIS_ROLLOVER, ',');
    file->println(p->pressure - *last_sample);
    *last_sample = p->pressure;
  /*}else{
    file->print(F("D,"));
    file->printField(p->time, ',');
    file->printField(p->pressure, ',');
    file->println((uint16_t)millis() - p->time);
  }*/ // disable uncompressed output
}
void printmeta(SdFile* file, NilStatsFIFO<Record_t, FIFO_DIM>* fifo, uint16_t* maxWriteTime, uint8_t* GPS_on, float* AVCC){
  uint16_t reading;
  // to reduce adc error, preliminary throwaway reading is taken before final reading for each pin. This gives the pin time to settle after being connected to ADC.
  analogRead(BATVOLT); 
  file->print(F("M,"));
  file->printField((uint16_t)millis() % MILLIS_ROLLOVER, ',');
  reading = analogRead(BATVOLT);
  analogRead(TEMP);
  file->printField(((float)reading)/1023.0 * *AVCC * 5.54545, ',', 2); // in V; 5.54545 is the inverse of the voltage divider gain (12.2/2.2)
  reading = analogRead(TEMP);
  analogRead(2);
  file->printField(((float)reading)* *AVCC/1023.0 * 100.0 - 50.0, ',', 1); // Celsius
  reading = analogRead(2);
  analogRead(3);
  file->printField(((float)reading)* *AVCC/1023.0, ',', 3); // V
  reading = analogRead(3);
  file->printField(((float)reading)* *AVCC/1023.0, ',', 3); // V
  
  file->printField(*maxWriteTime, ',');
  file->printField(fifo->minimumFreeCount(), ',');
  file->printField(FIFO_DIM - fifo->minimumFreeCount(), ','); 
  file->printField(fifo->maxOverrunCount(), ',');
  file->printField(*GPS_on, ','); // 2015/06/08: tell us whether the GPS is on
  nilPrintUnusedStack(file); // 2015/06/08: ends in \n; no need for println 
  *maxWriteTime = 0; // reset the max write time
  fifo->resetMaxOverrun();
  fifo->resetMinFree();
}
void printRMC(RMC* G, SdFile* file, volatile uint16_t *pps_millis){
  if((uint16_t)(millis()-*pps_millis) > 1000){
    file->print(F("E,1,")); file->printField(millis(), ','); file->printField(*pps_millis, ','); file->println((uint16_t)(millis()-*pps_millis));
    return; // pps_millis probably matches a different sample if this is the case
  }
  if(G->year == 2000){
    file->print(F("E,2,")); file->println(G->year);
    return; // year=2000 is a missing value
  }
  if(G->lat == 0.0){
    file->print(F("E,3,"));file->println(G->lat);
    return; // lat = 0 is a missing value
  } // lat = 0 is a missing value code and it's also the equator. Bad design, but even if we're on the equator within about a meter, only a fraction of strings will be rejected.
  if(G->second != (float)((int)G->second)){
    file->print(F("E,4,")); file->println(G->second);
    return; // non-integer seconds is unreliable--typically means that the time comes from RTC instead of GPS
  }
  file->print("G,");
  file->printField(*pps_millis % MILLIS_ROLLOVER, ',');file->printField(G->millisParsed-*pps_millis, ',');
  file->printField(G->year, ',');
  file->printField(G->month, ',');
  file->printField(G->day, ',');
  file->printField(G->hour, ',');
  file->printField(G->minute, ',');
  file->printField(G->second, ',', 1); // 0 decimal points (s)
  file->printField(G->lat, ',', 5); // 5 decimal points (~1m)
  file->println(G->lon, 5); // 5 decimal points (~1m)
}

void FindFirstFile(char fname[13], SdFat* sd, SdFile* file, int16_t* SN){
//  SdFile file;
  
  int16_t current_fn = -1;
  int16_t greatest_fn = -1;
  sd->chdir("/",true);
  while (file->openNext(sd->vwd(),O_READ)){
    file->getFilename(fname);
    if(fname[0]=='F' && fname[1]=='I' && fname[2]=='L' && fname[3]=='E' &&
       fname[4]>='0' && fname[4]<='9' && fname[5]>='0' && fname[5]<='9' &&
       fname[6]>='0' && fname[6]<='9' && fname[7]>='0' && fname[7]<='9'){
  
      current_fn = 1000*(fname[4]-'0') + 100*(fname[5]-'0') + 
                10*(fname[6]-'0') + (fname[7]-'0');
      if(current_fn > greatest_fn){
        greatest_fn = current_fn;  
 //       Serial.println(fname);
      }      
    }
    file->close();
  }
  greatest_fn++;
  fname[0] = 'F'; fname[1] = 'I'; fname[2] = 'L'; fname[3] = 'E';
  fname[4] = '0' + greatest_fn/1000;
  fname[5] = '0' + greatest_fn/100 % 10;
  fname[6] = '0' + greatest_fn/10 % 10;
  fname[7] = '0' + greatest_fn % 10;
  //fname[8] = '.'; fname[9] = 'T'; fname[10] = 'X'; fname[11] = 'T';
  fname[8] = '.'; fname[9] = '0'+((*SN/100) % 10); fname[10] = '0'+((*SN/10) % 10); fname[11] = '0'+(*SN % 10); // set the serial number as the filename extension
  Serial.println(greatest_fn);
  Serial.println(fname);
  // These two apparently pointless lines are needed to prevent
  // delays on the first write to SD.
//  file->open("FILE0000.TXT", O_WRITE | O_TRUNC | O_CREAT);
    file->open("FILE0000.TXT", O_CREAT);
  file->close();

}
void IncrementFilename(char fname[13]){ // change to pointer? does it matter?
  uint16_t i = (1 + 1000*(fname[4]-'0') + 100*(fname[5]-'0') + 10*(fname[6]-'0') + (fname[7]-'0')) % 10000; 
  fname[4] = i/1000 + '0';
  fname[5] = (i/100) % 10 + '0';
  fname[6] = (i/10) % 10 + '0';
  fname[7] = i % 10 + '0';
}
  
void logstatus(int8_t logging[2]){
  // logging[0] is current status. logging[1] is number of consecutive changed readings.
  // change logging status when 10 changed readings are recorded. This is to ignore switch bouncing.
  //int8_t switchstatus = digitalRead(SWITCH); // read current switch status

  // if switchstatus is currently on, increment switch on count, and change logging status if it's >10
  if(digitalRead(SWITCH) == 1){  // switchstatus
    if(++logging[1] > 10){
      logging[1] = 10;
      logging[0] = 1;
    }
    // if switchstatus is off, increment switch off count and change logging status if it's <-10
  }else{
    if(--logging[1] < -10){
      logging[1] = -10;
      logging[0] = 0;
    }
  }
}

void OpenNewFile(SdFat* sd, char filename[13], SdFile* file, GemConfig* config, int16_t* last_sample){
  //Serial.println(10);
  if(!file->open(filename, O_WRITE | O_TRUNC | O_CREAT)) { 
    error(2); 
  }
  //Serial.println(20);
  /*
  if(config->compression == 0){ // compression off
    file->println(F("#GemCSV0.85")); 
    file->println(F("#D,msSamp,ADC,msLag"));
  }else if(config->compression == 1){ // compression on*/
    *last_sample = 0; // so it writes the actual value, not the diff, as the first sample in each file
    file->println(F("#GemCSV0.9"));
    file->println(F("#DmsSamp,ADC"));
  //}
  file->println(F("#G,msPPS,msLag,yr,mo,day,hr,min,sec,lat,lon"));
  file->println(F("#M,ms,batt(V),temp(C),A2,A3,maxLag,minFree,maxUsed,maxOver,gpsFlag,freeStack1,freeStackIdle"));

  file->print(F("S,"));
  file->print(EEPROM.read(0)-'0');
  file->print(EEPROM.read(1)-'0');
  file->println(EEPROM.read(2)-'0');

  file->print(F("#Firmware"));file->println(F(FIRMWARE_VERSION));

  // print configuration
  file->print(F("C,"));
  file->printField(config->gps_mode, ',');
  file->printField(config->gps_cycle, ',');
  file->printField(config->gps_quota, ',');
  file->printField(config->adc_range, ',');
  file->printField(config->led_shutoff, ',');
  file->println(config->serial_output);

}

void BlinkLED(uint32_t* sample_count, uint8_t* GPS_on, uint8_t* GPS_count){
  // codes: 1 (acq, GPS off), 2 (acq, GPS searching), 3 (acq, GPS fix)
  uint8_t s100 = *sample_count % 100;
  if(s100 == 0){
    digitalWrite(LED, HIGH);
  }else if(s100 == 2){
    digitalWrite(LED, LOW);
  }else if((s100 == 20) && (*GPS_on == 1)){ // GPS on
    digitalWrite(LED, HIGH);
  }else if((s100 == 22) && (*GPS_on == 1)){ // GPS on
    digitalWrite(LED, LOW);
  }else if((s100 == 40) && (*GPS_count > 0)){ // GPS on and strings logged
    digitalWrite(LED, HIGH);
  }else if((s100 == 42) && (*GPS_count > 0)){ // GPS on and strings logged
    digitalWrite(LED, LOW);
  }
}
void EndFile(SdFile* file){
  // no footer to print anymore.  maybe not necessary?
  file->close();
}
void EndLogging(uint16_t* maxWriteTime, NilStatsFIFO<Record_t, FIFO_DIM>* fifo, uint32_t* sample_count){
  nilPrintUnusedStack(&Serial);
  fifo->printStats(&Serial);
  digitalWrite(LED, HIGH);
  digitalWrite(ERRORLED, HIGH);
  delay(100);
  digitalWrite(LED, LOW);
  digitalWrite(ERRORLED, LOW);
  Serial.println(F(PMTK_STANDBY));
}

void GPS_startup(GemConfig* config){
  Serial.begin(9600);
  Serial.println(F(PMTK_SET_BAUD_57600));
  delay(50);
  Serial.begin(57600);
  delay(50);
  delay(50);
  if(config->gps_mode != 3){ // if GPS is set to "on" or "cycled", turn it on now
    Serial.println(F(PMTK_AWAKE)); // command to wake up GPS
    delay(50);
    Serial.println(F(PMTK_SET_NMEA_OUTPUT_RMCONLY));
    //Serial.println(F(PMTK_SET_NMEA_OUTPUT_ALLDATA));
    delay(50);
    Serial.println(F(PMTK_SET_NMEA_UPDATE_1HZ));
  }else{ // if it's set to "off", turn it to standby
    Serial.println(F(PMTK_STANDBY));
  }
}

//////////////////////////////
uint8_t ParseRMC(char* nmea, RMC* G) {
  // Heavily modified from Adafruit_GPS library: required attribution to Adafruit follows. Any bugs are my fault (JFA), not Adafruit's.
  /***********************************
  This is our GPS library
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!
  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, check license.txt for more information
  All text above must be included in any redistribution
  */
  uint8_t failure_code = 0;
  float degreef;
  float minutesf;
    
  // check to see whether nmea has a checksum and whether it is correct
  if (nmea[strlen(nmea)-4] == '*') {
    uint16_t sum = parseHex(nmea[strlen(nmea)-3]) * 16;
    sum += parseHex(nmea[strlen(nmea)-2]);
    
    // check checksum 
    for (uint8_t i=1; i < (strlen(nmea)-4); i++) {
      sum ^= nmea[i];
    }

    if (sum != 0) {
      failure_code = 1; // bad checksum
    }
  }
  char degreebuff[10];
  if(strstr(nmea, "$GPRMC")) {
    G->millisParsed = millis();
   // found RMC
    char *p = nmea; // p is an address that increments as we advance through nmea
    // get time
    p = strchr(p, ',')+1; 
    // why use atof? why not convert directly to long instead of float?
    G->hour = ((uint32_t)atof(p)) / 10000; // atof converts the first contiguous run of numbers in a character array to a float. so, the following comma and everything after is ignored.
    G->minute = (((uint32_t)atof(p)) % 10000) / 100;
    G->second = atof(p+4);// - ((float)G->hour)*10000 - ((float)G->minute)*100;

    if(G->second != ((float)((int)G->second))){ // if second isn't an int, don't trust it
      failure_code = 2; // "seconds" value is not an int, so we ignore it just to be safe
    }
    p = strchr(p, ',')+1;
    
    // check GPS fix status (active vs. void)
    if (p[0] == 'A'){
      // do nothing
    }else if (p[0] == 'V'){
      failure_code = 3; // GPS fix is void; ignore data
    }else{
      failure_code = 4; // GPS fix is neither void nor active: shouldn't happen except with transmission error. ignore.
    }
    
    // parse latitude
    p = strchr(p, ',')+1;
    strncpy(degreebuff, p, 2);
    p += 2;
    degreebuff[2] = '\0';
    /*
    long degree = atol(degreebuff) * 10000000;
    strncpy(degreebuff, p, 2); // minutes
    p += 3; // skip decimal point
    strncpy(degreebuff + 2, p, 4);
    degreebuff[6] = '\0';
    long minutes = 50 * atol(degreebuff) / 3;
    G->lat = ((float)(degree+minutes))/10000000.0;
    */
    degreef = atof(degreebuff); // 2015/6/8: try this with floats and see if it works.  if yes, change lat too; otherwise revert.
    strncpy(degreebuff, p, 2); // minutes
    p += 3; // skip decimal point
    strncpy(degreebuff + 2, p, 4);
    degreebuff[6] = '\0';
    minutesf = atof(degreebuff)/10000;
    G->lat = degreef + minutesf/60;
    p = strchr(p, ',')+1;
    if(p[0] == 'S') G->lat = -G->lat;
    
    if(G->lat == 0.0){ // 0.0 is missing value for lat--bad choice by GPS manufacturer
      failure_code = 5; // lat is equal to the missing value. If you're within a few m of the equator, this will sometimes happen spuriously.
    }

    // parse longitude
    p = strchr(p, ',')+1;
    strncpy(degreebuff, p, 3);
    p += 3;
    degreebuff[3] = '\0';
    float degreef = atof(degreebuff); // 2015/6/8: try this with floats and see if it works.  if yes, change lat too; otherwise revert.
    strncpy(degreebuff, p, 2); // minutes
    p += 3; // skip decimal point
    strncpy(degreebuff + 2, p, 4);
    degreebuff[6] = '\0';
    float minutesf = atof(degreebuff)/10000;
    G->lon = degreef + minutesf/60;
    p = strchr(p, ',')+1;
    if(p[0] == 'W') G->lon = -G->lon;
    
    p = strchr(p, ',')+1; // speed: discard
    p = strchr(p, ',')+1; // angle: discard

    // date
    p = strchr(p, ',')+1;
    G->day = (uint32_t)atof(p) / 10000;
    G->month = ((uint32_t)atof(p) % 10000) / 100;
    G->year = ((uint32_t)atof(p) % 100) + 2000;
    if(G->year == 2000){ // 2000 is missing value for year
      failure_code = 6; // year is the missing value of "2000"
    }
    if(G->year > 2040){ // bad strings often have years far in the future
      failure_code = 7; // year is improbably far in the future. I doubt the Gem will still be in use in 2040!
    }
    if(G->hour > 23 || G->minute > 59 || G->second > 59 || G->day > 31 || G->month > 12){ // all of these values are impossible and common in bad strings
      failure_code = 8; // one of the date/time values is impossible
    }
    return failure_code;
  }
  failure_code = 9;   // if we're here, an RMC string wasn't detected
  return failure_code;
}


uint8_t parseHex(char c) {
  // from Adafruit_GPS library
    if (c < '0')
      return 0;
    if (c <= '9')
      return c - '0';
    if (c < 'A')
       return 0;
    if (c <= 'F')
       return (c - 'A')+10;
}





int16_t SincFilt(int16_t buf1[4], int16_t buf2[4], int16_t buf3[4], int16_t buf4[4], int16_t buf5[4], int16_t buf6[4], int16_t* reading){ // change to pointers?
  // Digital Filter: treat each sample in the buffers as int32 to prevent overruns. 
  // This allows high precision in filter coefficients while keeping memory use low.
  int32_t x = ( ((int32_t) buf1[0] + *reading) * C0 + ((int32_t) buf1[1] + buf6[3]) * C1 + 
            ((int32_t) buf1[2] + buf6[2]) * C2 + ((int32_t) buf1[3] + buf6[1]) * C3 +
            ((int32_t) buf2[0] + buf6[0]) * C4 + ((int32_t) buf2[1] + buf5[3]) * C5 + 
            ((int32_t) buf2[2] + buf5[2]) * C6 + ((int32_t) buf2[3] + buf5[1]) * C7 +
            ((int32_t) buf3[0] + buf5[0]) * C8 + ((int32_t) buf3[1] + buf4[3]) * C9 + 
            ((int32_t) buf3[2] + buf4[2]) * CA + ((int32_t) buf3[3] + buf4[1]) * CB +
            buf4[0] * CC ) / 32767; //32767 is the sum of the coefficients (rounded off)

  // make sure that the output is within the bounds of the int16 type. if not, clip it at the positive or negative limit.
  if(x > 32767){
    return (int16_t) 32767;
  }else if(x < -32768){
    return (int16_t) -32768;
  }else{
    return (int16_t) x;
  }
  
}
void ReadConfig(SdFile *file, char *buffer, uint8_t *buffidx, GemConfig *config){
  int32_t x;
  // parse first line: gps_mode (1 cycled, 2 on, 3 off)
  x = ReadConfigLine(file, buffer, buffidx);
  Serial.println(buffer);
  Serial.println(x);
  if(config->gps_mode != 3){ // if gps_mode is already 3, then it has been set to labtest mode--override the config file
    if(x > 3 || x < 1){
      config->gps_mode = 1;
    }else{
      config->gps_mode = x;
    }
  }
  // parse second line: gps_cycle
  x = ReadConfigLine(file, buffer, buffidx);
  if(x > 65535 || x < 1){ // cycle cannot be less than 1 minute. use "gps_mode" to 
    config->gps_cycle = GPS_CYC_DEFAULT;
  }else{
    config->gps_cycle = x;
  }
  // parse third line: gps_quota
  x = ReadConfigLine(file, buffer, buffidx);
  if(x > 255 || x < 0){
    config->gps_quota = GPS_QUOTA_DEFAULT;
  }else{
    config->gps_quota = x;
  }
  // parse fourth line: gain
  x = ReadConfigLine(file, buffer, buffidx);
  if(x == 1){ 
    config->adc_range = 1; // ADC input range +/-0.512V
  }else{
    config->adc_range = 0; // default: ADC input range +/-0.256V
  }
  // parse fifth line: led_shutoff
  x = ReadConfigLine(file, buffer, buffidx);
  if(x > 255 || x < 0){
    config->led_shutoff = 0; // default: never turn off
  }else{
    config->led_shutoff = x;
  }
  // parse sixth line: serial_output
  x = ReadConfigLine(file, buffer, buffidx);
  if(x == 1 || config->serial_output == 1){ // if serial_output is already set (by user on startup), it's in labtest mode. preserve this.
    config->serial_output = 1; // send pressure data over serial connection as well as to SD. This could maybe interfere with GPS.
  }else{
    config->serial_output = 0; // default: do not send pressure data over serial
  } 
  // User doesn't really need control over these things; disable as config options
  // parse sixth line: data compression
  //x = ReadConfigLine(file, buffer, buffidx);
  //if(x == 0){ 
  //  config->compression = 0; // no compression
  //}else{
    config->compression = 1; // default: compression mode 1 (omit certain commas, log pressure differences, skip data millisLag, 13-bit millis
  //}
  // parse eighth line: file length
  //x = ReadConfigLine(file, buffer, buffidx) / 10;
  //if(x > 0 && x < pow(2,8)){ 
  //  config->file_length = x; // file length in tens of minutes
  //}else{
    config->file_length = FILE_LENGTH_DEFAULT/10;
  //}*/
}
int32_t ReadConfigLine(SdFile *file, char *buffer, uint8_t *buffidx){
  /* procedure: log characters (except whitespace and anything after a #) until \n is found. Then, run the result through strtol.
   *  Leading whitespace is allowed in the line. The first non-digit after a digit or 
   *  non-whitespace, non-digit before the first digit truncates parsing in strtol.
  */

  char *c;
  int32_t output;
  // first: reset the buffer
  for(*buffidx = 0; *buffidx < BUFFERLENGTH; (*buffidx)++) buffer[*buffidx] = 0;
  *buffidx = 0;
  while(*buffidx == 0){ // keep reading new lines until we read at least one digit before a comment
    while(file->available()){  // main loop, starting at the beginning of the line
      *c = file->read();
      //Serial.print(*buffidx); Serial.println(*c);
      if(*c == ' ' || *c == '\r' || *c == '\t') continue; // ignore spaces, CR, tabs: this is necessary to avoid parsing a line that just has whitespace followed by # 
      
      // if we hit the comment character '#', skip ahead to the next '\n'
      if(*c == '#'){
        while(file->available()){
          *c = file->read();
          if(*c == '\n') break; // stop advancing once we hit '\n'
        }
      } // end of comment-processing if statement
 
      
      // if we've hit a '\n', regardless of whether it's after a number, in a comment, or neither, stop advancing
      if(*c == '\n') break; 

      // if we're here, c is neither a space, tab, CR, \n, and does not follow a comment. so, save it to buffer.
      buffer[(*buffidx)++] = *c;
    }
    if(!file->available()) break; // if we're out of characters in the file, stop trying to read it
  }
  /* strtol does all the hard work. it eliminates any leading whitespace and parses an optional 
   * sign and all consecutive digits, and throws out the rest of the string. It returns zero
   * if it can't parse a number by this method, which is a problem, BUT it also tells you (through
   * its second argument) the address of the first byte where it couldn't parse any more. So, if
   * that address is equal to the address of buffer, it's an error. If it isn't, then the zero is real.
   */
  output = strtol(buffer, &c, 10); 
  // Catch errors here, since strtol returns zero if there's an error.
  if(*buffidx == 0 || (buffer - c) == 0){ // never read any data, or line cannot be parsed as a number
    output = -2147483648; // error code
  }
  Serial.println(buffer);
  Serial.println(output);
  return(output);
}

/*
 *     From ReadConfigLine
      // decide what to do with the current character. 
      c = file->read();
      if(c == ' ' || c == '\r') next; // ignore spaces and CR

      // if we hit the comment character '#', skip ahead to the next '\n'
      if(c == '#'){
        while(file->available()){
          c = file->read();
          if(c == '\n') break; // stop advancing once we hit '\n'
        }
      } // end of comment-processing if statement
  
  
      // finally, if we're at this point, check to see if it's a digit. if yes, keep reading numbers until they stop being numbers, then skip to the end of the line.
      if((c >= '0' && c <= '9')){
        [(*buffidx)++] = c;
        while(file->available()){
          c = file->read();
          if(c >= '0' && c <= '9'){ // this character is a digit, so add it to buffer
            buffer[(*buffidx)++] = c;
          }else{
            break; // stop reading numbers if we're here; time to skip to the end of the line
          }
        } // end of number-reading loop
  
        if(c == '\n') break; // if the first character after the digits is \n, break from main loop
        
        // if we're here, we found a character that isn't a digit and isn't \n. keep reading until we hit \n.
        while(file->available()){
          c = file->read();
          if(c == '\n') break;
        }
      } // end of number-processing if statement
  
*/

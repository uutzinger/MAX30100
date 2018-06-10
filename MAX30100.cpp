/***************************************************
  This is a library written for the Maxim MAX30100 Optical Pulse Detector
  It should also work with the MAX30102. However, the MAX30102 does not have a Green LED. 
  It will also work for MAX30100 sensor.

  These sensors use I2C to communicate, as well as a single (optional)
  interrupt line that is not currently supported in this driver.

  Written by Peter Jansen and Nathan Seidle (SparkFun)
  BSD license, all text above must be included in any redistribution.
 *****************************************************/

#include "MAX30100.h"

//The MAX30100 stores up to 16 samples on the IC
//This is additional local storage to the microcontroller
//const int STORAGE_SIZE = MAX30100_FIFO_DEPTH;
const int STORAGE_SIZE = 4;
struct Record
{
  uint16_t red[STORAGE_SIZE];
  uint16_t IR[STORAGE_SIZE];
  byte head;
  byte tail;
} sense; //This is our circular buffer of readings from the sensor

MAX30100::MAX30100() {
  // Constructor
}

boolean MAX30100::begin(TwoWire &wirePort, uint32_t i2cSpeed, uint8_t i2caddr) {

  _i2cPort = &wirePort; //Grab which port the user wants us to use

  _i2cPort->begin();
  _i2cPort->setClock(i2cSpeed);

  _i2caddr = i2caddr;

  // Step 1: Initial Communication and Verification
  // Check that a MAX30100 is connected
  uint8_t id = readPartID();
  Serial.println(id);
  Serial.println(MAX_30100_EXPECTEDPARTID);
  if (!(id == MAX_30100_EXPECTEDPARTID)) {
    // Error -- Part ID read from MAX30100 does not match expected part ID.
    // This may mean there is a physical connectivity problem (broken wire, unpowered, etc).
    return false;
  }
  // Populate revision ID
  readRevisionID();
  return true;
}

//
// Configuration
//

///////////////////////////////////////
//Begin Interrupt configuration
uint8_t MAX30100::getINT(void) {
  return (readRegister8(_i2caddr, MAX30100_INTSTAT));
}

void MAX30100::enableAFULL(void) {
  bitMask(MAX30100_INTENABLE, MAX30100_INT_A_FULL_MASK, MAX30100_INT_A_FULL_ENABLE);
}
void MAX30100::disableAFULL(void) {
  bitMask(MAX30100_INTENABLE, MAX30100_INT_A_FULL_MASK, MAX30100_INT_A_FULL_DISABLE);
}

void MAX30100::enableTEMPRDY(void) {
  bitMask(MAX30100_INTENABLE, MAX30100_INT_TEMP_RDY_MASK, MAX30100_INT_TEMP_RDY_ENABLE);
}
void MAX30100::disableTEMPRDY(void) {
  bitMask(MAX30100_INTENABLE, MAX30100_INT_TEMP_RDY_MASK, MAX30100_INT_TEMP_RDY_DISABLE);
}

void MAX30100::enableHRRDY(void) {
  bitMask(MAX30100_INTENABLE, MAX30100_INT_HR_RDY_MASK, MAX30100_INT_HR_RDY_ENABLE);
}
void MAX30100::disableHRRDY(void) {
  bitMask(MAX30100_INTENABLE, MAX30100_INT_HR_RDY_MASK, MAX30100_INT_HR_RDY_DISABLE);
}

void MAX30100::enableSPO2RDY(void) {
  bitMask(MAX30100_INTENABLE, MAX30100_INT_SPO2_RDY_MASK, MAX30100_INT_SPO2_RDY_ENABLE);
}
void MAX30100::disableSPO2RDY(void) {
  bitMask(MAX30100_INTENABLE, MAX30100_INT_SPO2_RDY_MASK, MAX30100_INT_SPO2_RDY_DISABLE);
}

//End Interrupt configuration
////////////////////////////////////

void MAX30100::softReset(void) {
  bitMask(MAX30100_MODECONFIG, MAX30100_RESET_MASK, MAX30100_RESET);
  // Poll for bit to clear, reset is then complete
  // Timeout after 100ms
  unsigned long startTime = millis();
  while (millis() - startTime < 100)
  {
    uint8_t response = readRegister8(_i2caddr, MAX30100_MODECONFIG);
    if ((response & MAX30100_RESET) == 0) break; //We're done!
    delay(1); //Let's not over burden the I2C bus
  }
}

void MAX30100::shutDown(void) {
  // Put IC into low power mode (datasheet pg. 19)
  // During shutdown the IC will continue to respond to I2C commands but will
  // not update with or take new readings (such as temperature)
  bitMask(MAX30100_MODECONFIG, MAX30100_SHUTDOWN_MASK, MAX30100_SHUTDOWN);
}

void MAX30100::wakeUp(void) {
  // Pull IC out of low power mode (datasheet pg. 19)
  bitMask(MAX30100_MODECONFIG, MAX30100_SHUTDOWN_MASK, MAX30100_WAKEUP);
}

void MAX30100::setLEDMode(uint8_t mode) {
  // Set which LEDs are used for sampling -- HR only or SPO2 mode.
  // MAX30100_MODE_HR
  // MAX30100_MODE_SPO2
  bitMask(MAX30100_MODECONFIG, MAX30100_MODE_MASK, mode);
}

void MAX30100::setSampleRate(uint8_t sampleRate) {
  // sampleRate: one of MAX30100_SAMPLERATE_50, 
  // _100, _167, _200, _400, _600, _800, _1000
  bitMask(MAX30100_SPO2CONFIG, MAX30100_SAMPLERATE_MASK, sampleRate);
}

void MAX30100::setPulseWidth(uint8_t pulseWidth) {
  // pulseWidth: one of MAX30100_PULSEWIDTH_200, _400, _800, _1600
  bitMask(MAX30100_SPO2CONFIG, MAX30100_PULSEWIDTH_MASK, pulseWidth);
}

void MAX30100::setPulseAmplitudeRed(uint8_t amplitude) {
  // NOTE: Amplitude values: 0b0000 = 0mA, 0b0111 = 24.0mA, 0b1111 = 50mA (typical)
  bitMask(MAX30100_LEDCONFIG, MAX30100_REDLED_CURR_MASK, amplitude);
}
void MAX30100::setPulseAmplitudeIR(uint8_t amplitude) {
  bitMask(MAX30100_LEDCONFIG, MAX30100_IRLED_CURR_MASK, amplitude);
}

void MAX30100::setHighresModeEnabled(void)
{
  bitMask(MAX30100_SPO2CONFIG, MAX30100_SPO2HIRESEN_MASK, MAX30100_SPO2HIRES_ENABLE);
}
void MAX30100::setHighresModeDisabled(void)
{
  bitMask(MAX30100_SPO2CONFIG, MAX30100_SPO2HIRESEN_MASK, MAX30100_SPO2HIRES_DISABLE);
}

//
// FIFO Configuration
//

//Resets all points to start in a known state
//Page 15 recommends clearing FIFO before beginning a read
void MAX30100::clearFIFO(void) {
  writeRegister8(_i2caddr, MAX30100_FIFOWRITEPTR, 0);
  writeRegister8(_i2caddr, MAX30100_FIFOOVERFLOW, 0);
  writeRegister8(_i2caddr, MAX30100_FIFOREADPTR,  0);
}

//Read the FIFO Write Pointer
uint8_t MAX30100::getWritePointer(void) {
  return (readRegister8(_i2caddr, MAX30100_FIFOWRITEPTR));
}

//Read the FIFO Read Pointer
uint8_t MAX30100::getReadPointer(void) {
  return (readRegister8(_i2caddr, MAX30100_FIFOREADPTR));
}

// Die Temperature
// Returns temp in C
float MAX30100::readTemperature(void) {
  // Step 1: Config die temperature register to take 1 temperature sample
  bitMask(MAX30100_MODECONFIG, MAX30100_TEMPREAD_MASK, MAX30100_TEMPREAD);
  // Poll for bit to clear, reading is then complete
  // Timeout after 100ms
  unsigned long startTime = millis();
  while (millis() - startTime < 100)
  {
    uint8_t response = readRegister8(_i2caddr, MAX30100_MODECONFIG);
    if ((response & MAX30100_TEMPREAD_MASK) == 0) break; //We're done!
    delay(1); //Let's not over burden the I2C bus
  }
  //TODO How do we want to fail? With what type of error?
  //? if(millis() - startTime >= 100) return(-999.0);
  // Step 2: Read die temperature register (integer)
  int8_t tempInt = readRegister8(_i2caddr, MAX30100_DIETEMPINT);
  float tempFrac = readRegister8(_i2caddr, MAX30100_DIETEMPFRAC);
  // Step 3: Calculate temperature 
  return ((float)tempInt + (tempFrac * 0.0625));
}

// Returns die temp in F
float MAX30100::readTemperatureF(void) {
  float temp = readTemperature();
  if (temp != -999.0) temp = temp * 1.8 + 32.0;
  return (temp);
}

//
// Device ID and Revision
//
uint8_t MAX30100::readPartID() {
  return readRegister8(_i2caddr, MAX30100_PARTID);
}

void MAX30100::readRevisionID() {
  revisionID = readRegister8(_i2caddr, MAX30100_REVISIONID);
}

uint8_t MAX30100::getRevisionID() {
  return revisionID;
}

//Setup the sensor
//Use the default setup if you are just getting started with the MAX30100 sensor
void MAX30100::setup(byte powerLevel, byte ledMode, int sampleRate, int pulseWidth, bool highresMode) {
  byte powerLevelRed;
  byte powerLevelIR;
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  softReset(); //Reset all configuration, threshold, and data registers to POR values
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  //Mode Configuration
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  // MAX30100_MODE_HR
  // MAX30100_MODE_SPO2
  Serial.println(ledMode);
  setLEDMode(ledMode); 
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  //The longer the pulse width the longer range of detection you'll have
  //At 69us and 0.4mA it's about 2 inches
  //At 411us and 0.4mA it's about 6 inches
  Serial.println(pulseWidth);
  if (pulseWidth < 400) setPulseWidth(MAX30100_PULSEWIDTH_200); //Page 26, Gets us 15 bit resolution
  else if (pulseWidth < 800) setPulseWidth(MAX30100_PULSEWIDTH_400); //16 bit resolution
  else if (pulseWidth < 1600) setPulseWidth(MAX30100_PULSEWIDTH_800); //17 bit resolution
  else if (pulseWidth == 1600) setPulseWidth(MAX30100_PULSEWIDTH_1600); //18 bit resolution
  else setPulseWidth(MAX30100_PULSEWIDTH_200);
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  Serial.println(sampleRate);
  if (sampleRate < 100) setSampleRate(MAX30100_SAMPLERATE_50); //Take 50 samples per second
  else if (sampleRate < 167) setSampleRate(MAX30100_SAMPLERATE_100);
  else if (sampleRate < 200) setSampleRate(MAX30100_SAMPLERATE_167);
  else if (sampleRate < 400) setSampleRate(MAX30100_SAMPLERATE_200);
  else if (sampleRate < 600) setSampleRate(MAX30100_SAMPLERATE_400);
  else if (sampleRate < 800) setSampleRate(MAX30100_SAMPLERATE_600);
  else if (sampleRate < 1000) setSampleRate(MAX30100_SAMPLERATE_800);
  else if (sampleRate == 1000) setSampleRate(MAX30100_SAMPLERATE_1000);
  else setSampleRate(MAX30100_SAMPLERATE_50);
  //LED Pulse Amplitude Configuration
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  //powerLevel = 0.4mA  - Presence detection of ~4 inch
  //powerLevel = 6.4mA  - Presence detection of ~8 inch
  //powerLevel = 25.4mA - Presence detection of ~8 inch
  //powerLevel = 50.0mA - Presence detection of ~12 inch
  Serial.println(powerLevel);
  if (powerLevel < 0x01) {
    powerLevelRed=MAX30100_REDLED_CURR_0MA;
    powerLevelIR=MAX30100_IRLED_CURR_0MA;
  } else if (powerLevel < 0x02){
    powerLevelRed=MAX30100_REDLED_CURR_4_4MA;
    powerLevelIR=MAX30100_IRLED_CURR_4_4MA;    
  } else if (powerLevel < 0x03){
    powerLevelRed=MAX30100_REDLED_CURR_7_6MA;
    powerLevelIR=MAX30100_IRLED_CURR_7_6MA;    
  } else if (powerLevel < 0x04){
    powerLevelRed=MAX30100_REDLED_CURR_11MA;
    powerLevelIR=MAX30100_IRLED_CURR_11MA;    
  } else if (powerLevel < 0x05){
    powerLevelRed=MAX30100_REDLED_CURR_14_2MA;
    powerLevelIR=MAX30100_IRLED_CURR_14_2MA;    
  } else if (powerLevel < 0x06){
    powerLevelRed=MAX30100_REDLED_CURR_17_4MA;
    powerLevelIR=MAX30100_IRLED_CURR_17_4MA;    
  } else if (powerLevel < 0x07){
    powerLevelRed=MAX30100_REDLED_CURR_20_8MA;
    powerLevelIR=MAX30100_IRLED_CURR_20_8MA;    
  } else if (powerLevel < 0x08){
    powerLevelRed=MAX30100_REDLED_CURR_24MA;
    powerLevelIR=MAX30100_IRLED_CURR_24MA;    
  } else if (powerLevel < 0x09){
    powerLevelRed=MAX30100_REDLED_CURR_27_1MA;
    powerLevelIR=MAX30100_IRLED_CURR_27_1MA;    
  } else if (powerLevel < 0x0A){
    powerLevelRed=MAX30100_REDLED_CURR_30_6MA;
    powerLevelIR=MAX30100_IRLED_CURR_30_6MA;    
  } else if (powerLevel < 0x0B){
    powerLevelRed=MAX30100_REDLED_CURR_33_8MA;
    powerLevelIR=MAX30100_IRLED_CURR_33_8MA;    
  } else if (powerLevel < 0x0C){
    powerLevelRed=MAX30100_REDLED_CURR_37MA;
    powerLevelIR=MAX30100_IRLED_CURR_37MA;    
  } else if (powerLevel < 0x0D){
    powerLevelRed=MAX30100_REDLED_CURR_40_2MA;
    powerLevelIR=MAX30100_IRLED_CURR_40_2MA;    
  } else if (powerLevel < 0x0E){
    powerLevelRed=MAX30100_REDLED_CURR_43_6MA;
    powerLevelIR=MAX30100_IRLED_CURR_43_6MA;    
  } else if (powerLevel < 0x0F){
    powerLevelRed=MAX30100_REDLED_CURR_46_8MA;
    powerLevelIR=MAX30100_IRLED_CURR_46_8MA;    
  } else {
    powerLevelRed=MAX30100_REDLED_CURR_50MA;
    powerLevelIR=MAX30100_IRLED_CURR_50MA;    
  }
  setPulseAmplitudeRed(powerLevelRed);
  setPulseAmplitudeIR(powerLevelIR);
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  Serial.println("Clear FIFO");
  clearFIFO(); //Reset the FIFO before we begin checking the sensor
  Serial.println(highresMode);
  if (highresMode) { setHighresModeEnabled(); }
  else { setHighresModeDisabled();  }
}

//
// Data Collection
//

//Tell caller how many samples are available
uint8_t MAX30100::available(void)
{
  uint8_t numberOfSamples = sense.head - sense.tail;
  if (numberOfSamples < 0) numberOfSamples += STORAGE_SIZE;
  return (numberOfSamples);
}

//Report the most recent red value
uint16_t MAX30100::getRed(void)
{
  //Check the sensor for new data for 250ms
  if(safeCheck(250))
    return (sense.red[sense.head]);
  else
    return(0); //Sensor failed to find new data
}

//Report the most recent IR value
uint16_t MAX30100::getIR(void)
{
  //Check the sensor for new data for 250ms
  if(safeCheck(250))
    return (sense.IR[sense.head]);
  else
    return(0); //Sensor failed to find new data
}

//Report the next Red value in the FIFO
uint16_t MAX30100::getFIFORed(void)
{
  return (sense.red[sense.tail]);
}

//Report the next IR value in the FIFO
uint16_t MAX30100::getFIFOIR(void)
{
  return (sense.IR[sense.tail]);
}

//Advance the tail
void MAX30100::nextSample(void)
{
  if(available()) //Only advance the tail if new data is available
  {
    sense.tail++;
    sense.tail %= STORAGE_SIZE; //Wrap condition
  }
}

// Polls the sensor for new data
// Call regularly
// If new data is available, it updates the head and tail in the main struct
// Returns number of new samples obtained
uint16_t MAX30100::check(void)
{
  //Read register FIDO_DATA in (2-byte * number of active LED (always 2 in MAX30100) chunks
  //Until FIFO_RD_PTR = FIFO_WR_PTR

  byte readPointer = getReadPointer();
  byte writePointer = getWritePointer();
  Serial.print(readPointer);  
  Serial.print(", ");  
  Serial.println(writePointer);  

  int numberOfSamples = 0;

  //Do we have new data?
  if (readPointer != writePointer)
  {
    //Calculate the number of readings we need to get from sensor
    //numberOfSamples = (writePointer - readPointer) & (MAX30100_FIFO_DEPTH-1);
    if (numberOfSamples < 0) numberOfSamples += MAX30100_FIFO_DEPTH; //Wrap condition

    //We now have the number of readings, now calc bytes to read
    //For this example we are doing Red and IR (2 bytes each)
    int bytesLeftToRead = numberOfSamples * 4;

    //Get ready to read a burst of data from the FIFO register
    _i2cPort->beginTransmission(MAX30100_ADDRESS);
    _i2cPort->write(MAX30100_FIFODATA);
    _i2cPort->endTransmission();

    //We may need to read as many as 16*4 (64) bytes so we read in blocks no larger than I2C_BUFFER_LENGTH
    //I2C_BUFFER_LENGTH changes based on the platform. 64 bytes for SAMD21, 32 bytes for Uno.
    //Wire.requestFrom() is limited to BUFFER_LENGTH which is 32 on the Uno
    while (bytesLeftToRead > 0)
    {
      int toGet = bytesLeftToRead;
      if (toGet > I2C_BUFFER_LENGTH)
      {
        toGet = I2C_BUFFER_LENGTH - (I2C_BUFFER_LENGTH % 4); //Trim toGet to be a multiple of the samples we need to read
      }
      bytesLeftToRead -= toGet;
      //Request toGet number of bytes from sensor
      _i2cPort->requestFrom(MAX30100_ADDRESS, toGet);
      while (toGet > 0)
      {
        sense.head++; //Advance the head of the storage struct
        sense.head %= STORAGE_SIZE;  // Wrap condition
        byte temp[sizeof(uint16_t)]; // Array of 2 bytes that we will convert into short
        uint16_t tempShort;
        //Burst read three bytes - IR
        temp[1] = _i2cPort->read();
        temp[0] = _i2cPort->read();
        //Convert array to long
        memcpy(&tempShort, temp, sizeof(tempShort));
        sense.IR[sense.head] = tempShort; //Store this reading into the sense array
        //Burst read three more bytes - RED
        temp[1] = _i2cPort->read();
        temp[0] = _i2cPort->read();
        //Convert array to long
        memcpy(&tempShort, temp, sizeof(tempShort));
        sense.red[sense.head] = tempShort;
        toGet -= 4;
      }
    } //End while (bytesLeftToRead > 0)
  } //End readPtr != writePtr
  return (numberOfSamples); //Let the world know how much new data we found
}

//Check for new data but give up after a certain amount of time
//Returns true if new data was found
//Returns false if new data was not found
bool MAX30100::safeCheck(uint8_t maxTimeToCheck)
{
  uint32_t markTime = millis();
  while(1)
  {
	  if(millis() - markTime > maxTimeToCheck) return(false);
	  if(check() == true) //We found new data!
	    return(true);
	  delay(1);
  }
}

//Given a register, read it, mask it, and then set the thing
void MAX30100::bitMask(uint8_t reg, uint8_t mask, uint8_t thing)
{
  // Grab current register context
  uint8_t originalContents = readRegister8(_i2caddr, reg);
  // Zero-out the portions of the register we're interested in
  originalContents = originalContents & mask;
  // Change contents
  writeRegister8(_i2caddr, reg, originalContents | thing);
}

//
// Low-level I2C Communication
//
uint8_t MAX30100::readRegister8(uint8_t address, uint8_t reg) {
  _i2cPort->beginTransmission(address);
  _i2cPort->write(reg);
  _i2cPort->endTransmission(false);
  _i2cPort->requestFrom(address, 1);   // Request 1 byte

  int tries = 0;
  while (!_i2cPort->available())
  {
    delay(1);
    if (tries++ > 200) break;
  }
  if (tries == 200) return (0); //Fail

  return (_i2cPort->read());
}

void MAX30100::writeRegister8(uint8_t address, uint8_t reg, uint8_t value) {
  _i2cPort->beginTransmission(address);
  _i2cPort->write(reg);
  _i2cPort->write(value);
  _i2cPort->endTransmission();
}

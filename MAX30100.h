/*************************************************** 
 This is a library written for the Maxim MAX30105 Optical Smoke Detector
 It should also work with the MAX30102. However, the MAX30102 does not have a Green LED.

 These sensors use I2C to communicate, as well as a single (optional)
 interrupt line that is not currently supported in this driver.
 
 Written by Peter Jansen and Nathan Seidle (SparkFun)
 BSD license, all text above must be included in any redistribution.
 *****************************************************/

#pragma once

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include <Wire.h>
#include "MAX30100_Registers.h"

#define I2C_SPEED_STANDARD        100000
#define I2C_SPEED_FAST            400000

//Define the size of the I2C buffer based on the platform the user has
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)

  //I2C_BUFFER_LENGTH is defined in Wire.H
  #define I2C_BUFFER_LENGTH BUFFER_LENGTH

#elif defined(__SAMD21G18A__)

  //SAMD21 uses RingBuffer.h
  #define I2C_BUFFER_LENGTH SERIAL_BUFFER_SIZE

#else

  //The catch-all default is 32
  #define I2C_BUFFER_LENGTH 32

#endif

class MAX30100 {
 public: 
  MAX30100(void);

  boolean begin(TwoWire &wirePort = Wire, uint32_t i2cSpeed = I2C_SPEED_STANDARD, uint8_t i2caddr = MAX30100_ADDRESS);

  uint16_t getRed(void); //Returns immediate red value
  uint16_t getIR(void); //Returns immediate IR value
  uint16_t getFIFORed(void); //Returns the FIFO sample pointed to by tail
  uint16_t getFIFOIR(void); //Returns the FIFO sample pointed to by tail
 
  // Configuration
  void softReset(void);
  void shutDown(void);
  void wakeUp(void);

  // SPO2 Mode
  // 50,100S/s 200-1600uS pulse, 16 bit
  // 167,200S/s 200-800uS pulse, 15bit
  // 400S/s 200,400uS, 14bit
  // 600,800,1000, 200uS, 13bit
  void setLEDMode(uint8_t mode);
  void setSampleRate(uint8_t sampleRate);
  void setPulseWidth(uint8_t pulseWidth);
  void setPulseAmplitudeRed(uint8_t value);
  void setPulseAmplitudeIR(uint8_t value);
  void setHighresModeEnabled(void);
  void setHighresModeDisabled(void);

  //Interrupts 
  uint8_t getINT(void);    //Returns the main interrupt group
  void enableAFULL(void);  //Enable/disable individual interrupts
  void disableAFULL(void);
  void enableTEMPRDY(void);
  void disableTEMPRDY(void);
  void enableHRRDY(void);
  void disableHRRDY(void);
  void enableSPO2RDY(void);
  void disableSPO2RDY(void); 
  
  //FIFO Configuration 
  void setFIFOAlmostFull(uint8_t samples);
  void clearFIFO(void);    //Sets the read/write pointers to zero
  
  //FIFO Reading
  uint16_t check(void);    //Checks for new data and fills FIFO
  uint8_t available(void); //Tells caller how many new samples are available (head - tail)
  void nextSample(void);   //Advances the tail of the sense array
  bool safeCheck(uint8_t maxTimeToCheck); //Given a max amount of time, check for new data

  uint8_t getWritePointer(void);
  uint8_t getReadPointer(void);

  // Die Temperature
  float readTemperature(void);
  float readTemperatureF(void);

  // Detecting ID/Revision
  uint8_t getRevisionID(void);
  uint8_t readPartID(void);  

  // Setup the IC with user selectable settings
  // The MAX30100 has many settings. By default we select:
  // powerLevel=  
  // Mode = SPO2 011 or HR
  // Sample rate = 50S/s
  // Puleswith=1600
  // SPO2 Mode:
  // -----------
  // 50,100S/s     200 - 1600uS pulse 16 it
  // 167,200S/s    200 -  800uS pulse 15bit
  // 400S/s        200 -  400uS pulse 14bit
  // 600,800,1000,        200uS pulse 13bit
  // MAX30100_IRLED_CURR_0MA  _4_4MA _7_6MA _11MA 
  // _14_2MA _17_4MA _20_8MA _24MA _27_1MA _30_6MA _33_8MA   
  // _37MA _40_2MA _43_6MA _46_8MA _50MA
  void setup(byte powerLevel = 0x0F, byte ledMode = MAX30100_MODE_HR, int sampleRate = 50, int pulseWidth = 1600, bool highresMode = false);

  // Low-level I2C communication
  uint8_t readRegister8(uint8_t address, uint8_t reg);
  void   writeRegister8(uint8_t address, uint8_t reg, uint8_t value);

 private:
  TwoWire *_i2cPort; //The generic connection to user's chosen I2C hardware
  uint8_t _i2caddr;
  uint8_t revisionID; 
  void readRevisionID();
  void bitMask(uint8_t reg, uint8_t mask, uint8_t thing);
};

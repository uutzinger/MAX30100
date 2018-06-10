/*
  This program is based on
  Optical SP02 Detection (SPK Algorithm) 
  By: Nathan Seidle @ SparkFun Electronics
  Date: October 19th, 2016
  https://github.com/sparkfun/MAX30105_Breakout

  This code was adapted to work with MAX30100 and MAX30102.
  
  It is best to attach the sensor to your finger using a rubber band or other tightening 
  device. Humans are generally bad at applying constant pressure to a thing. When you 
  press your finger against the sensor it varies enough to cause the blood in your 
  finger to flow differently which causes the sensor readings to go wonky.

  This example is based on MAXREFDES117 and RD117_LILYPAD.ino from Maxim. Their example
  was modified to work with MAX30100 and to compile under Arduino 1.8.5
  Please see license file for more info.

  Hardware Connections (Breakoutboard to Arduino):
  -5V = 5V (3.3V is allowed)
  -GND = GND
  -SDA = A4 (or SDA)
  -SCL = A5 (or SCL)
  -INT = Not connected
 
  The MAX30100 Breakout can handle 5V or 3.3V I2C logic. We recommend powering the board with 5V
  but it will also run at 3.3V.
*/

#include <Wire.h>
#include "MAX30100.h"
#include "algorithm.h"

MAX30100 sensor;

#if defined(ARDUINO_AVR_NANO)
//Arduino Uno doesn't have enough SRAM to store 100 samples of IR led data and red led data in 32-bit format
//To solve this problem, 16-bit MSB of the sampled data will be truncated. Samples become 16-bit data.
uint16_t irBuffer[100];   //infrared LED sensor data
uint16_t redBuffer[100];  //red LED sensor data
#else
uint32_t irBuffer[100];   //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data
#endif

int32_t bufferLength; //data length
int32_t spo2; //SPO2 value
int8_t  validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t  validHeartRate; //indicator to show if the heart rate calculation is valid

byte readLED = 13; //Blinks with each data read

void setup()
{
  Serial.begin(115200); // initialize serial communication at 115200 bits per second:

  //pinMode(pulseLED, OUTPUT);
  pinMode(readLED, OUTPUT);

  // Initialize sensor
  if (!sensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    while (1) {
          Serial.println(F("MAX30100 was not found. Please check wiring/power."));
    }
  }
  else  {
    Serial.println(F("MAX30100 was found."));
  }

 // Serial.println(F("Attach sensor to finger with rubber band. Press any key to start conversion"));
 // while (Serial.available() == 0) ; //wait until user presses a key
 // Serial.read();

  //SLOW
  byte ledBrightness  = 0x0F;                // MAX30100_IRLED_CURR_50MA
  byte ledMode = MAX30100_MODE_SPO2;         // Options: SPO2, HR
  int sampleRate = 50;                       // Options: 50, 100, 167, 200, 400, 600, 800, 1000,
  int pulseWidth = MAX30100_PULSEWIDTH_1600; // Options: 200, 400, 800, 1600[us] 
  bool highresMode = true;
  
  //FAST
  //byte ledBrightness  = 0x0F;               // MAX30100_IRLED_CURR_50MA
  //byte ledMode = MAX30100_MODE_SPO2;        // Options: SPO2, HR
  //int sampleRate = 1000 ;                   // Options: 50, 100, 167, 200, 400, 600, 800, 1000,
  //int pulseWidth = MAX30100_PULSEWIDTH_200; // Options: 200, 400, 800, 1600[us] 
  //bool highresMode = false;
  
  //Configure sensor with these settings
  sensor.setup(ledBrightness, ledMode, sampleRate, pulseWidth, highresMode);
  Serial.println(F("Sensor Configured."));
}

void loop()
{
  bufferLength = 100; //buffer length of 100 stores 4 seconds of samples running at 25sps

  //read the first 100 samples, and determine the signal range
  for (byte i = 0 ; i < bufferLength ; i++)
  {
    while (sensor.available() == false) //do we have new data?
    {
      // Serial.println(F("Checking for Data."));  
      sensor.check(); //Check the sensor for new data
    }
    Serial.println(F("Reading Red Data."));  
    redBuffer[i] = sensor.getRed();
    Serial.println(F("Reading IR Data."));  
    irBuffer[i]  = sensor.getIR();
    sensor.nextSample(); //We're finished with this sample so move to next sample

    // Serial.print(F("R:"));
    // Serial.print(redBuffer[i], DEC);
    // Serial.print(F(","));
    // Serial.println(irBuffer[i], DEC);
  }

  //calculate heart rate and SpO2 after first 100 samples (first 4 seconds of samples)
  //maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  //Continuously taking samples from MAX3010x.  Heart rate and SpO2 are calculated every 1 second
  while (1)
  {
    //dumping the first 25 sets of samples in the memory and shift the last sets of samples to the top
    for (byte i = 25; i < bufferLength; i++)
    {
      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25] = irBuffer[i];
    }

    //take 25 sets of samples before calculating the heart rate.
    for (byte i = bufferLength-25; i < bufferLength; i++)
    {
      while (sensor.available() == false) //do we have new data?
        sensor.check(); //Check the sensor for new data

      digitalWrite(readLED, !digitalRead(readLED)); //Blink onboard LED with every data read

      redBuffer[i] = sensor.getRed();
      irBuffer[i] = sensor.getIR();
      sensor.nextSample(); //We're finished with this sample so move to next sample

      // Send samples and calculation result to terminal program through UART
      Serial.print(F("R:"));
      Serial.print(redBuffer[i], DEC);
      Serial.print(F(","));
      Serial.print(irBuffer[i], DEC);

      //Serial.print(F(",T:"));
      //Serial.print(millis());

      //Serial.print(F(",H:"));
      //Serial.print(heartRate, DEC);

      //Serial.print(F(",B:"));
      //Serial.print(validHeartRate, DEC);

      //Serial.print(F(",O:"));
      //Serial.print(spo2, DEC);
      
	    //Serial.print(F(",V:"));
      //Serial.println(validSPO2, DEC);
	  }

    //After gathering 25 new samples recalculate HR and SP02
    //maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
  }
} 

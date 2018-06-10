/*
Arduino-MAX30100 oximetry / heart rate integrated sensor library
Copyright (C) 2016  OXullo Intersecans <x@brainrapers.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Grapher helper for the Arduino MAX30100 library

import processing.serial.*;

// NOTE: when using PULSEOXIMETER_DEBUGGINGMODE_RAW_VALUES
// set this to -1 to enable the auto range mode
final int ABSMAX = -1;
final int ABSMIN = -1;
// Adjust to the serial port. Under OSX, UNO platforms and alike are auto-detected.
final String serialPort1 = "COM3";
final String serialPort2 = "COM5";

final int WIDTH = 1600;
final int HEIGHT = 600;
final int CHANNELS = 2;
final color[] colors = {color(0, 0, 0), color(255, 0, 0), color(0, 255, 0), color(0, 0, 255)};

float[][] series1 = new float[CHANNELS][WIDTH/4];
float[][] series2 = new float[CHANNELS][WIDTH/4];
int[]     seriesT = new int[WIDTH/4];

float heartRate1 = 0;
float heartRate2 = 0;
int spO21 = 0;
int spO22 = 0;
boolean beatDetected1 = false;
boolean beatDetected2 = false;
boolean spO2Detected1 = false;
boolean spO2Detected2 = false;
boolean synced = false;
boolean portOneStarted = false;
boolean portTwoStarted = false;
int ptr1 = 0;
int ptr2 = 0;
int expectedNextPort = 1;
float sample;

Serial portOne;    // the first serial port
Serial portTwo;    // the second serial port

PrintWriter output;

void settings()
{
  size(WIDTH, HEIGHT);
}

void setup ()
{
  String attemptPort1 = serialPort1;
  for (int i=0 ; i < Serial.list().length ; ++i) {
    String port = Serial.list()[i];
    if (port.matches(".+tty\\.usbmodem.+")) {
      attemptPort1 = port;
      break;
    }
  }
    println("Opening port " + attemptPort1);
  portOne = new Serial(this, attemptPort1, 115200);
  portOne.bufferUntil(10);  // Call serial event when LF is received (text, CR, LF)
 
  String attemptPort2 = serialPort2;
  for (int i=0 ; i < Serial.list().length ; ++i) {
    String port = Serial.list()[i];
    if (port.matches(".+tty\\.usbmodem.+")) {
      attemptPort2 = port;
      break;
    }
  }
  println("Opening port " + attemptPort2);
  portTwo = new Serial(this, attemptPort2, 115200);
  portTwo.bufferUntil(10);
  
  portTwo.clear();
  portOne.clear();
  
  stroke(0);
  fill(0);
  textSize(32);
}

void draw ()
{
    background(255);
  
  stroke(30);
  
  line(0, height/2, width, height/2);

  float[] maxv1 = new float[CHANNELS];  // max of each channel
  float[] minv1 = new float[CHANNELS];  // min of each channel
  for (int s=0 ; s < CHANNELS ; ++s) {
    maxv1[s] = 0;                       // set max to minimum of sensor range
    minv1[s] = 9999999;                 // initilaize minimum above sensor range
  }    
  for (int s=0 ; s < CHANNELS ; ++s) {
    float[] samples1 = series1[s];
    // maxv = max(maxv, abs(max(samples)), abs(min(samples)));
    // maxv[s] = max(maxv[s], max(samples));
    // minv[s] = min(minv[s], min(samples));
    maxv1[s] = max(samples1);
    minv1[s] = min(samples1);
    if (ABSMAX != -1) {
      maxv1[s] = min(maxv1[s], ABSMAX);
    }
    if (ABSMIN != -1) {
      minv1[s] = max(minv1[s], ABSMIN);
    }
  }

  float[] maxv2 = new float[CHANNELS];  // max of each channel
  float[] minv2 = new float[CHANNELS];  // min of each channel
  for (int s=0 ; s < CHANNELS ; ++s) {
    maxv2[s] = 0;                       // set max to minimum of sensor range
    minv2[s] = 9999999;                 // initilaize minimum above sensor range
  }    
  for (int s=0 ; s < CHANNELS ; ++s) {
    float[] samples2 = series2[s];
    // maxv = max(maxv, abs(max(samples)), abs(min(samples)));
    // maxv[s] = max(maxv[s], max(samples));
    // minv[s] = min(minv[s], min(samples));
    maxv2[s] = max(samples2);
    minv2[s] = min(samples2);
    if (ABSMAX != -1) {
      maxv2[s] = min(maxv2[s], ABSMAX);
    }
    if (ABSMIN != -1) {
      minv2[s] = max(minv2[s], ABSMIN);
    }
  }


  //DISPLAY PORT 1
  
  // Avoids map() errors
  for (int s=0 ; s < CHANNELS ; ++s) {
    if (maxv1[s] == 0) {    maxv1[s] =  1;  }
    if (maxv2[s] == 0) {    maxv2[s] =  1;  }
  }

  for (int s=0 ; s < CHANNELS ; ++s) {
    stroke(colors[s]);

    float[] samples1 = series1[s];
    float seriesMax1 = max(samples1);
    float seriesMin1 = min(samples1);
    
    textSize(32);
    text("ch " + s + " cur:" + samples1[ptr1] + " max:" + maxv1[s] + " min:" + minv1[s], 0, 32 + 32 * s + HEIGHT/2);

    boolean maxDisplayed1 = false;
    for (int i = 0 ; i < WIDTH/4 ; ++i) {
      if (i > 0) {
        float ipy = HEIGHT - map(samples1[i-1], minv1[s], maxv1[s], 0, HEIGHT/2);
        float iy  = HEIGHT - map(samples1[i],   minv1[s], maxv1[s], 0, HEIGHT/2);
        
        if (abs(samples1[i] - seriesMax1) < 0.001 && !maxDisplayed1) {
          text("v=" + samples1[i], i*4, iy);
          maxDisplayed1 = true;
        }
          
        line(i*4 - 1, ipy, i*4, iy);
      }
    }
  }
  if (beatDetected1 == true) { text("Rate1: " + heartRate1, 0,  96+HEIGHT/2); }
  if (spO2Detected1 == true) { text("SpO21: " + spO21 + "%", 0, 128+HEIGHT/2); }
  
  
  //DISPLAY PORT 2

  // Avoids map() errors
  for (int s=0 ; s < CHANNELS ; ++s) {
    if (maxv2[s] == 0) {    maxv2[s] =  1;  }
  }

  for (int s=0 ; s < CHANNELS ; ++s) {
    stroke(colors[s]);

    float[] samples2 = series2[s];
    float seriesMax2 = max(samples2);
    float seriesMin2 = min(samples2);
    
    text("ch " + s + " cur:" + samples2[ptr2] + " max:" + maxv2[s] + " min:" + minv2[s], 0, 32 + 32 * s);

    boolean maxDisplayed2 = false;
    for (int i = 0 ; i < WIDTH/4 ; ++i) {
      if (i > 0) {
        float ipy = HEIGHT/2 - map(samples2[i-1], minv2[s], maxv2[s], 0, HEIGHT/2);
        float iy  = HEIGHT/2 - map(samples2[i],   minv2[s], maxv2[s], 0, HEIGHT/2);
        
        if (abs(samples2[i] - seriesMax2) < 0.001 && !maxDisplayed2) {
          text("v=" + samples2[i], i*4, iy);
          maxDisplayed2 = true;
        }
          
        line(i*4 - 1, ipy, i*4, iy);
      }
    }
  }
  if (beatDetected2==true) { text("Rate2: " + heartRate2,  0, HEIGHT/2+96); }
  if (spO2Detected2==true) { text("SpO22: " + spO22 + "%", 0, HEIGHT/2+128); }
}
  
void serialEvent (Serial thisPort)
{
  //println("this port " + thisPort);
  String sLine = thisPort.readStringUntil(10);
  //println("In: " + sLine);
  
  if (sLine == null) {   return;  }
  String[] sValues = split(sLine, ',');
 
  if (synced == false){
    if (thisPort == portOne){
      portOneStarted = true;
    }
    if (thisPort == portTwo){
      portTwoStarted = true;
    }
    if (portOneStarted && portTwoStarted){
      synced = true;
    }
  }


  if (synced){
    if (thisPort == portOne) {        
      if (expectedNextPort == 1){
        if (sValues[0].substring(0, 2).equals("R:")) {
          sample = float(sValues[0].substring(2));
          //println(sample);
          if (!Float.isNaN(sample)) {
            series1[0][ptr1] = sample;
          }
          sample = float(sValues[1]);
          //println(sample);
          if (!Float.isNaN(sample)) {
          series1[1][ptr1] = sample;
          }
          seriesT[ptr1] = millis();
          ptr1 = (ptr1 + 1) % (WIDTH/4);
       } // R
       for (int i=2 ; i < sValues.length ; ++i) {
         if (sValues[i].substring(0, 2).equals("H:")) {
            heartRate1 = int(sValues[i].substring(2));
         } else if (sValues[i].substring(0, 2).equals("B:")) {
            if (sValues[i].substring(2, 3).equals("1")) { beatDetected1 = true; } else {beatDetected1 = false;}
         } else if (sValues[i].substring(0, 2).equals("O:")) {
            spO21 = int(sValues[i].substring(2));
         } else if (sValues[i].substring(0, 2).equals("V:")) {
            //println(sValues[i].substring(2,3));
            if (sValues[i].substring(2, 3).equals("1")) { spO2Detected1 = true; } else {spO2Detected1 = false;}
         }
       } // for
       expectedNextPort = 2;
      }// Expected port
    } // port one

    
    if (thisPort == portTwo) {        
      if (expectedNextPort == 2){
        if (sValues[0].substring(0, 2).equals("R:")) {
          sample = float(sValues[0].substring(2));
          //println(sample);
          if (!Float.isNaN(sample)) {
            series2[0][ptr1] = sample;
          }
          sample = float(sValues[1]);
          //println(sample);
          if (!Float.isNaN(sample)) {
          series2[1][ptr1] = sample;
          }
          //seriesT[ptr1] = millis();
          ptr2 = (ptr2 + 1) % (WIDTH/4);
       } // R
       for (int i=2 ; i < sValues.length ; ++i) {
         if (sValues[i].substring(0, 2).equals("H:")) {
            heartRate2 = int(sValues[i].substring(2));
         } else if (sValues[i].substring(0, 2).equals("B:")) {
            if (sValues[i].substring(2, 3).equals("1")) { beatDetected2 = true; } else {beatDetected2 = false;}
         } else if (sValues[i].substring(0, 2).equals("O:")) {
            spO22 = int(sValues[i].substring(2));
         } else if (sValues[i].substring(0, 2).equals("V:")) {
            //println(sValues[i].substring(2,3));
            if (sValues[i].substring(2, 3).equals("1")) { spO2Detected2 = true; } else {spO2Detected2 = false;}
         }
       } // for
       expectedNextPort = 1;
      }// Expected port
    } // port two
  } // synced
} // main

//void keyPressed() {
//  if (key == 'q') {
//    ;
//  } 
//

void keyPressed() {
  if (key == 's') {
    int d = day();    // Values from 1 - 31
    int m = month();  // Values from 1 - 12
    int y = year();   // 2003, 2004, 2005, etc.
    int s = second();
    int mi = minute();
    int h = hour();
    String myName = String.valueOf(y) + "_" + String.valueOf(m) + "_" + String.valueOf(d) + "_" + String.valueOf(h) + "_" + String.valueOf(mi) + "_" + String.valueOf(s) ;
    output = createWriter("Hunt"+ myName + ".csv"); 
    for(int i=0; i < WIDTH; i ++){
      output.println(seriesT[i] + "," + series1[0][i] + "," + series1[1][i] + "," + series2[0][i] + "," + series2[1][i]);
    }
    output.flush(); // Writes the remaining data to the file
    output.close(); // Finishes the file
  }
  if (key == 'q') {
    exit();
  }
}
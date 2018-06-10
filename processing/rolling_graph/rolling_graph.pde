/*
Arduino-MAX3010x oximetry / heart rate integrated sensor library
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

// Grapher helper for the Arduino MAX3010x library

import processing.serial.*;

// NOTE: when using PULSEOXIMETER_DEBUGGINGMODE_RAW_VALUES
// set this to -1 to enable the auto range mode
final int ABSMAX = -1;
final int ABSMIN = -1;
// Adjust to the serial port. Under OSX, UNO platforms and alike are auto-detected.
// final String serialPort = "/dev/tty.usbmodemFD13131";
final String serialPort = "COM5";

final int WIDTH = 1600;
final int HEIGHT = 600;
final int CHANNELS = 2;
final color[] colors = {color(0, 0, 0), color(255, 0, 0), color(0, 255, 0), color(0, 0, 255)};


float[][] series  = new float[CHANNELS][WIDTH/4];
    int[] seriesT = new int[WIDTH/4];
float heartRate = 0;
int spO2 = 0;
boolean beatDetected = false;
boolean spO2Detected = false;
int ptr = 0;
float sample;

Serial myPort;
PrintWriter output;

void settings()
{
  size(WIDTH, HEIGHT);
}

void setup ()
{
  String attemptPort = serialPort;
  for (int i=0 ; i < Serial.list().length ; ++i) {
    String port = Serial.list()[i];
    //if (port.matches(".+tty\\.usbmodem.+")) {
    if (port.matches("COM.+")) {
      attemptPort = port;
      break;
    }
  }
  println("Opening port " + attemptPort);
  myPort = new Serial(this, attemptPort, 115200);
  myPort.bufferUntil(10); // Buffer until LF is received
  myPort.clear();
  
  stroke(0);
  fill(0);
  textSize(32);
}

void draw ()
{
  //if (beatDetected) {
  //  background(255, 200, 200);
  //  beatDetected = false;
  //} else {
    background(255);
  //}
  
  stroke(30);
  
  line(0, height/2, width, height/2);

  float[] maxv = new float[CHANNELS];  // max of each channel
  float[] minv = new float[CHANNELS];  // min of each channel
  for (int s=0 ; s < CHANNELS ; ++s) {
    maxv[s] = 0;                       // set max to minimum of sensor range
    minv[s] = 9999999;                 // initilaize minimum above sensor range
  }    
  for (int s=0 ; s < CHANNELS ; ++s) {
    float[] samples = series[s];
    // maxv = max(maxv, abs(max(samples)), abs(min(samples)));
    // maxv[s] = max(maxv[s], max(samples));
    // minv[s] = min(minv[s], min(samples));
    maxv[s] = max(samples);
    minv[s] = min(samples);
    if (ABSMAX != -1) {
      maxv[s] = min(maxv[s], ABSMAX);
    }
    if (ABSMIN != -1) {
      minv[s] = max(minv[s], ABSMIN);
    }
  }

  // Avoids map() errors
  for (int s=0 ; s < CHANNELS ; ++s) {
    if (maxv[s] == 0) {    maxv[s] =  1;  }
  }

  for (int s=0 ; s < CHANNELS ; ++s) {
    stroke(colors[s]);

    float[] samples = series[s];
    float seriesMax = max(samples);
    float seriesMin = min(samples);
    
    textSize(32);
    text("ch " + s + " cur:" + samples[ptr] + " max:" + maxv[s] + " min:" + minv[s] , 0, 32 + 32 * s);

    boolean maxDisplayed = false;
    for (int i = 0 ; i < WIDTH/4 ; ++i) {
      if (i > 0) {
        float ipy = HEIGHT - map(samples[i-1], minv[s], maxv[s], 0, HEIGHT);
        float iy  = HEIGHT - map(samples[i],   minv[s], maxv[s], 0, HEIGHT);
        
        if (abs(samples[i] - seriesMax) < 0.001 && !maxDisplayed) {
          text("v=" + samples[i], i*4, iy);
          maxDisplayed = true;
        }
          
        line(i*4 - 1, ipy, i*4, iy);
      }
    }
  }
  if (beatDetected==true) { text("Rate: " + heartRate, 0, 96); }
  if (spO2Detected==true) { text("SpO2: " + spO2 + "%", 0, 128); }
}
  
void serialEvent (Serial myPort)
{
  String sLine = myPort.readStringUntil('\n');
  
  if (sLine == null) {
    return;
  }

  //println("Incoming: " + sLine);
 
  String[] sValues = split(sLine, ',');
  
  //println(sValues);
  
  if (sValues[0].substring(0, 2).equals("R:")) {
    sample = float(sValues[0].substring(2));
    //println(sample);
    if (!Float.isNaN(sample)) {
      series[0][ptr] = sample;
    }
    sample = float(sValues[1]);
    //println(sample);
    if (!Float.isNaN(sample)) {
    series[1][ptr] = sample;
    }
    seriesT[ptr] = millis();
    ptr = (ptr + 1) % (WIDTH/4);
  }
  
  for (int i=2 ; i < sValues.length ; ++i) {
    if (sValues[i].substring(0, 2).equals("H:")) {
        heartRate = int(sValues[i].substring(2));
    } else if (sValues[i].substring(0, 2).equals("B:")) {
        if (sValues[i].substring(2, 3).equals("1")) { beatDetected = true; } else {beatDetected = false;}
    } else if (sValues[i].substring(0, 2).equals("O:")) {
        spO2 = int(sValues[i].substring(2));
    } else if (sValues[i].substring(0, 2).equals("V:")) {
        //println(sValues[i].substring(2,3));
        if (sValues[i].substring(2, 3).equals("1")) { spO2Detected = true; } else {spO2Detected = false;}
    }
  }
  // println(spO2Detected);
  // println(beatDetected);
}


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
    for(int i=0; i < WIDTH/4; i ++){
      output.println(seriesT[i] + "," + series[0][i] + "," + series[1][i]);
    }
    output.flush(); // Writes the remaining data to the file
    output.close(); // Close the file
  }
  if (key == 'q') {
    exit();
  }
}
/* 3D Printer Bed Leveler
 * Copyright (C) 2022 by Dominick Lee (http://dominicklee.com)
 *
 * See https://www.instructables.com/3D-Print-Bed-Leveling-Tool-Using-M5StickC/
 * 
 * Last Modified Jun, 2022.
 * This program is free software: you can use it, redistribute it, or modify
 * it under the terms of the MIT license (See LICENSE file for details).
 * The above copyright notice and this permission notice shall be included 
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR 
 * ANY CLAIM, DAMAGES,OR OTHER LIABILITY. YOU AGREE TO USE AT YOUR OWN RISK.
 * 
*/

#include "M5StickCPlus2.h"
#include <EEPROM.h>
#include "OneButton.h"
#include "thumbs-down.h"
#include "thumbs-up.h"

// Icons converted at: 
// http://rinkydinkelectronics.com/_t_doimageconverter565.php

#define EEPROM_SIZE 1
bool isActive = true;

int ledPin = G10;
int buzzerPin = 33;
int currentPressureLevel = 0;
int minimumRequiredPressureLevel = 20;
float errorThreshold = 6.0; //printbed is considered leveled if error goes below this (ideally 5.0-8.0)

int shiftX = 10;
int shiftY = 10;

//Object initialization
OneButton mainButton(G37, true);
OneButton rightButton(G39, true);

void setup() {
  M5.begin();
  Serial.begin(115200);       //Initialize Serial
  pinMode(ledPin, OUTPUT);    //Set up LED
  pinMode(buzzerPin, OUTPUT);
  digitalWrite (ledPin, HIGH); // turn off the LED

  M5.Lcd.setRotation(0);
  M5.Lcd.fillScreen(BLACK);

  mainButton.attachClick(mainButtonClick);
  mainButton.setDebounceMs(40);
  rightButton.attachClick(rightButtonClick);
  rightButton.setDebounceMs(25);
  
  M5.Lcd.drawRect(shiftX+5, 12, 21, 133, 0x7bef);  //show frame for progressbar
  getCalibration(); //show calibration mark

  showRightButtonHelperText("Vib on");

  //Show Calibrate instructions
  M5.Lcd.setTextColor(RED);
  M5.Lcd.setCursor(40, 230);
  M5.Lcd.printf("Calibrate");

  // Swap the colour byte order when rendering
  M5.Lcd.setSwapBytes(true);
}

void loop() {
  //poll for button press
  mainButton.tick();
  rightButton.tick();
  
  currentPressureLevel = map(analogRead(G36), 0, 4095, 0, 127); //get reading
  Serial.println(minimumRequiredPressureLevel);

  progressBar(currentPressureLevel);  //show reading on progressbar

  if (currentPressureLevel < minimumRequiredPressureLevel) {
    setLED(true);
    M5.Lcd.pushImage(60, 15, 32, 32, thumbs_down);  // Draw icon

    if (isActive) {
      analogWrite(buzzerPin, 120);
    }
    else {
      analogWrite(buzzerPin, 0);
    }
    
  } else {
    setLED(false);
    M5.Lcd.pushImage(60, 15, 32, 32, thumbs_up); // Draw icon
    analogWrite(buzzerPin, 0);
  }
  
  delay(50);
}

void printpressureLevelOnLCD(int value)
{
  M5.Lcd.setTextColor(BLUE);
  M5.Lcd.setCursor(15, 160);
  M5.Lcd.fillRect(0,0,240,20,0); 
  M5.Lcd.printf("%03d",value);
}
void rightButtonClick()
{
  switchMode();
}

void switchMode()
{
  isActive = ! isActive;

  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(85, 105);
  M5.Lcd.printf("        ");
  M5.Lcd.setCursor(85, 105);

  if (isActive) {
    showRightButtonHelperText("Vib on");
  } else {
    showRightButtonHelperText("Vib off");
  }
}

void showRightButtonHelperText(String textToPrint)
{
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(85, 105);
  M5.Lcd.printf("        ");
  M5.Lcd.setCursor(85, 105);

  // TODO: use the texteToPrint variable
  if (isActive) {
    M5.Lcd.printf("Vib on");
  } else {
    M5.Lcd.printf("Vib off");
  }
  
}

void mainButtonClick()
{
  updateCalibration(currentPressureLevel);
}

float getPercentError(float approx, float exact)
{
  return (abs(approx-exact)/exact)*100;
}

void updateCalibration(int value)
{ 
  EEPROM.write(0, value);  // save in EEPROM
  EEPROM.commit();
  // clear old line
  M5.Lcd.drawLine(shiftX+1, 15+(127-minimumRequiredPressureLevel), shiftX+4, 15+(127-minimumRequiredPressureLevel), BLACK);
  M5.Lcd.drawLine(shiftX+26, 15+(127-minimumRequiredPressureLevel), shiftX+29, 15+(127-minimumRequiredPressureLevel), BLACK);
  // set new line
  minimumRequiredPressureLevel = value; //set global
  M5.Lcd.drawLine(shiftX+1, 15+(127-minimumRequiredPressureLevel), shiftX+4, 15+(127-minimumRequiredPressureLevel), 0x7bef);
  M5.Lcd.drawLine(shiftX+26, 15+(127-minimumRequiredPressureLevel), shiftX+29, 15+(127-minimumRequiredPressureLevel), 0x7bef);
}

void getCalibration()
{
  minimumRequiredPressureLevel = EEPROM.read(0);  // retrieve calibration in EEPROM
  // set new line
  M5.Lcd.drawLine(shiftX+1, 15+(127-minimumRequiredPressureLevel), shiftX+4, 15+(127-minimumRequiredPressureLevel), 0x7bef);
  M5.Lcd.drawLine(shiftX+26, 15+(127-minimumRequiredPressureLevel), shiftX+29, 15+(127-minimumRequiredPressureLevel), 0x7bef);
}

void setLED(bool isON)
{
  digitalWrite (ledPin, !isON); // set the LED
}

void progressBar(int value)
{
  // Value is expected to be in range 0-127
  for (int i = 0; i <= value; i++) {  //draw bar
    M5.Lcd.fillRect(shiftX+8, 142-i, 15, 1, rainbow(i));
  }
  for (int i = value+1; i <= 128; i++) {  //clear old stuff
    M5.Lcd.fillRect(shiftX+8, 142-i, 15, 1, BLACK);
  }
  // print numeric value below the progress bar
  M5.Lcd.fillRect(shiftX+5,160,50,10,0);
  M5.Lcd.drawNumber(currentPressureLevel, shiftX+5, 160);
}

unsigned int rainbow(int value)
{
  // Value is expected to be in range 0-127
  // The value is converted to a spectrum colour from 0 = blue through to red = blue
  //int value = random (128);
  byte red = 0; // Red is the top 5 bits of a 16 bit colour value
  byte green = 0;// Green is the middle 6 bits
  byte blue = 0; // Blue is the bottom 5 bits

  byte quadrant = value / 32;

  if (quadrant == 0) {
    blue = 31;
    green = 2 * (value % 32);
    red = 0;
  }
  if (quadrant == 1) {
    blue = 31 - (value % 32);
    green = 63;
    red = 0;
  }
  if (quadrant == 2) {
    blue = 0;
    green = 63;
    red = value % 32;
  }
  if (quadrant == 3) {
    blue = 0;
    green = 63 - 2 * (value % 32);
    red = 31;
  }
  return (red << 11) + (green << 5) + blue;
}

/* A'Grip - Fabrikarium - Humanlab Saint-Pierre - 2024
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
String versionNumber = "1.0.0";
float errorThreshold = 6.0; //printbed is considered leveled if error goes below this (ideally 5.0-8.0)

bool rightHandIsHold = false; // The right hand FSR hold status
bool usesDuoMode = false; // When true, the left hand state is only checked when right hand is hold too

int currentPressureLevel = 0; // The left hand pressure level
int minimumRequiredPressureLevel = 0; // Will be reset after calibration 

int ledPin = G10;
int mainButtonPin = G37;
int rightButtonPin = G39;
int leftButtonPin = 35;
int vibrationPin = 33;
int rightHandPin = G26;
int fsrPin = G36;

// The 5 vibration step levels from disabled to max
int vibrationLevels[5] = {0, 120, 160, 200, 255};
int currentVibrationLevel = 0; // 4 = maximum

OneButton mainButton(mainButtonPin, true);
OneButton rightButton(rightButtonPin, true);
OneButton leftButton(leftButtonPin, true);

void setup() {
  // Board global initialization
  M5.begin();
  M5.Lcd.setRotation(0);
  M5.Lcd.fillScreen(BLACK);
  Serial.begin(115200);

  // Pins initialization
  pinMode(ledPin, OUTPUT);    //Set up LED
  pinMode(vibrationPin, OUTPUT);
  pinMode(rightHandPin, INPUT);

  digitalWrite (ledPin, HIGH);
  
  
  // Buttons event handlers initialization
  mainButton.attachClick(mainButtonClick);
  mainButton.setDebounceMs(40);
  leftButton.attachClick(leftButtonClick);
  leftButton.setDebounceMs(40);
  rightButton.attachClick(rightButtonClick);
  rightButton.setDebounceMs(40);
  
  M5.Lcd.drawRect(15, 12, 21, 133, 0x7bef);  //show frame for progressbar
  getCalibration(); //show calibration mark

  //Show Calibrate instructions
  M5.Lcd.setTextColor(BLUE);
  M5.Lcd.setCursor(40, 230);
  M5.Lcd.printf("Calibrate");

  // Swap the colour byte order when rendering
  M5.Lcd.setSwapBytes(true);

  showVersionNumber();
}

void loop() {
  // Buttons press detection
  mainButton.tick();
  rightButton.tick();
  leftButton.tick();
  
  showRightHandDetectionType();
  showVibrationMode();
  showBatteryLevel();
  showRighHandHoldStatus();

  if (usesDuoMode) {
    rightHandIsHold = digitalRead(rightHandPin);
  }
  
  currentPressureLevel = map(analogRead(fsrPin), 0, 4095, 0, 127);
  Serial.println(minimumRequiredPressureLevel);

  progressBar(currentPressureLevel);

  if (currentPressureLevel < minimumRequiredPressureLevel) {
    setLED(true);
    M5.Lcd.pushImage(60, 25, 32, 32, thumbs_down);  // Draw icon

    if (currentVibrationLevel > 0 && isRightHandHold()) {
      analogWrite(vibrationPin, getVibrationLevelRawValue());
    }
    else {
      analogWrite(vibrationPin, 0);
    }
    
  } else {
    setLED(false);
    M5.Lcd.pushImage(60, 25, 32, 32, thumbs_up); // Draw icon
    analogWrite(vibrationPin, 0);
  }
  
  delay(50);
}

/*
 * BUTTON CLICK HANDLERS
 */
void mainButtonClick()
{
  updateCalibration(currentPressureLevel);
}

void rightButtonClick()
{
  switchRightHandDetectionMode();
}

void leftButtonClick()
{
  increaseVibrationLevel();
}

void switchRightHandDetectionMode()
{
  usesDuoMode = ! usesDuoMode;
}

void setRightHandHoldStatus(bool status)
{
  rightHandIsHold = status;
}

void showRightHandDetectionType()
{
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.setTextSize(1);

  // Draw a rectangle to erase previous text
  M5.Lcd.setCursor(65, 105);
  M5.Lcd.fillRect(60, 100, 70, 20, BLACK);

  M5.Lcd.setCursor(65, 105);
  M5.Lcd.printf(usesDuoMode ? "DUO" : "SOLO");
}

void showVibrationMode()
{
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.setTextSize(1);

  // Draw a rectangle to erase previous text
  M5.Lcd.setCursor(10, 200);
  M5.Lcd.fillRect(10, 200, 70, 20, BLACK);

  M5.Lcd.setCursor(10, 200);

  String message = "Vib " + String(getVibrationLevelDisplayValue());
  M5.Lcd.printf("%s\n", message.c_str());
}

void showRighHandHoldStatus()
{
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.setTextSize(1);

  // Draw a rectangle to erase previous text
  M5.Lcd.setCursor(65, 135);
  M5.Lcd.fillRect(60, 130, 70, 20, BLACK);

  if (usesDuoMode) {
    M5.Lcd.setCursor(65, 135);
    if (isRightHandHold()) {
      M5.Lcd.setTextColor(GREEN);
      M5.Lcd.printf("PRESSED");
    } else {
      M5.Lcd.setTextColor(RED);
      M5.Lcd.printf("NOT PRESS.");
    }
  }
}

void showBatteryLevel()
{
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.setTextSize(1);

  // Draw a rectangle to erase previous text
  M5.Lcd.setCursor(70, 200);
  M5.Lcd.fillRect(70, 200, 40, 20, BLACK);

  M5.Lcd.setCursor(70, 200);
 
  M5.Display.printf("Bat %d %%", getBatteryLevel());
}

void showVersionNumber()
{
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(1);

  // Draw a rectangle to erase previous text
  M5.Lcd.setCursor(40, 2);

  M5.Lcd.printf("Ver. Left %s\n", versionNumber.c_str());
}

int getBatteryLevel()
{
  float batteryPercentage = map(M5.Power.getBatteryVoltage(), 3000, 4200, 0, 100);
  if (batteryPercentage > 100) batteryPercentage = 100;
  if (batteryPercentage < 0) batteryPercentage = 0;

  return (int) batteryPercentage;
}

bool isRightHandHold() {
  return rightHandIsHold || ! usesDuoMode;
}

// The vibration raw value (from 0 to 255)
int getVibrationLevelRawValue() {
  return vibrationLevels[currentVibrationLevel];
}

// The vibration display value (0, 1, 2, 3 or 4)
int getVibrationLevelDisplayValue() {
  return currentVibrationLevel;
}

void increaseVibrationLevel() {
  currentVibrationLevel ++;
  if (currentVibrationLevel > 4) {
    currentVibrationLevel = 0;
  }
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
  M5.Lcd.drawLine(16, 15+(127-minimumRequiredPressureLevel), 14, 15+(127-minimumRequiredPressureLevel), BLACK);
  M5.Lcd.drawLine(36, 15+(127-minimumRequiredPressureLevel), 39, 15+(127-minimumRequiredPressureLevel), BLACK);
  // set new line
  minimumRequiredPressureLevel = value; //set global
  M5.Lcd.drawLine(11, 15+(127-minimumRequiredPressureLevel), 14, 15+(127-minimumRequiredPressureLevel), 0x7bef);
  M5.Lcd.drawLine(36, 15+(127-minimumRequiredPressureLevel), 39, 15+(127-minimumRequiredPressureLevel), 0x7bef);
}

void getCalibration()
{
  minimumRequiredPressureLevel = EEPROM.read(0);  // retrieve calibration in EEPROM
  // set new line
  M5.Lcd.drawLine(11, 15+(127-minimumRequiredPressureLevel), 14, 15+(127-minimumRequiredPressureLevel), 0x7bef);
  M5.Lcd.drawLine(36, 15+(127-minimumRequiredPressureLevel), 39, 15+(127-minimumRequiredPressureLevel), 0x7bef);
}

void setLED(bool isON)
{
  digitalWrite (ledPin, !isON); // set the LED
}

void progressBar(int value)
{
  // Value is expected to be in range 0-127
  for (int i = 0; i <= value; i++) {  //draw bar
    M5.Lcd.fillRect(18, 142-i, 15, 1, rainbow(i));
  }
  for (int i = value+1; i <= 128; i++) {  //clear old stuff
    M5.Lcd.fillRect(18, 142-i, 15, 1, BLACK);
  }
  // print numeric value below the progress bar
  M5.Lcd.setTextColor(CYAN);
  M5.Lcd.fillRect(25,160,50,10,0);
  M5.Lcd.drawNumber(currentPressureLevel, 25, 160);
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

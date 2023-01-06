/***********************************************************************
  Medication tracker program
  By Gerald Sears
  This program uses a SparkFun Redboard with Qwiic accessories to track 
  and display if/when medications are taken, both for morning and 
  evenings.

  Hardware is SparkFun(SF) Redboard Qwiic, SF Qwiic Button x2(one green, 
  one blue), SF Qwiic RTC RV-8803, and SF Qwiic OpenLog.  All connected 
  via the RB's Qwiic port daisy-chained.

  I set the green button up using the i2c address setup in the SparkFun 
  Qwiic button example libary Change I2C address (example 5).  The blue 
  button was left on the default I2C address. 
************************************************************************/

#include <Wire.h>
#include "SparkFun_Qwiic_OpenLog_Arduino_Library.h"  //Get the library for Qwiic Openlog here: https://learn.sparkfun.com/tutorials/qwiic-openlog-hookup-guide
#include <SparkFun_RV8803.h> //Get the library here:http://librarymanager/All#SparkFun_RV-8803
#include <SparkFun_Qwiic_Button.h> //Get the libary here: https://learn.sparkfun.com/tutorials/sparkfun-qwiic-button-hookup-guide

RV8803 rtc; //Initialize RTC
OpenLog myLog; //Create log instance
QwiicButton morningButton; 
QwiicButton eveningButton; 

uint8_t brightnessPress = 200;  //The brightness to set the LED to when the button is pushed
                                //Can be any value between 0 (off) and 255 (max)
uint8_t brightnessOn = 100; // The brightness to set the LED to when the button is ON
                            // Can be any value between 0 (off) and 255(max)

// Set Alarm times {start hour, end hour}
int morningMedAlarm [2] = {6, 13};
int eveningMedAlarm [2] = {16, 24};

int currentHour;
int takenMorning = 0;
int hourMorning;
int takenEvening = 0;
int hourEvening;
int medTime = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin();  //Join i2c bus
  Wire.setClock(400000); //Go super fast

  if (rtc.begin() == false)
  {
    Serial.println("Device not found. Please check wiring. Freezing.");
    while(1);
  }
  Serial.println("RTC online!");
  rtc.setEVIEventCapture(RV8803_ENABLE); //Enables the Timestamping function
  rtc.setEVIDebounceTime(EVI_DEBOUNCE_256HZ); //Debounce the button with a 3.9 ms(256 Hz) sampling period, other options are EVI_DEBOUNCE_NONE, EVI_DEBOUNCE_64HZ, and EVI_DEBOUNCE_8HZ
  //rtc.setEVIEdgeDetection(RISING_EDGE); // Uncomment to set event detection to button release instead of press
  
  rtc.updateTime();
  rtc.clearInterruptFlag(FLAG_EVI);
  String currentDate = rtc.stringDateUSA(); //Get the current date in mm/dd/yyyy format (we're weird)
  //String currentDate = rtc.stringDate(); //Uncomment this line to get the current date in dd/mm/yyyy format
  String timestamp = rtc.stringTimestamp();

  Serial.print("System Start ");
  Serial.print(currentDate);
  Serial.print(" ");
  Serial.println(timestamp);
  
  myLog.begin(); //Open connection to OpenLog (no pun intended)
  myLog.append("medLog.txt");
  myLog.print("Log Started: ");
  myLog.print(currentDate);
  myLog.print(" ");
  myLog.println(timestamp);
  myLog.syncFile();

  //check if the buttons will acknowledge over I2C
  //connect to Qwiic button at address 0x5B
  if (morningButton.begin(0x5B) == false){
    Serial.println("Button 1 did not acknowledge! Freezing.");
    while(1);
  }
  //connect to Qwiic button at default address, 0x6F
  if (eveningButton.begin() == false) {
    Serial.println("Button 2 did not acknowledge! Freezing.");
    while(1);
  }
  Serial.println("Both buttons acknowledged.");
  currentHour = rtc.getHours();
}

int buttonPressChk() {
  if (morningButton.isPressed() == true) {
    morningButton.LEDon(brightnessPress);
    Serial.println("Morning Button pressed!");
    while(morningButton.isPressed() == true)
      delay(10);
    morningButton.LEDoff();
    return 1;
  }
  else if(eveningButton.isPressed() == true) {
    eveningButton.LEDon(brightnessPress);
    Serial.println("Evening Button pressed!");
    while(eveningButton.isPressed() == true)
      delay(10);
    eveningButton.LEDoff();
    return 2;
  }
  else {
    return 0;
  }
}

void loop() {
  int nowHour = rtc.getHours();
  int isPM = rtc.isPM();
  if(isPM == true) {
    nowHour = nowHour + 12;
  }
  //Serial.println(nowHour);
  //String currentTime = rtc.stringTime();
  //Serial.println(currentTime);

  // Reset taken flags
  if(nowHour == 0 && rtc.getMinutes() == 0) {
    takenMorning = 0;
  }
  if(nowHour == 12 && rtc.getMinutes() == 0) {
    takenEvening = 0;
  }

  // Check if it is time to take morning or evening meds (1 morning, 2 evening, 0 not time)
  if(nowHour >= morningMedAlarm[0] && nowHour < morningMedAlarm[1]) {
    medTime = 1;
  }
  else if(nowHour > eveningMedAlarm[0] && nowHour < eveningMedAlarm[1]) {
    medTime = 2;
  }
  else {
    medTime = 0;
  }

/*****
    Set serials below to logs 
*****/
  // If it is time to take meds and meds have not been taken light the appropriate button
  if(medTime == 1 && takenMorning == 0) {
    Serial.println("It's Morning Med time!");
    morningButton.LEDon(brightnessOn);
    eveningButton.LEDoff();
    // If morning button pressed log it and set takenMorning to 1
    if(buttonPressChk() == 1){
      takenMorning = 1;
      hourMorning = nowHour;
      if (rtc.updateTime() == true) //Updates the time variables from RTC 
      {
        String currentDate = rtc.stringDateUSA(); //Get the current date in mm/dd/yyyy format (we're weird)
        //String currentDate = rtc.stringDate(); //Get the current date in dd/mm/yyyy format
        String currentTime = rtc.stringTime(); //Get the time
        Serial.print("Morning Meds taken @ ");
        Serial.print(currentDate);
        Serial.print(" ");
        Serial.println(currentTime);
      }
      else
      {
        Serial.print("RTC read failed");
      }  
    }
  }
  else if(medTime == 2 && takenEvening == 0) {
    Serial.println("It's Evening Med time!");
    morningButton.LEDoff();
    eveningButton.LEDon(brightnessOn);
    // If Evening button pressed log it and set takenEvening to 1
    if(buttonPressChk() == 1){
      takenEvening = 1;
      hourEvening = nowHour;
      if (rtc.updateTime() == true) //Updates the time variables from RTC 
      {
        String currentDate = rtc.stringDateUSA(); //Get the current date in mm/dd/yyyy format (we're weird)
        //String currentDate = rtc.stringDate(); //Get the current date in dd/mm/yyyy format
        String currentTime = rtc.stringTime(); //Get the time
        Serial.print("Evening Meds taken @ ");
        Serial.print(currentDate);
        Serial.print(" ");
        Serial.println(currentTime);
      }
      else
      {
        Serial.print("RTC read failed");
      }  
    }
  }
  else {
    // > Add functionality to light when pressed and flash m/e button pressed and if meds have been taken.
    morningButton.LEDoff();
    eveningButton.LEDoff();
  }
  delay(20);
}

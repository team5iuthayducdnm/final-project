#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time.h"
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

#define SENSOR_ENTRY_1 34
#define SENSOR_ENTRY_2 35
#define SERVO_ENTRY 13

#define SENSOR_EXIT_1 32
#define SENSOR_EXIT_2 33
#define SERVO_EXIT 12 

#define SENSOR_SLOT_1 14
#define SENSOR_SLOT_2 27
#define SENSOR_SLOT_3 26
#define SENSOR_SLOT_4 25

#define SIREN 19

#define SENSOR_FIRE 2
#define ZERO_INDEX 999

/*====================ALL FUNCTION=====================*/
void sendToGoogleSheets(int slot, String isEmpty);
void sendWarningToGoogleSheets(String fire);
String getDateString();
String getTimeString();
void sirenActive(int quantity);
void printTextByLCD(int col, int row, String text);
void printSlot();
void setSensorSlotValue(int slot, int value);
/*=====================================================*/


// ARRAY PIN
const int SENSOR_PINS[10] = {
    ZERO_INDEX,
    SENSOR_SLOT_1, SENSOR_SLOT_2, SENSOR_SLOT_3, SENSOR_SLOT_4,
    SENSOR_FIRE,
    SENSOR_ENTRY_1, SENSOR_ENTRY_2, SENSOR_EXIT_1, SENSOR_EXIT_2
};

// ARRAY CURREN ALL SENSOR VALUE
int currentSensorValues[10] = {ZERO_INDEX};
int lastSensorValues[10] = {ZERO_INDEX, 1, 1, 1, 1, 1, 1, 1, 1, 1};

// LCD
LiquidCrystal_I2C lcd(0X27, 16, 2);

// WIFI
const char *ssid = "KIETPC";
const char *password = "hihihihi";

// GOOGLE_APP_SCRIPT
const char* serverName = "https://script.google.com/macros/s/AKfycbzxgrm6yITSK0M8oj4vbL0C4Q5gOwkRx4t9pbaxM5Ya--FOQrv8mUGPXqlK6n4rs_wfIQ/exec";

// TIME
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600;
const int   daylightOffset_sec = 0;


// SERVO
Servo servoEntry;
Servo servoExit;

// SLOT VALUES
int sensorSlot1Value = 1;
int sensorSlot2Value = 1;
int sensorSlot3Value = 1;
int sensorSlot4Value = 1;

// TOTAL_SLOT_EMPTY
int numberOfEmptySlots = 4;

// BUZZER
boolean buzzerState = false;

void setup() {
  Serial.begin(115200);
  
  // WIFI
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");
  
  // TIME
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Is Starting .....");
  
  // SENSOR
  pinMode(SENSOR_ENTRY_1, INPUT);
  pinMode(SENSOR_ENTRY_2, INPUT);
  pinMode(SENSOR_EXIT_1, INPUT);
  pinMode(SENSOR_EXIT_2, INPUT);
  pinMode(SENSOR_SLOT_1, INPUT);
  pinMode(SENSOR_SLOT_2, INPUT);
  pinMode(SENSOR_SLOT_3, INPUT);
  pinMode(SENSOR_SLOT_4, INPUT);
  pinMode(SENSOR_FIRE, INPUT);
  
  // SERVO
  servoEntry.attach(SERVO_ENTRY);
  servoEntry.write(0);
  servoExit.attach(SERVO_EXIT);
  servoExit.write(90);

  // VALUES
  sendToGoogleSheets(1, "TRUE");
  sendToGoogleSheets(2, "TRUE");
  sendToGoogleSheets(3, "TRUE");
  sendToGoogleSheets(4, "TRUE");
  sendWarningToGoogleSheets("FALSE");

  // SIREN
  pinMode(SIREN, OUTPUT);
  sirenActive(3);
  printSlot();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    
    /*==================READ SENSOR VALUES==================*/
    for(int i = 1; i <= 9; i++){
       currentSensorValues[i] = digitalRead(SENSOR_PINS[i]);
    }
    for(int i = 1; i <= 9; i++){
       Serial.print("Sensor ");
       Serial.print(i);
       Serial.print(": ");
       Serial.println(currentSensorValues[i]);
    }
    /*=====================================================*/ 

    if (currentSensorValues[5] == 0) {
        if (!buzzerState) {
            digitalWrite(SIREN, HIGH);
            buzzerState = true;
            servoEntry.write(90);
            servoExit.write(0);
            sendWarningToGoogleSheets("TRUE");
        }
    } else {
        if (buzzerState) {
            digitalWrite(SIREN, LOW);
            buzzerState = false;
            servoEntry.write(0);
            servoExit.write(90);
            sendWarningToGoogleSheets("FALSE");
        }
        boolean checkDupGate = true;
        for(int i = 6; i <= 9; i++){
          if(currentSensorValues[i] != lastSensorValues[i]){
            checkDupGate = false;
          }
        }
        if(!checkDupGate){
          /*=====================ENTRY CONTROL====================*/
          if(currentSensorValues[6] && currentSensorValues[7]){
            servoEntry.write(0);
          } else if(currentSensorValues[6] == 0 && 
                   (currentSensorValues[7] == 0 || currentSensorValues[7])){
            servoEntry.write(90);
            (currentSensorValues[6] == 0 && currentSensorValues[7]) ? sirenActive(1) : void();
          }
    
          /*=====================================================*/ 
          
          /*=====================EXIT CONTROL====================*/    
          if(currentSensorValues[8] && currentSensorValues[9] ){
            servoExit.write(90);
          } else if(currentSensorValues[8] == 0 && 
                   (currentSensorValues[9] == 0 || currentSensorValues[9])){
            servoExit.write(0);
            (currentSensorValues[8] == 0 && currentSensorValues[9]) ? sirenActive(1) : void();
          }
          /*=====================================================*/
        }
        
        /*===================SLOT CONTROL===================*/
        boolean arrCheckDupSlotAndFire[5] = {true, true, true, true, true};
        boolean checkDupArr = true;
        for(int i = 1; i <= 4; i++){
          if(currentSensorValues[i] != lastSensorValues[i]){
            arrCheckDupSlotAndFire[i] = false;
            checkDupArr = false;
          }
        }
        
        if(!checkDupArr){
          printSlot();
          for(int i = 1; i <= 4; i++){
            if(!arrCheckDupSlotAndFire[i]){
              if(currentSensorValues[i] == 0){
                  setSensorSlotValue(i, 0);
                  numberOfEmptySlots -=1;
                  sirenActive(1);
                  sendToGoogleSheets(i, "FALSE");
              } else {
                  setSensorSlotValue(i, 1);
                  numberOfEmptySlots +=1;
                  sirenActive(1);
                  sendToGoogleSheets(i, "TRUE");
              } 
            }
          }
          printSlot();
        }
        /*=====================================================*/
        
        /*====================ASSIGN ARRAY CONTROL=============*/
        for(int i = 1; i <= 9; i++){
          lastSensorValues[i] = currentSensorValues[i];
        }
        /*=====================================================*/
    }
  } else {
    Serial.println("WiFi not connecting!");
  }
  delay(500);
}

/*==================GGSHEET FUNCTION===================*/
void sendToGoogleSheets(int slot, String isEmpty) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Send request POST to Google Apps Script
    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    // Create JSON
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["time"] = getTimeString();
    jsonDoc["date"] = getDateString();
    jsonDoc["slot"] = slot;
    jsonDoc["isEmpty"] = isEmpty;
    
    String jsonData;
    serializeJson(jsonDoc, jsonData);

    // Send data
    int httpResponseCode = http.POST(jsonData);
    
    Serial.println("Send data success !!!");
    http.end();
  } else {
    Serial.println("WiFi not connecting!");
  }
}

void sendWarningToGoogleSheets(String fire) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    StaticJsonDocument<200> jsonDoc;
    jsonDoc["fire"] = fire;
    
    String jsonData;
    serializeJson(jsonDoc, jsonData);

    int httpResponseCode = http.POST(jsonData);
    
    Serial.println("Send data success !!!");
    http.end();
  } else {
    Serial.println("WiFi not connecting!");
  }
}

String getDateString(){
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
      Serial.println("Cannot get date");
  }
  char dateStr[11];
  strftime(dateStr, sizeof(dateStr), "%Y/%m/%d", &timeinfo);
  return String(dateStr);
}

String getTimeString(){
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
      Serial.println("Cannot get hour");
  }
  char timeStr[9];
  strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

  return String(timeStr);
}
/*=====================================================*/

/*===================SIREN FUNCTION====================*/
void sirenActive(int quantity){
  digitalWrite(SIREN, LOW);
  for(int i = 0; i <= quantity - 1; i++){
    digitalWrite(SIREN, HIGH);
    delay(200);
    digitalWrite(SIREN, LOW);
    delay(200);
  }
}
/*=====================================================*/

/*=====================LCD FUNCTION====================*/
void printTextByLCD(int col, int row, String text){
  lcd.setCursor(col, row);
  lcd.print(text);
}

void printSlot(){
  lcd.clear();
  lcd.setCursor(1,1);
  lcd.print("numOfEmpty: ");
  lcd.print(numberOfEmptySlots);
  lcd.setCursor(0, 0);
  lcd.print("1:");
  String slot1StringValue = sensorSlot1Value == 1 ? "E" : "F";
  lcd.print(slot1StringValue);
  
  lcd.setCursor(4, 0);
  lcd.print("2:");
  String slot2StringValue = sensorSlot2Value == 1 ? "E" : "F";
  lcd.print(slot2StringValue);
  
  lcd.setCursor(8, 0);
  lcd.print("3:");
  String slot3StringValue = sensorSlot3Value == 1 ? "E" : "F";
  lcd.print(slot3StringValue);
  
  lcd.setCursor(12, 0);
  lcd.print("4:");
  String slot4StringValue = sensorSlot4Value == 1 ? "E" : "F";
  lcd.print(slot4StringValue);
}

void setSensorSlotValue(int slot, int value){
  switch(slot){
    case 1: {
      sensorSlot1Value = value;
      break;
    }
    case 2: {
      sensorSlot2Value = value;
      break;
    }
    case 3: {
      sensorSlot3Value = value;
      break;
    }
    case 4: {
      sensorSlot4Value = value;
      break;
    }
  }
}
/*=====================================================*/

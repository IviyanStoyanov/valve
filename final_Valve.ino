#include "BluetoothSerial.h"
#include <SPI.h>
#include <mySD.h>
#include <RTClib.h>
#include <Adafruit_I2CDevice.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` and enable it
#endif

BluetoothSerial SerialBT;

RTC_DS3231 rtc;

ext::File myFile;
const int chipSelect = 5;

int valvePins[8] = {2, 4, 15, 16, 17, 32, 34, 35};
int lastValveState[8];

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Bluetooth
  SerialBT.begin("ESP32test");
  
  //SD модул
  pinMode(SS, OUTPUT);
  if (!SD.begin(5)) {
    Serial.println("Problem with the sd card");
    return;
  }
  Serial.println("the sd card is ready");

  // RTC модул
  if (! rtc.begin()) {
    Serial.println("Problem with the RTC module");
    Serial.flush();
    while (1);
  }
  else
  {
    Serial.println("the RTC module is ready");
  }

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  //pins
  for (int i = 0; i < 8; i++) {
    pinMode(valvePins[i], INPUT);
    lastValveState[i] = digitalRead(valvePins[i]);
  }
}

void loop() 
{

  int valveHour, valveMinute, valveSecond;
  //RTC start
  for (int i = 0; i < 8; i++) 
  {
      int currentValveState = digitalRead(valvePins[i]);

      if (currentValveState != lastValveState[i]) 
      {
          DateTime now = rtc.now();

          if (currentValveState == HIGH) 
          {
              myFile = SD.open("times.txt", FILE_WRITE);
              if (myFile) {
              myFile.print("valve: ");
              myFile.print(valvePins[i]);
               }else{
                Serial.println("error opening times.txt");
              }
              delay(1000);
              valveHour = now.hour();
              valveMinute = now.minute();
              valveSecond = now.second();
          } 
          else 
            {
             
              if (myFile) {
              myFile.print(" ");
              myFile.print(now.day());
              myFile.print("/");
              myFile.print(now.month());
              myFile.print(" ");
              myFile.print("from");
              myFile.print(" ");
              myFile.print(valveHour);  
              myFile.print(":");
              myFile.print(valveMinute);
              myFile.print(":");
              myFile.print(valveSecond);
              myFile.print(" ");
              myFile.print("to");
              myFile.print(" ");
              myFile.print(now.hour());
              myFile.print(":");
              myFile.print(now.minute());
              myFile.print(":");
              myFile.println(now.second() + 1);
              myFile.close();
              }else{
                Serial.println("error opening times.txt");
              }
              
            }

          delay(100);
          lastValveState[i] = currentValveState;
       }
  }
  //RTC end
  
  //bluetooth start
  if (SerialBT.available()) 
  {
    if (SerialBT.read() == 'd')  // Compare with a character literal
    {
      if (SerialBT.read() == 'a' && SerialBT.read() == 't' && SerialBT.read() == 'a')  // Check for the complete string "data"
      {
        myFile = SD.open("times.txt", FILE_READ);
        Serial.println("opened");
        SerialBT.println("received");
        if (myFile) 
        {
          while (myFile.available()) {
            char c = myFile.read();
            SerialBT.write(c);
          }
          myFile.close();
          Serial.println("closed");
        } else{
           Serial.println("error opening file");
        }
        delay(1000);
      }
    } 
    else {
      SerialBT.print("invalid command");
    }
  }
  //bluetooth end

  delay(20);
}

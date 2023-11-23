#include "BluetoothSerial.h"
#include <SPI.h>
#include <mySD.h>
#include <RTClib.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` and enable it
#endif

BluetoothSerial SerialBT;

RTC_PCF8523 rtc;

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
    Serial.println("проблем със sd картата");
    return;
  }
  Serial.println("sd картата е готова");

  // RTC модул
  if (! rtc.begin()) {
    Serial.println("проблем с RTC модулът");
    Serial.flush();
    while (1);
  }
  else
  {
    Serial.println("RTC модулът е готов");
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
              Serial.print("valve: ");
              Serial.print(valvePins[i]);
              delay(1000);
              valveHour = now.hour();
              valveMinute = now.minute();
              valveSecond = now.second();
          } 
          else 
            {
              Serial.print(" ");
              Serial.print(now.day());
              Serial.print("/");
              Serial.print(now.month());
              Serial.print(" ");
              Serial.print("от");
              Serial.print(" ");
              Serial.print(valveHour);
              Serial.print(":");
              Serial.print(valveMinute);
              Serial.print(":");
              Serial.print(valveSecond);
              Serial.print(" ");
              Serial.print("до");
              Serial.print(" ");
              Serial.print(now.hour());
              Serial.print(":");
              Serial.print(now.minute());
              Serial.print(":");
              Serial.println(now.second() + 1);
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
        myFile = SD.open("test.txt", FILE_READ);
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
        } 
        Serial.println("bluetooth true");
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

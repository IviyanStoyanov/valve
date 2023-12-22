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

int valvePins[8] = {0, 4, 15, 16, 17, 25, 26, 33};
int lastValveState[8];
long valveOpenStartTime[8];  // Track individual valve open time

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Bluetooth
  SerialBT.begin("ESP32test");

  // SD module
  pinMode(SS, OUTPUT);
  if (!SD.begin(5)) {
    Serial.println("Problem with the sd card");
    return;
  }
  Serial.println("the sd card is ready");

  // RTC module
  if (!rtc.begin()) {
    Serial.println("Problem with the RTC module");
    Serial.flush();
    while (1);
  } else {
    Serial.println("the RTC module is ready");
  }

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // pins

  for (int i = 0; i < 8; i++) {
    pinMode(valvePins[i], INPUT);
  }
}

void loop() {
  for (int i = 0; i < 8; i++) {
    int currentValveState = digitalRead(valvePins[i]);

    if (currentValveState != lastValveState[i]) {
      DateTime now = rtc.now();

      if (currentValveState == LOW) {
        // Valve opened
        valveOpenStartTime[i] = millis();
      } else {
        // Valve closed
        long valveOpenTime = millis() - valveOpenStartTime[i];

        if (valveOpenTime >= 3000) {
            Serial.print(valvePins[i]);
            Serial.println(" Open" );
                                           char timeBuffer[12];
            sprintf(timeBuffer, " %02u:%02u:%02u", now.hour(), now.minute(), now.second());
            Serial.print(now.year());
            Serial.print(".");
            Serial.print(now.day());
            Serial.print(".");
            Serial.print(now.month());
            Serial.print(", ");
            Serial.print(timeBuffer);
            Serial.println();
            Serial.print(valveOpenTime/1000);
            Serial.println("sec.");
           
          
          myFile = SD.open("valves.csv", FILE_WRITE);
          if (myFile) {
            
            
            myFile.print(valvePins[i]);
            myFile.println(" Open");
            myFile.print(", ");

            char timeBuffer[12];
            sprintf(timeBuffer, " %02u:%02u:%02u", now.hour(), now.minute(), now.second());

            myFile.print(now.year());
            myFile.print(".");
            myFile.print(now.day());
            myFile.print(".");
            myFile.print(now.month());
            myFile.print(", ");
            myFile.print(timeBuffer);
            myFile.println();
            myFile.print(valveOpenTime/1000);
            myFile.println("sec.");
            myFile.close();
          } else {
            Serial.println("error opening valves.csv");
          }
        }
      }

      delay(100);
      lastValveState[i] = currentValveState;
    }
  }

  // Bluetooth start
  if (SerialBT.available()) {
    if (SerialBT.read() == 'd') {
      if (SerialBT.read() == 'a' && SerialBT.read() == 't' && SerialBT.read() == 'a') {
        myFile = SD.open("valves.csv", FILE_READ);
        SerialBT.println("received");
        if (myFile) {
          while (myFile.available()) {
            char c = myFile.read();
            SerialBT.write(c);
          }
          myFile.close();
        } else {
          Serial.println("error opening file");
        }
        delay(1000);
      }
    } else {
      SerialBT.print("invalid command");
    }
  }
  // Bluetooth end

  delay(20);
}

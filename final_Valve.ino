#include "BluetoothSerial.h"
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` and enable it
#endif

BluetoothSerial SerialBT;
RTC_PCF8523 rtc;
File myFile;

const int chipSelect = 5;
static bool dataSentToBluetooth = false;

int valvePins[8] = {2, 4, 15, 16, 17, 34, 35, 32};

int lastValveState[8];

int hourBegin;
int minuteBegin;
int secondBegin;

int hourEnd;
int minuteEnd;
int secondEnd;

int hourFinal;
int minuteFinal;
int secondFinal;

void setup() {
  Serial.begin(115200);

  // Bluetooth
  SerialBT.begin("ESP32test");

  // SD module
  pinMode(SS, OUTPUT);

  if (!SD.begin(5)) {
    Serial.println("Error initializing SD card!");
    return;
  }

  // RTC module
  if (!rtc.begin()) {
    Serial.flush();
    Serial.println("Error initializing RTC");
    while (1);
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  for (int i = 0; i < 8; i++) {
    pinMode(valvePins[i], INPUT);
    lastValveState[i] = digitalRead(valvePins[i]);
  }
}

void loop() {
  for (int i = 0; i < 8; i++) {
    int currentValveState = digitalRead(valvePins[i]);

    if (currentValveState != lastValveState[i]) {
      DateTime now = rtc.now();

      if (currentValveState == HIGH) {
        Serial.print(valvePins[i]);
        Serial.println(": on");
        hourBegin = now.hour();
        minuteBegin = now.minute();
        secondBegin = now.second();
      } else {
        Serial.println("off");
        hourEnd = now.hour();
        minuteEnd = now.minute();
        secondEnd = now.second();

        // Calculate the time difference
        uint32_t startTime = hourBegin * 3600 + minuteBegin * 60 + secondBegin;
        uint32_t endTime = hourEnd * 3600 + minuteEnd * 60 + secondEnd;
        uint32_t timeDifference = endTime - startTime;

        // Calculate hours, minutes, and seconds from the total in seconds
        hourFinal = timeDifference / 3600;
        minuteFinal = (timeDifference % 3600) / 60;
        secondFinal = timeDifference % 60;


        myFile = SD.open("times.txt", FILE_WRITE);
        if (myFile) {
          Serial.println("myFile opened");
          myFile.print("valve ");
          myFile.print(valvePins[i]);
          myFile.print(": ");
          myFile.print(now.day());
          myFile.print("/");
          myFile.print(now.month());
          myFile.print("/");
          myFile.print(now.year());
          myFile.print(" ");
          myFile.print(hourFinal);
          myFile.print("h ");
          myFile.print(minuteFinal);
          myFile.print("m ");
          myFile.print(secondFinal);
          myFile.println("s");

          myFile.close();
        }else
        {
          Serial.println("!!! ERROR FILE DID NOT OPEN !!!");
        }
      }

      delay(100);
      lastValveState[i] = currentValveState;
    }
  }

  if (SerialBT.available() && !dataSentToBluetooth) {
    myFile = SD.open("times.txt", FILE_READ);
    Serial.println("opened");
    SerialBT.println("received");
    if (myFile) {
      while (myFile.available()) {
        char c = myFile.read();
        SerialBT.write(c);
      }
      myFile.close();
      Serial.println("closed");
      dataSentToBluetooth = true;
      Serial.println("bluetooth true");
    } else {
      SerialBT.print("fail!");
    }
  }

  if (dataSentToBluetooth) {
    Serial.println("almost false");
    if (SerialBT.available()) {
      Serial.println("bluetooth false");
      dataSentToBluetooth = false;
    }
  }

  delay(20);
}

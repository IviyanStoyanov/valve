#include "BluetoothSerial.h"
#include <SPI.h>
#include <mySD.h>
#include <RTClib.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;
RTC_PCF8523 rtc;
ext::File myFile;

const int chipSelect = 5;
static bool dataSentToSerial = false;
static bool dataSentToBluetooth = false;

const int buttonPin = 2;
int buttonPushCounter = 0;
int buttonState = 0;
int lastButtonState = 0;


int hourBegin;
int minuteBegin;
int secondBegin;

int hourEnd;
int minuteEnd;
int secondEnd;

int hourFinal;
int minuteFinal;
int secondFinal;

char daysOfWeek[7][12] = {
  "Sunday",
  "Monday",
  "Tuesday",
  "Wednesday",
  "Thursday",
  "Friday",
  "Saturday"
};


void setup() {
  Serial.begin(115200);

  //bluetooth
  SerialBT.begin("ESP32test"); 
  //Serial.println("The device started, now you can pair it with bluetooth!");

  //SD module
  //Serial.print("Initializing SD card...");
   pinMode(SS, OUTPUT);
   
  if (!SD.begin(5)) {
    //Serial.println("initialization failed!");
    return;
  }
  //Serial.println("initialization done.");
  
  //RTC module
  if (!rtc.begin()) {
    //Serial.println("RTC module is NOT found");
    Serial.flush();
    while (1);
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  pinMode(buttonPin, INPUT);

}

void loop() {

buttonState = digitalRead(buttonPin);
myFile = SD.open("times.txt", FILE_WRITE);

  if (buttonState != lastButtonState) {
    if (buttonState == HIGH) {
      DateTime now = rtc.now();
      hourBegin = now.hour();
      minuteBegin = now.minute();
      secondBegin = now.second();
    } else {
      DateTime now = rtc.now();
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

      if (myFile) {
        Serial.print(now.dayOfTheWeek());
        myFile.print(" ");
        myFile.print(hourFinal);
        myFile.print("h ");
        myFile.print(minuteFinal);
        myFile.print("m ");
        myFile.print(secondFinal);
        myFile.println("s");
        myFile.close();
      } else {
        Serial.println("error opening test.txt");
      }
    }


    delay(50);
  }
  lastButtonState = buttonState;
  

 if (SerialBT.available() && !dataSentToBluetooth) {
  myFile = SD.open("times.txt", FILE_READ);
  SerialBT.println("received");
    if (myFile) {
      while (myFile.available()) {
        char c = myFile.read();
        SerialBT.write(c); // Send individual characters directly to Bluetooth
      }
      myFile.close();
      dataSentToBluetooth = true;
    }else{
      SerialBT.print("fail!");
    }
  }

  if (dataSentToBluetooth && !SerialBT.available()) {
    Serial.println("bluetooth false");
    dataSentToBluetooth = false;
  }

  delay(20);
}


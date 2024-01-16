//libraries
  #include "BluetoothSerial.h"
  #include <SPI.h>
  #include <mySD.h>
  #include <RTClib.h>
  #include <Adafruit_I2CDevice.h>

//bluetooth config
  #if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
  #error Bluetooth is not enabled! Please run `make menuconfig` and enable it
  #endif
  BluetoothSerial SerialBT;

//definitions
  RTC_DS3231 rtc;
  ext::File myFile;
  const int chipSelect = 5;

//variables
  int ButtonState;
  int SetClock = 35;
  int Tbut = 36;
  int Rled = 10;
  int lastButtonState = HIGH;

//array for valves
  int valvePins[8] = {39, 34, 35, 15, 25, 26, 27, 9};
  char valveNames[8][3] = {"P1", "P2", "P3", "P4", "P5", "P6", "P7", "P8"};
  int lastValveState[8];
  long valveOpenStartTime[8];  // Track individual valve open time

//sleep
  #define BUTTON_PIN_BITMASK 0x8C0E008200 // GPIOs 39, 34, 35, 15, 25, 26, 27, 9
  RTC_DATA_ATTR int bootCount = 0;

  void print_wakeup_reason(){
    esp_sleep_wakeup_cause_t wakeup_reason;

    wakeup_reason = esp_sleep_get_wakeup_cause();

    switch(wakeup_reason)
    {
      case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
      case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
      case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
      case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
      case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
      default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
    }
  }

  void print_GPIO_wake_up(){
    uint64_t GPIO_reason = esp_sleep_get_ext1_wakeup_status();
    Serial.print("GPIO: ");
    Serial.println((log(GPIO_reason))/log(2), 0);
  }


void setup() {
  pinMode(Tbut, INPUT);
  //pinMode(SetClock, INPUT);
  pinMode(Rled, OUTPUT);
  delay(1000);
  Serial.begin(115200);
  //SetClockButton = digitalRead(SetClock);


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
      digitalWrite(Rled, HIGH); 
    }
  
// if (SetClockButton = LOW) {
// rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
// }
  
  // pins
    for (int i = 0; i < 8; i++) {
      pinMode(valvePins[i], INPUT);
      digitalWrite(valvePins[i], LOW);
    }

}

void loop() {
  int ButtonState = digitalRead(Tbut);

  //printing current time
  if (ButtonState == LOW && lastButtonState == HIGH) {
    DateTime now = rtc.now();
    char timeBuffer[12];
    sprintf(timeBuffer, " %02u:%02u:%02u", now.hour(), now.minute(), now.second());
    Serial.printf("%s\n", timeBuffer);
  }
  lastButtonState = ButtonState;

 //cheking for a change in the states       
 for (int i = 0; i < 8; i++) 
  {
    pinMode(valvePins[i],INPUT);
    int currentValveState = digitalRead(valvePins[i]);

    if (currentValveState != lastValveState[i]) 
    {
      DateTime now = rtc.now();

      if (currentValveState == LOW) 
      {
        // Valve opened
        valveOpenStartTime[i] = millis();
      } 
      else 
      {
        // Valve closed
        long valveOpenTime = millis() - valveOpenStartTime[i];

        if (valveOpenTime >= 3000) 
        {
            Serial.print( valveNames[i]);
            Serial.print(" Open  " );       
            char timeBuffer[12];
            sprintf(timeBuffer, " %02u:%02u:%02u", now.hour(), now.minute(), now.second());
            
            
            Serial.print(now.day());
            Serial.print(".");
            Serial.print(now.month());
            Serial.print(".");
            Serial.print(now.year());
            Serial.print(" , ");
            Serial.print(timeBuffer);
            Serial.print(" , ");
            Serial.print(valveOpenTime/1000);
            Serial.println("sec.");
          myFile = SD.open("valves.csv", FILE_WRITE);

          if (myFile)
          {
            myFile.print(valveNames[i]);
            myFile.print(" Open");
            myFile.print(" ,  ");
            char timeBuffer[12];
            sprintf(timeBuffer, " %02u:%02u:%02u", now.hour(), now.minute(), now.second());
            
            
            myFile.print(now.day());
            myFile.print(".");
            myFile.print(now.month());
            myFile.print(".");
            myFile.print(now.year());
            myFile.print(" , ");
            myFile.print(timeBuffer);
            myFile.print(" , ");
            myFile.print(valveOpenTime/1000);
            myFile.println("sec.");
            myFile.close();
          } else {
            Serial.println("error opening valves.csv");
          }
        }
      }

        ++bootCount;
        Serial.println("Boot number: " + String(bootCount));

        //Print the wakeup reason for ESP32
          print_wakeup_reason();

        //Print the GPIO used to wake up
          print_GPIO_wake_up();
          esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK,ESP_EXT1_WAKEUP_ANY_HIGH);

        //go to sleep
          if (currentValveState == lastValveState[i]) {
            Serial.println("SLEEP");
            delay(1000);
            esp_deep_sleep_start();
          }

      delay(100);
      lastValveState[i] = currentValveState;
    }
  }

  // Bluetooth start
    if (SerialBT.available()) 
    {
        DateTime now = rtc.now();
        char command[4]; // Assuming commands are 6 characters long

        for (int i = 0; i < 4; ++i) 
        {
            command[i] = SerialBT.read();
        }

        if (strncmp(command, "del", 3) == 0)
        {
          char filename[] = "valves.csv";
          SD.remove(filename);
          SerialBT.println("File deleted");
            myFile = SD.open("valves.csv", FILE_WRITE);
          char timeBuffer[12];
          sprintf(timeBuffer, " %02u.%02u.%02u, %02u:%02u:%02u", now.day(), now.month(),now.year(),now.hour(), now.minute(), now.second());
          myFile.print("deleted at: ");
          myFile.println(timeBuffer);
          myFile.println("**********************************");
          myFile.close();
            
        } 
        else if (strncmp(command, "data", 4) == 0)
        {
          myFile = SD.open("valves.csv", FILE_READ);
          SerialBT.println("received"); 
          
        }
        if (myFile) 
        {
            while (myFile.available()) 
            {
                char c = myFile.read();
                SerialBT.write(c);
            }
            myFile.close();
        } 
        else 
        {
        Serial.println("error opening valves.csv");
        }
                    
    }
  delay(20);
}
 

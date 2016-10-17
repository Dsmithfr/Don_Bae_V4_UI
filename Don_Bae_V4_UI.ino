
/* THIS VERSION OF THE CODE V2 EXCLUDES THE CLASSES/METHODS AND PUTS THEM DIRECTLY
INTO THE MAIN LOOP METHOD. ALSO TESTS GETTING RID OF THE CTPTOUCHED METHOD TO AND
INSTEAD RELIES ON "if (! ctp.touched()){ return;}" WHICH SHOULD STAY UNTIL THE
SCREEN IS TOUCHED BEFORE MOVING ON. TOUCHSCREENCOUNT IS ALSO EXCLUDED.

WILL NEED TO DO FUNNY THINGS WITH THE COUNT UP AND COUNT DOWN TIMER VARIABLES

-DUNCAN, 5/20/2016
*/


#include <Adafruit_GFX.h>                                 // Core graphics library
#include <SPI.h>                                          // this is needed for display
#include <Adafruit_ILI9341.h>                             // Color Library
#include <Wire.h>
#include <Adafruit_FT6206.h>                              // this is needed for FT6206
#include <SD.h>                                           // SD Card Library
#include "Adafruit_ILI9341.h"
#include <math.h>                                         // Math Library
#include <Average.h>                                      // Averaging Library

// The FT6206 uses hardware I2C (SCL/SDA)
Adafruit_FT6206 ctp = Adafruit_FT6206();                  // Touchscreen Object Definition

#define TFT_CS 10                                         // The display also uses hardware SPI, plus #9 & #10
#define TFT_DC 9
#define SD_CS 4                                           // SD Card pin
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

//#define BOXSIZE 40                                      // Size of the color selection boxes and the paintbrush size. probably not needed (Excluding it)
//#define PENRADIUS 3

double Vrail = 5;                                         // Rail voltage in temp equation
double R = 10000;                                         // Constant resistor in temp equation
double Rv;                                                // Variable Resistance
double vNode;                                             // Vnode Voltage
double SH_A = 1.1293e-3;                                  // Steinhart-Hart A Constant
double SH_B = 2.3411e-4;                                  // Steinhart-Hart B Constant
double SH_C = 8.7733e-8;                                  // Steinhart-Hart C Constant
double max = 0;                                           // max VNode
File dataFile;                                            // Initializing DataFile
int cuttingTimer = 0;
int participantNumber = 1;
int analogPinMux1 = 8;
int analogPinMux2 = 9;
int analogPinMux3 = 10;
int analogPinMux4 = 11;
double maxTempVoltage = 5;

void setup(void) {
  while (!Serial);     // used for leonardo debugging

  Serial.begin(115200);

  tft.begin();
  tft.setCursor (0, 0);
  tft.fillScreen(ILI9341_BLACK);

  tft.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    tft.println("SD card not found!");
    while (1);
  }
  else {
    tft.println("SD card found!");
    tft.fillScreen(ILI9341_BLACK);
  }

  if (! ctp.begin(40)) {  // pass in 'sensitivity' coefficient
    Serial.println("Couldn't start FT6206 touchscreen controller");
    while (1);
  }

  Serial.println("Capacitive touchscreen started");

  dataFile = SD.open("data.csv", FILE_WRITE);
  if (! dataFile) {
    tft.println("error opening data.csv");
    while (1);
  }


  pinMode(53, OUTPUT);
  pinMode(51, OUTPUT);
  pinMode(49, OUTPUT);
  pinMode(47, OUTPUT);
  pinMode(45, OUTPUT);
  pinMode(43, OUTPUT);
  pinMode(41, OUTPUT);
  pinMode(39, OUTPUT);
  pinMode(37, OUTPUT);
  pinMode(35, OUTPUT);
  pinMode(33, OUTPUT);
  pinMode(31, OUTPUT);

  pinMode(52, OUTPUT);
  digitalWrite(52, LOW);

}

void loop() {

  tft.drawRect(90, 150, 80, 40, ILI9341_BLUE);                //Make box in the middle of the screen
  tft.setTextSize(2);
  tft.setCursor (110, 160);
  tft.setTextColor(ILI9341_WHITE);
  tft.println("Start");

  if (!ctp.touched()) {                                         //Waits for the screen to be touched
    return;
  }
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor (0, 160);
  tft.setTextSize(3);
  tft.println("Please wait");
  delay(1000);
  tft.setTextSize(2);

  //WRAPTIMER

  int wrapTimes = 0;
  int wrapMinutes;
  int wrapSeconds;
  tft.setTextColor(ILI9341_BLACK);

  while (!ctp.touched()) {

    tft.fillScreen(ILI9341_WHITE);

    tft.setCursor(0, 0);
    tft.println("wrapTimer");

    wrapMinutes = (wrapTimes / 60);
    wrapSeconds = (wrapTimes - (wrapMinutes * 60));
    tft.print("Time:");
    tft.print(wrapMinutes);
    tft.print(" : ");
    tft.println(wrapSeconds);
    wrapTimes++;
    tft.println();
    tft.println();
    tft.println("Touch Screen");
    tft.println("When Done Wrapping");

    delay(323);                                                //This delay is dependant on experimental timing because this loops takes too long for a simple 1 second delay

  }

  tft.fillScreen(ILI9341_WHITE);
  tft.println("Starting Cure Timer");
  delay(3000);

  //CURETIMER

  int cureTimes = 10;                                        //change back to 240
  int cureMinutes;
  int cureSeconds;

  while (cureTimes > 0) {

    tft.fillScreen(ILI9341_WHITE);
    tft.setCursor(0, 0);
    tft.println("cureTimer");

    cureMinutes = (cureTimes / 60);
    cureSeconds = (cureTimes - (cureMinutes * 60));
    tft.print("Time:");
    tft.print(cureMinutes);
    tft.print(" : ");
    tft.print(cureSeconds);
    cureTimes--;
    delay(330);                                        //Double check this to make sure that this loop is actually 1 second
  }

  //CASTCUTTING

  tft.fillScreen(ILI9341_WHITE);

  analogWrite(A0, HIGH);
  analogWrite(A1, HIGH);
  analogWrite(A2, HIGH);
  analogWrite(A3, HIGH);
  analogWrite(A4, HIGH);
  analogWrite(A5, HIGH);
  analogWrite(A6, HIGH);
  analogWrite(A7, HIGH);

  dataFile.print("Participant Number ");
  dataFile.println(participantNumber);

  tft.setCursor(0, 0);

  while (! ctp.touched()) {

    double realVNode [30];                                        //Array for real time Vode. Resets every time

    for (int state = 0; state < 30; state++) {

      int j = state;
      int k = state - 8;
      int l = state - 16;
      int m = state - 24;

      if (state < 8) {

        String address = String(j, BIN);        //converts 8 to binary

        int intAddress = address.toInt();          //converts binary address into int


        int z = bitRead(intAddress, 2);            //third bit, first mux
        int y = bitRead(intAddress, 1);            //second bit, first mux
        int x = bitRead(intAddress, 0);            //first bit, first mux



        if (x == 1) {
          digitalWrite(53, HIGH);                  //sets pin 53 to high if first bit is high
        }
        if (x == 0) {
          digitalWrite(53, LOW);                   //sets pin 53 to low if first bit is low
        }
        if (y == 1) {
          digitalWrite(51, HIGH);                  //sets pin 51 to high if second bit is high
        }
        if (y == 0) {
          digitalWrite(51, LOW);                   //sets pin 51 to low if second bit is low
        }
        if (z == 1) {
          digitalWrite(49, HIGH);                  //sets pin 49 to high if third bit is high
        }
        if (z == 0) {
          digitalWrite(49, LOW);                   //sets pin 49 to low if third bit is low
        }

        realVNode[state] = analogRead(8);            //adds thermistor value to int table
        delay(10);

      }

      if (8 <= state  && state < 16) {

        String address = String(k, BIN);            //converts 8 to binary

        int intAddress = address.toInt();            //converts binary address into int


        int z = bitRead(intAddress, 2);            //third bit, second mux
        int y = bitRead(intAddress, 1);            //second bit, second mux
        int x = bitRead(intAddress, 0);            //first bit, second mux


        if (x == 1) {
          digitalWrite(47, HIGH);                  //sets pin 47 to high if first bit is high
        }
        if (x == 0) {
          digitalWrite(47, LOW);                   //sets pin 47 to low if first bit is low
        }
        if (y == 1) {
          digitalWrite(45, HIGH);                  //sets pin 45 to high if second bit is high
        }
        if (y == 0) {
          digitalWrite(45, LOW);                   //sets pin 45 to low if second bit is low
        }
        if (z == 1) {
          digitalWrite(43, HIGH);                  //sets pin 43 to high if third bit is high
        }
        if (z == 0) {
          digitalWrite(43, LOW);                   //sets pin 43 to low if third bit is low
        }

        realVNode[state] = analogRead(9);            //adds thermistor value to int table
        delay(10);

      }


      if (16 <= state && state < 24) {

        String address = String(l, BIN);        //converts 8 to binary

        int intAddress = address.toInt();          //converts binary address into int


        int z = bitRead(intAddress, 2);            //third bit, third mux
        int y = bitRead(intAddress, 1);            //second bit, third mux
        int x = bitRead(intAddress, 0);            //first bit, third mux


        if (x == 1) {
          digitalWrite(41, HIGH);                  //sets pin 41 to high if first bit is high
        }
        if (x == 0) {
          digitalWrite(41, LOW);                   //sets pin 41 to low if first bit is low
        }
        if (y == 1) {
          digitalWrite(39, HIGH);                  //sets pin 39 to high if second bit is high
        }
        if (y == 0) {
          digitalWrite(39, LOW);                   //sets pin 39 to low if second bit is low
        }
        if (z == 1) {
          digitalWrite(37, HIGH);                  //sets pin 37 to high if third bit is high
        }
        if (z == 0) {
          digitalWrite(37, LOW);                   //sets pin 37 to low if third bit is low
        }


        realVNode[state] = analogRead(10);            //adds thermistor value to int table
        delay(10);
      }

      if (24 <= state && state < 30) {

        String address = String(m, BIN);        //converts 8 to binary

        int intAddress = address.toInt();          //converts binary address into int


        int z = bitRead(intAddress, 2);            //third bit, third mux
        int y = bitRead(intAddress, 1);            //second bit, third mux
        int x = bitRead(intAddress, 0);            //first bit, third mux


        if (x == 1) {
          digitalWrite(35, HIGH);                  //sets pin 35 to high if first bit is high
        }
        if (x == 0) {
          digitalWrite(35, LOW);                   //sets pin 35 to low if first bit is low
        }
        if (y == 1) {
          digitalWrite(33, HIGH);                  //sets pin 33 to high if second bit is high
        }
        if (y == 0) {
          digitalWrite(33, LOW);                   //sets pin 33 to low if second bit is low
        }
        if (z == 1) {
          digitalWrite(31, HIGH);                  //sets pin 31 to high if third bit is high
        }
        if (z == 0) {
          digitalWrite(31, LOW);                   //sets pin 31 to low if third bit is low
        }

        realVNode[state] = analogRead(11);            //adds thermistor value to int table
        delay(10);

      }

    }

    String dataString = "";

    max = realVNode[0] / 308.78;

    for (int s = 0; s < 30; s++) {
      double sensor = realVNode[s] / 308.78;      //Turns "sensor" into the node voltage. 310.3 is 1024bits/3.3V
      dataString += String(sensor);

      if (s < 30) {
        dataString += ",";
      }
      if (sensor < max) {
        max = sensor;
      }
    }

    if (max > 1.4789) {
      tft.fillScreen(ILI9341_GREEN);
    }

    else if (max <= 1.4789 && max > 0.9958) {
      tft.fillScreen(ILI9341_YELLOW);
    }

    else if (max <= 0.9958) {
      tft.fillScreen(ILI9341_RED);
    }

    tft.setCursor(0, 0);
    tft.print(max);

    dataFile.println(dataString);
    dataFile.flush();

    if (max < maxTempVoltage) {
      maxTempVoltage = max;
    }

    cuttingTimer++;
    delay(44);                                                  //Need to double check this time

  }

  participantNumber++;

  //DISPLAYRESULTS

  tft.fillScreen(ILI9341_WHITE);
  tft.setTextColor(ILI9341_BLACK);
  tft.setCursor(0, 0);
  int dispMinutes;
  double dispSeconds;

  double dispTimes = cuttingTimer;                        //Because system runs at 3Hz

  dispMinutes = (dispTimes / 60);
  dispSeconds = (dispTimes - (dispMinutes * 60));
  tft.print("Removal Time:");
  tft.print(dispMinutes);
  tft.print(" : ");
  tft.println(dispSeconds);


  Rv = (maxTempVoltage * R) / (Vrail - maxTempVoltage);
  double lnRv = log(Rv);
  double inverseMaxTemp = SH_A + SH_B * lnRv + SH_C * (pow(lnRv, 3));
  double maxTemp = (1 / inverseMaxTemp) - 273.15;
  tft.print("Max Temp: ");
  tft.println(maxTemp);
  //tft.print("Max Temp Voltage: ");
  //tft.println(maxTempVoltage);
  //tft.print("Variable Resistance: ");
  //tft.println(Rv);


  if (! ctp.touched()) {
    return;
  }
  //Will loop back to the beginning after screen is touched

}

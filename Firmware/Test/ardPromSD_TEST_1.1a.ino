/*
  v1.1aT
  https://github.com/dtomcat/ArdPromSD

  ArduinoPromSD is a derivative of the work by Ryzee119 
  (ArduinoProm: https://github.com/dtomcat/ArduinoPromSD)
  This uses an SD card instead of serial communications to 
  backup/write the EEPROM data from the original Xbox.

  This software is designed to test to ensure board was soldered correctly.


  This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#define GLED                4
#define RLED                5
#define BUTT                6 

#include <SPI.h>
#include <SD.h>

void setup() {
  pinMode(GLED, OUTPUT);
  pinMode(RLED, OUTPUT);
  pinMode(BUTT, INPUT);
  
  Serial.begin(9600);
  
  while (!Serial) {  // wait for serial port to connect. Needed for native USB port only
    digitalWrite(GLED, !digitalRead(GLED));
    digitalWrite(RLED, !digitalRead(RLED));
    delay(500); 
  }
  
  digitalWrite(GLED, HIGH); //Green LED on
  digitalWrite(RLED, HIGH); //Red LED on
  
  Serial.print("Initializing SD card...");
  //Initialize SD card then check for folder... 
  //if doesn't exist; create it
  if (!SD.begin(10)) {
    delay(100);
    Serial.println("*********************************************");
    Serial.println("SD Card Initialization Failed!");
    Serial.println("*********************************************");
    setError();
    while (1);
  }
  Serial.println("OK");
  Serial.println("Checking for \"test\" folder...");
  if (!SD.exists("test"))
  {
    Serial.println("test doesn't exist...  creating it");
    if (!SD.mkdir("test")) {
      Serial.println("*********************************************");
      Serial.println("Error creating folder");
      Serial.println("*********************************************");
      setError();
      while (1);
    }
  }
  Serial.println("OK");
  //create text.txt and read and write ot it.
  char action;
  if (SD.exists("test/test.txt"))
  {  
    action = O_WRITE | O_TRUNC;
  }else
  {
    action = FILE_WRITE;
  }
  File myFile = SD.open("test/test.txt", action);
  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing Text to test.txt...");
    int returnStatus = -1;
    returnStatus = myFile.write("testing...");
    if (!returnStatus) {
        Serial.println("*********************************************");
        Serial.println("Error Writing test.txt file.  Please check SD card!");
        Serial.println("*********************************************");
        setError();
        myFile.close();
        while (1);
    }
  }
  else {
      Serial.println("*********************************************");
      Serial.println("Error opening test.txt!");
      Serial.println("*********************************************");
      setError();
      myFile.close();
      while (1);
  }
  // close the file:
  myFile.close();
  Serial.println("Done testing write process...");
  setOK();
  Serial.println("Checking read process...");
  File readFile = SD.open("test/test.txt");
  // if the file opened okay, write to it:
  if (readFile) {
    Serial.print("reading test.txt...");
    Serial.println("CONTENTS:");
    while (readFile.available()) { //execute while file is available
      char letter = readFile.read(); //read next character from file
      Serial.print(letter); //display character
      delay(100);
    }
    Serial.println("");
    Serial.println("*********************************************");
    Serial.println("If \"testing...\" is not displayed under \"CONTENTS:\"... then writing/reading failed");
    Serial.println("*********************************************");
  }
  else {
      Serial.println("*********************************************");
      Serial.println("Error opening test.txt!");
      Serial.println("*********************************************");
      setError();
      readFile.close();
      while (1);
  }
  // close the file:
  readFile.close();
  Serial.println("Done testing read process...");
  setOK();
}

void loop() {
  int returnStatus = -1;
  int buttonState = digitalRead(BUTT); // Button status
  //Check button state and display it
  Serial.println("Press and hold button to test button function!");
  while (!buttonState) {
      digitalWrite(GLED, !digitalRead(GLED));
      buttonState = digitalRead(BUTT);
      delay(100);
    }
  Serial.println("pressed pressed, release button");
  while (buttonState) {
      buttonState = digitalRead(BUTT);
      delay(100);
  }
  Serial.println("*********************************************");
  Serial.println("*********************************************");
  Serial.println("Congratulations, Board Passed!");
  Serial.println("*********************************************");
  Serial.println("*********************************************");
  setOK();
  while(1); 
}

void setError() {
  digitalWrite(RLED, HIGH); //Red LED on
  digitalWrite(GLED, LOW);  //Green LED off
}

void setOK() {
  digitalWrite(RLED, LOW); //Red LED off
  digitalWrite(GLED, HIGH);  //Green LED on
}

                                                                            /*
  v1.3a
  https://github.com/dtomcat/ArdPromSD

  ArduinoPromSD is a derivative of the work by Ryzee119 
  (ArduinoProm: https://github.com/dtomcat/ArduinoPromSD)
  This uses an SD card instead of serial communications to 
  backup/write the EEPROM data from the original Xbox.

  ArduinoProm. An Arduino based Original Xbox EEPROM reader and writer.
  It can be used to recover from a corrupt BIOS or recover HDD key etc.

  ArduinoProm is inspired and adapted upon the awesome work by Grimdoomer on PiPROM.
  https://github.com/grimdoomer/PiPROM


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

#define XBOX_EEPROM_ADDRESS 0x54
#define XBOX_EEPROM_SIZE    256
#define GLED                4
#define RLED                5
#define BUTT                6 

#include <Wire.h>
#include <SPI.h>
#include <SD.h>

char pbEEPROM[XBOX_EEPROM_SIZE];

void setup() {
  pinMode(GLED, OUTPUT);
  pinMode(RLED, OUTPUT);
  pinMode(BUTT, INPUT);
  digitalWrite(GLED, HIGH); //Green LED on
  digitalWrite(RLED, HIGH); //Red LED on

  Wire.begin();
  Wire.setWireTimeout(30000000);
  //Initialize SD card then check for folder... 
  //if doesn't exist; create it
  if (!SD.begin(10)) {
    delay(100);
    setError();
    while (1);
  }
  if (!SD.exists("epbackup"))
  {
    if (!SD.mkdir("epbackup")) {
      setError();
      while (1);
    }
  }
  //Check for communication with EEPROM
  int returnStatus = -1;
  returnStatus = XboxI2C_DetectEEPROM(XBOX_EEPROM_ADDRESS);
  if (returnStatus == -1) {
    setError();
    while (1);
  }
  //create eeprom.bin and read eeprom from xbox and
  //dump into eeprom.bin
  String filename;
  filename = "epbackup/eeprom.bin";
  int fn = 1;
  while (SD.exists(filename))
  {
    Serial.println(String(fn));  
    filename = "epbackup/eeprom" + String(fn++) + ".bin";
    delay(10);
  }
  File myFile = SD.open(filename, FILE_WRITE);
  // if the file opened okay, write to it:
  if (myFile) {
    returnStatus = XboxI2C_ReadEEPROM(XBOX_EEPROM_ADDRESS, pbEEPROM);
    if (returnStatus != -1) {
      returnStatus = myFile.write(pbEEPROM, XBOX_EEPROM_SIZE);
      if (!returnStatus) {
        setError();
        myFile.close();
        while (1);
      }
    }
    else {
      setError();
      myFile.close();
      while (1);
    }
    // close the file:
    myFile.close();
    setOK();
  }
  else {
    setError();
    while (1);
  }
  //Clear EEPROM data from memory
  memset(pbEEPROM, 0, XBOX_EEPROM_SIZE);
}

void loop() {
  int returnStatus = -1;
  int buttonState = digitalRead(BUTT); //Button Status
  
  //Check if eeprom.bin in writeep folder to write to EEPROM
  //If so, let user know it's ready to write (blinking Green LED)
  //if not, halt program
  if (SD.exists("writeep/eeprom.bin"))
  {
    while (!buttonState) {
      digitalWrite(GLED, !digitalRead(GLED));
      buttonState = digitalRead(BUTT);
      delay(100);
    }
    digitalWrite(GLED, HIGH);

    //Write bin to EEPROM
    memset(pbEEPROM, 0, XBOX_EEPROM_SIZE);
    File myFile = SD.open("writeep/eeprom.bin", FILE_READ);
    returnStatus = myFile.read(pbEEPROM, XBOX_EEPROM_SIZE);
    //Check if file read
    if (returnStatus) {
      returnStatus = XboxI2C_WriteEEPROM(XBOX_EEPROM_ADDRESS, pbEEPROM);
      //Check if write was successful
      if (returnStatus != -1) {
        myFile.close();
        //After successful write... remove file
        SD.remove("writeep/eeprom.bin");
        setOK();
        while (1);
      }
    } else {
      setError();
      myFile.close();
      while(1);
    }
  } else {
    while (1);
  }
}

void setError() {
  digitalWrite(RLED, HIGH); //Red LED on
  digitalWrite(GLED, LOW);  //Green LED off
}

void setOK() {
  digitalWrite(RLED, LOW); //Red LED off
  digitalWrite(GLED, HIGH);  //Green LED on
}

//***********************************************
//**** FOLLOWING CODE WRITTEN BY RYZEE119 *******
//***********************************************


//Read the EEPROM
//bAddress: I2C address of EEPROM
//pbBuffer: Pointer the receiver buffer (256 bytes minimum)
//Returns: -1 on error, 0 on success.
int XboxI2C_ReadEEPROM(char bAddress, char *pbBuffer)
{
  //Some input sanity
  if (bAddress < 0 || bAddress > 128)
    return -1;
  if (pbBuffer == 0)
    return -1;

  memset(pbBuffer, 0, XBOX_EEPROM_SIZE);

  // Read the EEPROM buffer from the chip
  int add = 0;
  Wire.beginTransmission(bAddress);
  Wire.write(add);
  if (Wire.endTransmission(false) == 0) {
    while (add < XBOX_EEPROM_SIZE) {
      Wire.requestFrom(bAddress, 1);
      if (Wire.available()) {
        pbBuffer[add] = Wire.read();
      } else {
        return -1;
      }
      add++;
    }
  } else {
    return -1;
  }

  // Successfully read the EEPROM chip.
  return 0;
}


//Write the EEPROM
//bAddress: I2C address of EEPROM
//pbBuffer: Pointer the transmit data buffer (256 bytes minimum)
//Returns: -1 on error, 0 on success.
int XboxI2C_WriteEEPROM(char bAddress, char *pbBuffer)
{
  char commandBuffer[2];
  int i;

  //Some input sanity
  if (bAddress < 0 || bAddress > 128)
    return -1;
  if (pbBuffer == 0)
    return -1;

  //Loop through the buffer to write.
  for (i = 0; i < XBOX_EEPROM_SIZE; i++)
  {
    Wire.beginTransmission(bAddress);
    // Set the target address and data for the current byte.
    commandBuffer[0] = (char)i;
    commandBuffer[1] = pbBuffer[i];

    // Write the data to the chip.
    Wire.write(commandBuffer, 2);

    if (Wire.endTransmission() != 0) {
      return -1;
    }

    //Wait before writing the next byte.
    delay(10);
  }

  //Successfully wrote the buffer to the EEPROM chip.
  return 0;
}

//Requests an ACK from the EEPROM to detect if it's present
//bAddress: I2C address of EEPROM
//Returns: -1 on error, 0 on success.
int XboxI2C_DetectEEPROM(char bAddress)
{
  Wire.beginTransmission(bAddress);
  if (Wire.endTransmission(true) != 0) {
    return -1;
  }
  //I2C device at specified address detected
  return 0;
}

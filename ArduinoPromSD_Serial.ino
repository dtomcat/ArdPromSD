/*
  https://github.com/dtomcat/ArduinoPromSD

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

#define XBOX_EEPROM_ADDRESS  0x54
#define XBOX_EEPROM_SIZE    256

#include <Wire.h>
#include <SPI.h>
#include <SD.h>

File myFile;  //SD card operations
int GLED = 4; //OK LED
int RLED = 5; //Error LED
int BUTT = 6; //Button

char pbEEPROM[XBOX_EEPROM_SIZE];
int returnStatus = -1;

// variables will change:
int buttonState = 0;  // Button status

void setup() {
  pinMode(GLED, OUTPUT);
  pinMode(RLED, OUTPUT);
  pinMode(BUTT, INPUT);
  digitalWrite(GLED, HIGH); //Green LED on
  digitalWrite(RLED, HIGH); //Red LED on

  Serial.begin(9600);
  Wire.begin();
  Wire.setWireTimeout(3000000);
  while (!Serial) {
  ; // wait for serial port to connect. Needed for native USB port only
  }
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
  Serial.println("Checking for \"epbackup\" folder...");
  if (!SD.exists("epbackup"))
  {
    Serial.println("epbackup doesn't exist...  creating it");
    if (!SD.mkdir("epbackup")) {
      Serial.println("*********************************************");
      Serial.println("Error creating folder");
      Serial.println("*********************************************");
      setError();
      while (1);
    }
  }
  Serial.println("OK");
  Serial.print("Trying to communicate with Xbox EEPROM...");
  //Check for communication with EEPROM
  returnStatus = XboxI2C_DetectEEPROM(XBOX_EEPROM_ADDRESS);
  if (returnStatus == -1) {
    Serial.println("*********************************************");
    Serial.println("Error - EEPROM not detected.  Please check wires and ensure Xbox is on");
    Serial.println("*********************************************");
    setError();
    while (1);
  }
  Serial.println("OK");
  //create eeprom.bin and read eeprom from xbox and
  //dump into eeprom.bin
  myFile = SD.open("epbackup/eeprom.bin", FILE_WRITE);
  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Reading EEPROM...");
    returnStatus = XboxI2C_ReadEEPROM(XBOX_EEPROM_ADDRESS, pbEEPROM);
    if (returnStatus != -1) {
      Serial.println("OK");
      returnStatus = myFile.write(pbEEPROM, XBOX_EEPROM_SIZE);
      if (!returnStatus) {
        Serial.println("*********************************************");
        Serial.println("Error creating eeprom.bin file.  Please check SD card!");
        Serial.println("EEPROM is NOT backed up!!!!");
        Serial.println("*********************************************");
        setError();
        while (1);
      }
    }
    else {
      Serial.println("*********************************************");
      Serial.println("Error reading eeprom!");
      Serial.println("*********************************************");
      setError();
      while (1);
    }
    // close the file:
    myFile.close();
    Serial.println("Done backing up process...");
    digitalWrite(RLED, LOW);
  }
  else {
    Serial.println("*********************************************");
    Serial.println("Error creating eeprom.bin file!");
    Serial.println("*********************************************");
    setError();
    while (1);
  }
  //Clear EEPROM data from memory
  memset(pbEEPROM, 0, XBOX_EEPROM_SIZE);
}

void loop() {
  returnStatus = -1;
  buttonState = digitalRead(BUTT);
  
  //Check if eeprom.bin in writeep folder to write to EEPROM
  //If so, let user know it's ready to write (blinking Green LED)
  //if not, halt program
  if (SD.exists("writeep/eeprom.bin"))
  {
    Serial.println("EEPROM.BIN found in writeep folder, waiting for button press to write EEPROM...");
    while (!buttonState) {
      digitalWrite(GLED, !digitalRead(GLED));
      buttonState = digitalRead(BUTT);
      delay(100);
    }
    digitalWrite(GLED, HIGH);

    //Write bin to EEPROM
    memset(pbEEPROM, 0, XBOX_EEPROM_SIZE);
    myFile = SD.open("epbackup/eeprom.bin", FILE_READ);
    returnStatus = myFile.read(pbEEPROM, XBOX_EEPROM_SIZE);
    //Check if file read
    if (returnStatus) {
      Serial.println("......................................");
      Serial.println("Writing");
      Serial.println("......................................");
      returnStatus = XboxI2C_WriteEEPROM(XBOX_EEPROM_ADDRESS, pbEEPROM);
      //Check if write was successful
      if (returnStatus != -1) {
        Serial.println("......................................");
        Serial.println("EEPROM writen successfully!");
        Serial.println("......................................");
        myFile.close();
        //After successful write... remove file
        SD.remove("writeep/eeprom.bin");
        Serial.println("......................................");
        Serial.println("EEPROM.bin erased from writeep folder!");
        Serial.println("......................................");
        digitalWrite(RLED, LOW);
        digitalWrite(GLED, HIGH);
        Serial.println("*********************************************");
        Serial.println("Backup and Write processes completed successfully");
        Serial.println("Remove module and plug back in to restart!");
        Serial.println("*********************************************");
        while (1);
      }
    } else {
      Serial.println("*********************************************");
      Serial.println("Error Reading EEPROM.bin from writeep folder!");
      Serial.println("*********************************************");
      setError();
    }
  } else {
    Serial.println("*********************************************");
    Serial.println("Write files not present... Program ended!");
    Serial.println("Remove module and plug back in to restart!");
    Serial.println("*********************************************");
    while (1);
  }
}

void setError() {
  digitalWrite(RLED, HIGH); //Red LED on
  digitalWrite(GLED, LOW);  //Green LED off
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
  Serial.println("READING...");
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

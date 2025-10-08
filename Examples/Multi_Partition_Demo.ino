/*
 ******************************************************************************
 * EEProm_Safe_Wear_Level Library v25.10.5 (Multi-Partition, Single Instance)
 * Copyright (C) 2025, Torsten Frieser / automatician
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library. If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************
 */

#include <EEProm_Safe_Wear_Level.h>
#include <Arduino.h>

// ----------------------------------------------------
// --- DEFINITIONS AND INSTANCES ---
// ----------------------------------------------------

const uint16_t PARTITION_VERSION = 4;
const uint8_t COUNTER_LENGTH_BYTES = 1;  // WARNING Max is 4

// --- HANDLE DEFINITIONS ---
#define PART_CNT 2                      //Two logical partitions 

// Unique handle IDs for the ONE instance
// consecutive numbering
#define HANDLE1  0
#define HANDLE2  1


// The ControlData structure must provide space for 2 partitions (4 * 16 Bytes = 64 Bytes)
typedef struct {
    uint8_t data[16 * PART_CNT];  
} __attribute__((aligned(8))) AlignedArray_t;
AlignedArray_t PartitionsData;

//Offset Definitions for PartitionsData
#define currentLogicalCounter 0 // Counter for the total number of sectors written
#define nextPhysicalSector 4    // Physical sector that will be written to next
#define startAddress 6          // Start address of the partition in the EEPROM
#define payloadSize 8           // Size of the pure payload data (EepromData)
#define numberOfSectors 10      // Total number of available sectors in the partition
#define counterByteLength 12    // Size of a single sector (Payload + Metadata)
#define status 13               // Status Byte
#define checksum 14             // Checksum for the control data

//Instanz von EEProm_Safe_Wear_Level erzeugen
EEProm_Safe_Wear_Level EEPRWL_Main(PartitionsData.data); 

// --- ADDRESSES AND SIZES ---
// Partition#1: 0 - 255
// Partition#2: 256 - 511
#define ADDR1 0
#define ADDR2 256
#define SIZE1 256
#define SIZE2 256

// Payload Sizes
#define PAYLOAD_SIZE1 9         //length of device_name
#define PAYLOAD_SIZE2 sizeof(unsigned long) //length of data type for millis() time stamp
// Data
char device_name[9];            // String Array (safe)
unsigned long time_stamp = 0;   // variable for millis()

// Control variable
byte loopCounter = 2;

// ----------------------------------------------------
// --- 2. SETUP ---
// ----------------------------------------------------

void setup() {
    // Zufällige Verzögerung zwischen 800 und 1500 ms
    randomSeed(analogRead(A0));
    long randomDelay = random(800, 1501); 
    
    Serial.begin(115200);
    delay(randomDelay); 

    Serial.println(F("\r\r\r\r\r\r\r\r\r\n"));
    Serial.println(F("---------------------------------------------------------------------------"));
    Serial.println(F("--- EEProm_Safe_Wear_Level Demo Start: Multi-Partition, Single Instance ---"));
    Serial.println(F("---------------------------------------------------------------------------"));
    
    int s1 = EEPRWL_Main.config(ADDR1, SIZE1, PAYLOAD_SIZE1, COUNTER_LENGTH_BYTES, HANDLE1); 
    int s2 = EEPRWL_Main.config(ADDR2, SIZE2, PAYLOAD_SIZE2, COUNTER_LENGTH_BYTES, HANDLE2);    

    if (!s1) Serial.println(F("Partition#1 config ERROR!"));
      else {
              Serial.print(F("Existing Partition#1 counter: "));
              Serial.println(s1);
      }
    if (!s2) Serial.println(F("Partition#2 config ERROR!"));
      else {
              Serial.print(F("Existing Partition#2 counter: "));
              Serial.println(s2);
      }
    
    s1 = EEPRWL_Main.setVersion(PARTITION_VERSION, HANDLE1);
    s2 = EEPRWL_Main.setVersion(PARTITION_VERSION, HANDLE2);

    Serial.print(F("Latest logical sector Partition#1: "));  
    if (!s1) Serial.println(F("no"));
      else {
              Serial.println(EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1));
      }
    Serial.print(F("Next physical sector Partition#1: "));
    Serial.println(EEPRWL_Main.getCtrlData(nextPhysicalSector, HANDLE1));
    Serial.print(F("Latest logical Sector Partition#2: "));  
    if (!s2) Serial.println(F("no"));
      else {
              Serial.println(EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE2));
      }
    Serial.print(F("Next physical Sector Partition#2: "));
    Serial.println(EEPRWL_Main.getCtrlData(nextPhysicalSector, HANDLE2));

    Serial.println(F("\n\nReading data ..."));
    s1 = EEPRWL_Main.read(0,device_name,HANDLE1,PAYLOAD_SIZE1);
    Serial.print(F("device name: "));Serial.println(device_name);
    s2 = EEPRWL_Main.read(0,time_stamp,HANDLE2);
    Serial.print(F("time stamp: "));Serial.println(time_stamp);
} 


// ----------------------------------------------------
// --- 3. LOOP ---
// ----------------------------------------------------

void loop() {
    
    if(loopCounter > 0){

        snprintf(device_name, PAYLOAD_SIZE1, "device#%d", loopCounter);
        int s1=EEPRWL_Main.write(device_name, HANDLE1);

        time_stamp = millis();
        int s2=EEPRWL_Main.write(time_stamp, HANDLE2);

        Serial.println(F("\nWriting Data ..."));
        Serial.print(F("time stamp: "));Serial.println(time_stamp);
        Serial.print(F("device name: "));Serial.println(device_name);
              
        if(!s1) Serial.println(F("\nWriting >device name< failed."));
          else {
                  Serial.print(F("OK. Next physical sector in Partition#1 >device name<: "));
                  Serial.println(EEPRWL_Main.getCtrlData(nextPhysicalSector, HANDLE1));
          }
        if(!s2) Serial.println(F("Writing >time stamp< failed.")); 
          else {
                  Serial.print(F("OK. Next physical sector in Partition#2 >time stamp<: "));
                  Serial.println(EEPRWL_Main.getCtrlData(nextPhysicalSector, HANDLE2));
          }
        
        Serial.println("\n");
        loopCounter--;
    }
    
    delay(10000); 
}

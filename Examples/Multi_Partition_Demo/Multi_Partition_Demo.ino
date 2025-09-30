/*
 ******************************************************************************
 * EEProm_Safe_Wear_Level Library v25.09.30
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
 ******************************************************************************/

#include <EEProm_Safe_Wear_Level.h>
#include <Arduino.h>

// ----------------------------------------------------
// --- 1. DEFINITIONS AND INSTANCES ---
// ----------------------------------------------------

const uint8_t PARTITION_VERSION = 1;

// The structure for time and date values to be read
struct Timestamp {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
};

// Your four instances
EEProm_Safe_Wear_Level EEPRWL_Time;
EEProm_Safe_Wear_Level EEPRWL_Values1;
EEProm_Safe_Wear_Level EEPRWL_Values2;
EEProm_Safe_Wear_Level EEPRWL_String;

// --- ADDRESSES AND SIZES (UNCHANGED) ---
// The addresses and partition sizes are kept as requested.
const int ADDR_Time = 0;
const int ADDR_Values1 = 256;
const int ADDR_Values2 = 512;
const int ADDR_String = 768;

const size_t SIZE_Time = 256;    // Unchanged
const size_t SIZE_Values1 = 256; // Unchanged
const size_t SIZE_Values2 = 256; // Unchanged
const size_t SIZE_String = 256;  // Unchanged

// --- DATA VARIABLES ---
Timestamp writeTime = {0, 9, 16, 28, 9, 2025};
float writeFloat = 1.23456f;
int32_t writeInteger = -789L;
const char* writeString = "Hello World";

// Read data variables
Timestamp readTime;
float readFloat;
int32_t readInteger;
char readBuffer[12]; // Buffer for "Hello World" (10 chars + '\0')

// Control variable
byte loopCounter = 5;


// --- 4. HELPER FUNCTIONS ---

void print2Digits(int val) {
  if (val < 10) {
    Serial.print("0");
  }
  Serial.print(val);
}

void readAllData() {
    
    // 1. Read Time
    bool s1 = EEPRWL_Time.read(readTime);
    if(!s1) Serial.println(F("No data records found for >Timestamp<."));
    
    // 2. Read Values
    bool s2 = EEPRWL_Values1.read(readFloat);
    if(!s2) Serial.println(F("No data records found for type >Float<."));

    bool s3 = EEPRWL_Values2.read(readInteger);
    if(!s3) Serial.println(F("No data records found for type >Integer<."));
    
    // 3. Read String
    // NOTE: The read() method for strings requires the buffer size for CRC-8 validation.
    bool s4 = EEPRWL_String.read(readBuffer, sizeof(readBuffer));
    if(!s4) Serial.println(F("No data records found for >String<."));

    // Output
    Serial.println("\n--------------- READ DATA ---------------");
    if(!s1||!s2||!s3||!s4) Serial.println(F("--- Random values if data not found ---"));
    Serial.println("");
    Serial.print(F("Timestamp: "));
    print2Digits(readTime.day); Serial.print(".");
    print2Digits(readTime.month); Serial.print(".");
    Serial.print(readTime.year); Serial.print(" ");
    print2Digits(readTime.hour); Serial.print(":");
    print2Digits(readTime.minute); Serial.print(":");
    print2Digits(readTime.second); Serial.println("");
    
    Serial.print(F("Floating Point Value: ")); Serial.println(readFloat, 5);
    Serial.print(F("Integer: ")); Serial.println(readInteger);
    Serial.print(F("String Buffer: ")); Serial.println(readBuffer);
}

// ----------------------------------------------------
// --- 2. SETUP ---
// ----------------------------------------------------

void setup() {
    delay(5000);
    Serial.begin(115200);
    delay(1000);
  #ifdef EEPROMWL_USING_STANDARD_LIB
        // If the macro was defined in the header, we know that NON-AVR is used.
        Serial.println(F("Board: Non-AVR detected (Standard Mode)."));
        Serial.println(F("Status: EEPROM.h (Standard Library) is ACTIVE."));
    #else
        // If the macro was NOT defined, we know that AVR is used.
        Serial.println(F("Board: AVR (e.g., Nano) detected (direct hardware access)."));
        Serial.println(F("Status: Fast AVR Register Code is ACTIVE."));
    #endif

    Serial.println(F("\n--- EEProm_Safe_Wear_Level Demo Start: Multi-Partition ---"));

    // Configuration of all independent areas
    // IMPORTANT: The data size (third parameter) MUST match the type size.
    
    int v1 = EEPRWL_Time.config(ADDR_Time, SIZE_Time, sizeof(Timestamp)); // OK
    int v2 = EEPRWL_Values1.config(ADDR_Values1, SIZE_Values1, sizeof(float));      // OK
    
    // Correction: Set size to int32_t (4 bytes) to ensure consistency.
    int v3 = EEPRWL_Values2.config(ADDR_Values2, SIZE_Values2, sizeof(int32_t)); 
    
    // Correction: Set size to that of the buffer (12 bytes)
    int v4 = EEPRWL_String.config(ADDR_String, SIZE_String, sizeof(readBuffer));

if (!v1) Serial.println(F("Time config ERROR!"));    else {Serial.print(F("Partition 'Time' Override-Counter: "));Serial.println(v1);}
    if (!v2) Serial.println(F("Values1 config ERROR!"));else {Serial.print(F("Partition 'Values1' Override-Counter: "));Serial.println(v2);}
    if (!v3) Serial.println(F("Values2 config ERROR!"));else {Serial.print(F("Partition 'Values2' Override-Counter: "));Serial.println(v3);}
    if (!v4) Serial.println(F("String config ERROR!")); else {Serial.print(F("Partition 'String' Override-Counter: "));Serial.println(v4);}
    
    // Version Control / State Restoration
    if(EEPRWL_Time.setVersion(PARTITION_VERSION))      Serial.println(F("Active partition 'Time' found."));     else Serial.println(F("Partition 'Time' newly created and formatted."));
    if(EEPRWL_Values1.setVersion(PARTITION_VERSION))   Serial.println(F("Active partition 'Values1' found.")); else Serial.println(F("Partition 'Values1' newly created and formatted."));
    if(EEPRWL_Values2.setVersion(PARTITION_VERSION))   Serial.println(F("Active partition 'Values2' found.")); else Serial.println(F("Partition 'Values2' newly created and formatted."));
    if(EEPRWL_String.setVersion(PARTITION_VERSION))    Serial.println(F("Active partition 'String' found."));  else Serial.println(F("Partition 'String' newly created and formatted."));
}

// ----------------------------------------------------
// --- 3. LOOP ---
// ----------------------------------------------------

void loop() {
    
    if(loopCounter > 0){

        readAllData(); 

        Serial.println("\n");
        Serial.println(F("ATTENTION: The following values are minimum numbers obtained through WearLeveling."));
        Serial.println(F("A typical manufacturer specification of 100,000 cycles was used for calculation."));
        Serial.println("");
        Serial.print(F("Remaining Lifetime Partition EEPRWL_Time: ")); Serial.print(EEPRWL_Time.healthPercent());Serial.print("%, remaining cycles: ");Serial.print((float)EEPRWL_Time.healthCycles()/1000000);Serial.println(" million");
        Serial.print(F("Remaining Lifetime Partition EEPRWL_Values1: ")); Serial.print(EEPRWL_Values1.healthPercent());Serial.print("%, remaining cycles: ");Serial.print((float)EEPRWL_Values1.healthCycles()/1000000);Serial.println(" million");
        Serial.print(F("Remaining Lifetime Partition EEPRWL_Values2: ")); Serial.print(EEPRWL_Values2.healthPercent());Serial.print("%, remaining cycles: ");Serial.print((float)EEPRWL_Values2.healthCycles()/1000000);Serial.println(" million");
        Serial.print(F("Remaining Lifetime Partition EEPRWL_String: ")); Serial.print(EEPRWL_String.healthPercent());Serial.print("%, remaining cycles: ");Serial.print((float)EEPRWL_String.healthCycles()/1000000);Serial.println(" million");
        
        // --- WRITE OPERATION ---
        Serial.println(F("\n--- WRITE OPERATION ---"));Serial.println("");
        
        // NOTE: The write() method returns the index of the sector that *will be* written next (index + 1), 
        // or 0 on error. To show the last written sector, we use (sX - 1).
        
        int s1=EEPRWL_Time.write(writeTime);
        if(!s1) Serial.println(F("Writing >Timestamp< failed."));
        else    {Serial.print(F("Last written sector in 'Time': "));Serial.println(s1-1);}      

        int s2=EEPRWL_Values1.write(writeFloat);
        if(!s2) Serial.println(F("Writing >Float< failed."));
        else    {Serial.print(F("Last written sector in 'Values1': "));Serial.println(s2-1);}    

        int s3=EEPRWL_Values2.write(writeInteger);
        if(!s3) Serial.println(F("Writing >Integer< failed."));
        else    {Serial.print(F("Last written sector in 'Values2': "));Serial.println(s3-1);}     
        
        int s4=EEPRWL_String.write(writeString);
        if(!s4) Serial.println(F("Writing >String< failed."));
        else    {Serial.print(F("Last written sector in 'String': "));Serial.println(s4-1);}      

        Serial.println(F("\r\nWrite operation complete."));
        
        loopCounter--;
    }
    
    delay(500); 
}
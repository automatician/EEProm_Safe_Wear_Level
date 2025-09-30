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

const uint8_t PARTITION_VERSION = 2;

// The consolidated data structure for the entire data record
// Total size (approximate sum): 6 + 8 + 12 = 26 Bytes
struct EepromData {
    // 1. Timestamp (6 Bytes)
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
    
    // 2. Values (8 Bytes)
    float floatValue;        // 4 Bytes
    int32_t integerValue;    // 4 Bytes
    
    // 3. String (12 Bytes)
    char stringBuffer[12]; // 12 Bytes (Space for "Hello World" + null termination)
};

// Only ONE instance for the entire EEPROM management
EEProm_Safe_Wear_Level EEPRWL_Main; 

// --- ADDRESSES AND SIZES ---
const int ADDR_Main = 0;
// We use the full 1024 bytes of the EEPROM for the single partition
const size_t SIZE_Main = 1024; 

// --- DATA VARIABLES (Consolidated) ---
EepromData writeData = {
    // Timestamp: sec, min, h, day, month, year
    0, 9, 16, 28, 9, 2025,
    // Values: float, int32_t
    1.23456f,
    -789L,
    // String
    "Hello World"
};
EepromData readData; // A single structure for reading

// Control variable
byte loopCounter = 5;


// --- 4. HELPER FUNCTIONS ---

void print2Digits(int val) {
    if (val < 10) {
        Serial.print(F("0"));
    }
    Serial.print(val);
}

void readAllData() {
    
    // Read the entire data record in one go!
    // We use the template read function which reads the entire structure.
    bool success = EEPRWL_Main.read(readData);
    
    if(!success) {
        Serial.println(F("ERROR: No complete data record found."));
    }

    // Output
    Serial.println(F("\n--------------- READ DATA ---------------"));
    if(!success) Serial.println(F("--- Values are from RAM cache and are invalid ---"));
    Serial.println(F(""));
    
    // Output the Timestamp
    Serial.print(F("Timestamp: "));
    print2Digits(readData.day); Serial.print(F("."));
    print2Digits(readData.month); Serial.print(F("."));
    Serial.print(readData.year); Serial.print(F(" "));
    print2Digits(readData.hour); Serial.print(F(":"));
    print2Digits(readData.minute); Serial.print(F(":"));
    print2Digits(readData.second); Serial.println(F(""));
    
    // Output the values
    Serial.print(F("Floating Point Value: ")); Serial.println(readData.floatValue, 5);
    Serial.print(F("Integer: ")); Serial.println(readData.integerValue);
    Serial.print(F("String Buffer: ")); Serial.println(readData.stringBuffer);
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

    Serial.println(F("\n--- EEProm_Safe_Wear_Level Demo Start: Single Config Struct ---"));

    // Configuration of the SINGLE partition. 
    // PayloadSize is the size of the entire data structure.
    int v = EEPRWL_Main.config(ADDR_Main, SIZE_Main, sizeof(EepromData));
    
    if (!v) Serial.println(F("Main config ERROR!"));
    else {
        Serial.print(F("Partition 'Main' Override-Counter: "));
        Serial.println(v);
    }
    
    // Version Control / State Restoration
    if(EEPRWL_Main.setVersion(PARTITION_VERSION)) {
        Serial.println(F("Active partition 'Main' found."));
    } else {
        Serial.println(F("Partition 'Main' newly created and formatted."));
    }
}

// ----------------------------------------------------
// --- 3. LOOP ---
// ----------------------------------------------------

void loop() {
    
    if(loopCounter > 0){
        
        readAllData(); 
        
        Serial.println(F("\n"));
        Serial.println(F("ATTENTION: The following values are minimum numbers obtained through WearLeveling."));
        Serial.println(F("A typical manufacturer specification of 100,000 cycles was used for calculation."));
        Serial.println(F(""));
        
        // Only one health check
        Serial.print(F("Remaining Lifetime Partition EEPRWL_Main: ")); 
        Serial.print(EEPRWL_Main.healthPercent());
        Serial.print(F("%, remaining cycles: "));
        Serial.print((float)EEPRWL_Main.healthCycles()/1000000);
        Serial.println(F(" million"));
        
        // --- WRITE OPERATION ---
        Serial.println(F("\n--- WRITE OPERATION ---"));Serial.println(F(""));
        
        // Update data structure before writing
        writeData.second = (uint8_t)(millis() / 1000) % 60;
        writeData.floatValue += 0.001f;
        writeData.integerValue++; 
        
        // Write the entire data record in a single call
        // NOTE: The write() method returns the index of the sector that *will be* written next (index + 1), or 0 on error.
        int success = EEPRWL_Main.write(writeData);
        
        if(success==0) Serial.println(F("Writing data record failed."));
        else    {Serial.print(F("Writing data record OK. Last written sector: "));Serial.println(success-1);}
        
        Serial.println(F("\r\nWrite operation complete."));
        
        loopCounter--;
    }
    
    delay(500); 
}
// ##############################################
// ############ Demo2: Datatype INT #############
// ##############################################
// EEProm_Safe_Wear_Level Library v25.10.5
// ##############################################
// Store the >int loopCounter< in the EEPROM and 
// read the counter when the Arduino is restarted.
// ----------------------------------------------
// For simple applications that store pure states 
// or measured values ​​(numbers, Booleans), you can 
// safely use the same pattern as with the int.

#include "EEProm_Safe_Wear_Level.h"

// --- 1. RAM Handle Configuration ---
#define PARTITIONS 1

// Struct for administrative metadata in RAM.
// Alignment (8) is required for efficient CPU access.

typedef struct {
  uint8_t data[16 * PARTITIONS];
} __attribute__((aligned(8))) administrative_control_structure;

// Declaration of the RAM variable that holds the administrative data.
administrative_control_structure PARTITIONS_DATA;

// Library Instantiation: Links the RAM handle with the library.
EEProm_Safe_Wear_Level EEPRWL_Main(PARTITIONS_DATA.data); 

// --- 2. Payload Definitions ---

// The length of the payload in bytes. An 'int' is typically 4 bytes.
#define PAYLOAD_SIZE sizeof(int)

// Counter for the loop: This will be stored.
int loopCounter = 0; 

// Handle ID for the partition used.
#define HANDLE1 0

// --- 3. Setup (Initialization) ---

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("EEPROM Quickstart started."));

  // Address and length definitions for the EEPROM partition:
  #define ADDR_START 0x0000 // Start address in EEPROM
  #define PART_LENGTH 144   // Total partition size (e.g., for 10 records of 12 bytes payload)
  #define COUNTER_LEN 1     // Length of the wear-leveling counter (1 Byte = 256 writes)

  // 1. Configure Partition (Calculates metadata and stores it in the RAM Handle)
  int STATUS = EEPRWL_Main.config(ADDR_START, PART_LENGTH, PAYLOAD_SIZE, COUNTER_LEN, HANDLE1);

  if(STATUS > 0){
    Serial.println(F("Configuration successful.")); 
  } else {
    Serial.println(F("Configuration unsuccessful or partition is empty!"));
  }

  // 2. Set/update the Partition Version.
  // Incrementing the version resets the partition (Reset/Formatting for wear-leveling).
  // We use a fixed number: 2
  EEPRWL_Main.setVersion(2, HANDLE1);

  // 3. Read the last valid int value from the partition and store it in loopCounter.
  // read() is overloaded to process the int value directly.
  if (EEPRWL_Main.read(loopCounter, HANDLE1, PAYLOAD_SIZE)) {
    Serial.print(F("Valid data read, loopCounter starts at: "));
    Serial.println(loopCounter);
  } else {
    Serial.print(F("No data available. Status: 0x"));
    Serial.println(PARTITIONS_DATA.data[13], HEX);
  }
}

// --- 4. Main Loop (Writing) ---

void loop() {
  loopCounter++;

  // Write the int value directly from loopCounter into the partition (HANDLE1).
  // The library automatically determines the size via overloading, 
  // but we provide the value as an int.
  bool error = EEPRWL_Main.write(&loopCounter, HANDLE1);

  if (error == 0) {
    Serial.print(F("WRITE ERROR. Code: 0x"));
    // Direct query of the status code via RAM Handle
    Serial.println(PARTITIONS_DATA.data[13], HEX); 
  } else {
    Serial.print(F("Data written (Sector "));
    Serial.print(EEPRWL_Main.getCtrlData(0, HANDLE1)); 
    Serial.print(F("). Value: ")); // ). Wert:
    Serial.print(loopCounter, DEC);
    Serial.print(F(", Next Sector: ")); 
    Serial.println(EEPRWL_Main.getCtrlData(4, HANDLE1)); 
  }

  delay(5000);
}
// END OF CODE

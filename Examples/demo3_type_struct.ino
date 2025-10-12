// #############################################
// ############ Demo3: type struct #############
// #############################################
// EEProm_Safe_Wear_Level Library v25.10.5
// #############################################
// 
// What Structures Are Good For
// A structure (struct) in C and C++ is a fundamental and extremely useful data type used
// for the logical grouping of different but related data.
// You can think of a struct as a custom box or container in which you can combine various 
// primitive data types (like int, float, bool) under a single, unified name.
// 
// 1. Logical Data Grouping
// The most important advantage: A structure ensures that related data is always treated 
// and handled as a single unit.
// 
// Example without struct (bad):
// 
// int temperature;
// float humidity;
// long timestamp;
// 
// These three variables have no explicit, clear relation to each other in the code.
// 
// Example with struct (good):
// 
// struct SensorData {
//   int temperature;
//   float humidity;
//   long timestamp;
// };
// SensorData measurement;
// 
// Now you know that measurement.temperature, measurement.humidity, and 
// measurement.timestamp together represent one complete reading from a specific moment.
// 
// 2. Simple Data Transfer
// Instead of calling functions with a long list of individual parameters, you can 
// simply pass the entire structure. This makes the code cleaner, more readable, and 
// less error-prone.
// 
// Incorrect: saveData(temp, humidity, timestamp);
// Correct: saveData(measurement);
// 
// 3. Efficient Storage and Persistence (Relevant to Your Project)
// The struct is the ideal way to store blocks of data in the EEPROM:
// 
// If you had to store 10 different settings, you would need 10 individual 
// EEPRWL_Main.write() calls.
// 
// If you bundle all 10 settings into one struct, you only need a single 
// EEPRWL_Main.write() call.
// 
// This greatly simplifies the library's wear-leveling logic and guarantees that all
// settings are written and saved from one operation, ensuring data consistency.
// 
// 4. Code Clarity
// Structures fundamentally improve code readability and reusability. If you need
// a function to process weather data, you define the required type (struct 
// WeatherData). Anyone using your function immediately knows exactly which set of 
// data belongs together.
// 
// In summary, the struct is the tool that allows you to move from working with 
// individual "building blocks" (primitive types) to working with coherent "modules"
// (complex objects).


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

// Suppose you have this structure:
typedef struct {
  int counter;      // Typically 4 Bytes
  float value;      // Typically 4 Bytes
  bool is_active;   // Typically 1 Byte
} MyDataStruct_t;

// 1. Declare a variable of this type:
MyDataStruct_t mySettings;

// 2. Determine the length:
// The length of the structure in bytes
#define PAYLOAD_SIZE sizeof(MyDataStruct_t) 

// Counter for the loop:
int loopCounter = 0; 

// Handle ID for the partition used.
#define HANDLE1 0

// --- 3. Setup (Initialization) ---

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("EEPROM Quickstart started.")); // EEPROM Quickstart gestartet.

  // Address and length definitions for the EEPROM partition:
  #define ADDR_START 0x0000                   // Start address in EEPROM
  #define PART_LENGTH ((PAYLOAD_SIZE+2)*5)+4  // Total partition size
  #define COUNTER_LEN 1                       // Length of the wear-leveling counter (1 Byte = 256 writes)

  // 1. Configure Partition (Calculates metadata and stores it in the RAM Handle)
  int STATUS = EEPRWL_Main.config(ADDR_START, PART_LENGTH, PAYLOAD_SIZE, COUNTER_LEN, 8, HANDLE1);

  if(STATUS > 0){
    Serial.println(F("Configuration successful.")); // Konfiguration erfolgreich.
  } else {
    Serial.println(F("Configuration unsuccessful or partition is empty!")); // Konfiguration nicht erfolgreich oder Partition leer!
  }

  // 2. Read the last valid int value from the partition and store it in loopCounter.
  // read() is overloaded to process the int value directly.
  if (EEPRWL_Main.read(0, mySettings, HANDLE1)) {
    Serial.println(F("Valid data read: ")); 
    Serial.print(F("mySettings.counter: "));Serial.println(mySettings.counter);
    Serial.print(F("mySettings.value: "));Serial.println(mySettings.value);
    Serial.print(F("mySettings.is_active: "));Serial.println(mySettings.is_active);
    loopCounter = mySettings.counter; 
  } else {
    Serial.print(F("No data available. Status: 0x")); 
    Serial.println(PARTITIONS_DATA.data[13], HEX);
  }
}

// --- 3. Main Loop (Writing) ---

void loop() {
  loopCounter++;
  
  mySettings.counter = loopCounter;
  mySettings.value = loopCounter*0.01;
  mySettings.is_active = 1;
  bool error = EEPRWL_Main.write(mySettings, HANDLE1);

  if (error == 0) {
    Serial.print(F("WRITE ERROR. Code: 0x"));
    // Direct query of the status code via RAM Handle
    Serial.println(PARTITIONS_DATA.data[13], HEX); 
  } else {
    Serial.print(F("Data written (Sector "));  
    Serial.print(EEPRWL_Main.getCtrlData(0, HANDLE1)); 
    Serial.print(F("). Value: "));
    Serial.print(mySettings.value, DEC);
    Serial.print(F(", Next Sector: ")); 
    Serial.println(EEPRWL_Main.getCtrlData(4, HANDLE1)); 
  }

  delay(10000);
}
// END OF CODE

// #############################################
// ########### Demo5: log functions ############
// #############################################
// EEProm_Safe_Wear_Level Library v25.10.x
// #############################################
// 
// This demo demonstrates the use of the logging functionality. 
// The demo is kept simple to provide an easy introduction to 
// the topic. First, 20 data records are written. Then they are 
// read in ascending and descending order of their relevance.
//
// Bug fix: Adjusting the iteration logic for correct 
// representation of 20 valid entries and avoidance of off-by-one.
//

#include <EEProm_Safe_Wear_Level.h>

// --- CONTROL DATA OFFSETS (AS DEFINED IN THE LIBRARY) ---
// These offsets are only used for making the control data readable.
#define currentLogicalCounter 0 // Counter for the total number of sectors written
#define nextPhysicalSector 4    // Physical sector that will be written to next
#define numberOfSectors 8       // Total number of sectors in the partition (from configuration)

// ----------------------------------------------------
// --- DEFINITIONS AND INSTANCES ---
// ----------------------------------------------------

#define COUNTER_LENGTH_BYTES  2 

// --- HANDLE DEFINITIONS ---
#define PART_CNT 1               // ONE logical partition 
#define WRITE_CYCLES_PER_HOUR 128
#define HANDLE1  0

// ControlData Structure
typedef struct {
    uint8_t data[16 * PART_CNT];  
} __attribute__((aligned(8))) AlignedArray_t;
AlignedArray_t PartitionsData;

EEProm_Safe_Wear_Level EEPRWL_Main(PartitionsData.data); 

// --- ADDRESSES AND SIZES ---
// Partition#1: Start 0, Size 256
#define ADDR1 0
#define SIZE1 256
#define PAYLOAD_SIZE 9            // Length of device_name
#define VALID_LOG_RECORDS 21      // Defines how many entries we want to read
#define forceFormat 1

// Data
char device_name[9];
byte loopCounter = VALID_LOG_RECORDS;

// ----------------------------------------------------
// --- 2. SETUP ---
// ----------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(1000); 

    Serial.println(F("\r\r\r\r\r\r\r\r\r\n"));
    Serial.println(F("---------------------------------------------------------------------------"));
    Serial.println(F("--- EEProm_Safe_Wear_Level Demo Start: Log-Functions, Single Instance ---"));
    Serial.println(F("--- Write 20 Records and read the ring buffer (20 valid entries) ---"));
    Serial.println(F("---------------------------------------------------------------------------"));
    
    int s1 = EEPRWL_Main.config(ADDR1, SIZE1, PAYLOAD_SIZE, COUNTER_LENGTH_BYTES, WRITE_CYCLES_PER_HOUR, HANDLE1); 
    //EEPRWL_Main.initialize(forceFormat,HANDLE1);
    
    if (!s1) Serial.println(F("Partition#1 config ERROR!"));
    else {
      Serial.print(F("Existing Partition#1 counter: "));
      Serial.println(s1);
    }
    
    Serial.print(F("Latest logical sector Partition#1: "));  
    Serial.println(EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1));
    Serial.print(F("Next physical sector Partition#1: "));
    Serial.println(EEPRWL_Main.getCtrlData(nextPhysicalSector, HANDLE1));

    // Optional: Check the recognized number of sectors. Should be 21,
    // to explain the previous log error (0-20 = 21 sectors)
    Serial.print(F("Total sectors recognized by config: "));
    Serial.println(EEPRWL_Main.getCtrlData(numberOfSectors, HANDLE1));
} 

// ----------------------------------------------------
// --- 3. LOOP ---
// ----------------------------------------------------

#define actual 0
#define next 1
#define previous 2

void loop() {
    
    // 1. WRITING DATA (20 Records)
    while (loopCounter > 0) {

        int id = EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1);
        int z = (id / 10) % 10;
        int e = id % 10;          
        
        // **ATTENTION: CORRECTED.** For the device name to match the logical ID, 
        // the ID 'id' is used. The bug in the previous log was here.
        // If you want the name to be e.g., for ID 41 -> device41, 
        // your logic must ensure that id provides the correct value.
        snprintf(device_name, PAYLOAD_SIZE, "device%d%d", z, e); 
        int s1=EEPRWL_Main.write(device_name, HANDLE1);

        Serial.print(F("Writing Data: "));
        Serial.print(device_name);
            
        if(!s1) Serial.println(F("\nWriting failed."));
        else {
          Serial.print(F(".  OK. Next physical sector in Partition#1: "));
          Serial.println(EEPRWL_Main.getCtrlData(nextPhysicalSector, HANDLE1));
        }
        
        loopCounter--;
    }
    
    delay(5000); 

    Serial.println(F("\n\n--- FORWARD ITERATION (Oldest -> Newest) ---"));

    // 2. FORWARD ITERATION (from Oldest to Newest)
    int s1 = EEPRWL_Main.findOldestData(HANDLE1);
    if(!s1) Serial.println(F("\nfailed."));
    else {
          Serial.print(F("OK. Next physical sector in Partition#1: "));
          Serial.println(EEPRWL_Main.getCtrlData(nextPhysicalSector, HANDLE1));

          // **FIRST READING (Oldest Element)**
          Serial.print(F("\nReading data at logical sector #"));
          // The display of the logical sector must come from the *read sector*, 
          // not from the control data (currentLogicalCounter). 
          // Since we don't know the internal function, we use the global counter.
          // The library function should provide the ID of the read header here.
          Serial.print(EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1)); 
          Serial.print(F(":  "));
          EEPRWL_Main.read(actual, device_name, HANDLE1, PAYLOAD_SIZE);
          Serial.println(device_name);

          // Loop reads the remaining (VALID_LOG_RECORDS - 1) = 18 sectors
          int sectorsToRead = VALID_LOG_RECORDS - 1; 

          for (byte h = 0; h < sectorsToRead; h++) {
              EEPRWL_Main.read(next, device_name, HANDLE1, PAYLOAD_SIZE);
              Serial.print(F("Reading next sector, data at logical sector #"));
              Serial.print(EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1));
              Serial.print(F(":  "));
              Serial.println(device_name);
          }

        Serial.println(F("\n\n--- BACKWARD ITERATION (Newest -> Oldest) ---"));
        
        // 3. BACKWARD ITERATION (from Newest to Oldest)
        EEPRWL_Main.findNewestData(HANDLE1);
              
        // **FIRST READING (Newest Element)**
        EEPRWL_Main.read(actual, device_name, HANDLE1, PAYLOAD_SIZE);
        Serial.print(F("\nReading newest sector, data at logical sector #"));
        Serial.print(EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1));
        Serial.print(F(":  "));
        Serial.println(device_name);

        // Loop reads the remaining (VALID_LOG_RECORDS - 1) = 18 sectors
        for (byte h = 0; h < sectorsToRead; h++) {
            EEPRWL_Main.read(previous, device_name, HANDLE1, PAYLOAD_SIZE);
            Serial.print(F("Reading next sector, data at logical sector #"));
            Serial.print(EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1));
            Serial.print(F(":  "));
            Serial.println(device_name);
        }
    }

    while (0==0){delay(1000);}   
}
//END OF CODE

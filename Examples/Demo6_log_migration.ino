// #############################################
// ########### Demo6: log migration ############
// #############################################
// EEProm_Safe_Wear_Level Library v25.10.5
// #############################################
// 
// Demonstrates the migration of sectors to a 
// second partition starting with a new logical counter.
//
// This demo builds on previous ones. Please understand that a
// basic understanding from those other demos is a prerequisite.
//

#include <EEProm_Safe_Wear_Level.h>

// ----------------------------------------------------
// --- DEFINITIONS AND INSTANCES ---
// ----------------------------------------------------

#define COUNTER_LENGTH_BYTES  2 // WARNING Max is 4

// --- HANDLE DEFINITIONS ---
#define PART_CNT 2                      //Two logical partitions 

// --- Write cycles for Write Load Management
#define WRITE_CYCLES_PER_HOUR 64

// Unique handle IDs for the ONE instance
// consecutive numbering
#define HANDLE1  0
#define HANDLE2  1

// The ControlData structure must provide space for 2 partitions (2 * 16 Bytes = 64 Bytes)
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
#define counterByteLength 11    // Size of a single sector (Payload + Metadata)
#define status 14               // Status Byte
#define checksum 15             // Checksum for the control data

//Instanz von EEProm_Safe_Wear_Level erzeugen
EEProm_Safe_Wear_Level EEPRWL_Main(PartitionsData.data); 

// --- ADDRESSES AND SIZES ---
// Partition#1: 0 - 255
#define ADDR1 0
#define ADDR2 128
#define SIZE1 128
#define SIZE2 128

// Payload Sizes
#define PAYLOAD_SIZE 9          //length of device_name
// Data
char device_name[9];            // String Array (safe)

// Control variable
byte loopCounter = 4;

// ----------------------------------------------------
// --- 2. SETUP ---
// ----------------------------------------------------
#define actual 0
#define next 1
#define previous 2
#define oldest 3
#define newest 4
#define count 10
#define forceFormat 1

void setup() {
    Serial.begin(115200);
    delay(1000); 

    Serial.println(F("\r\r\r\r\r\r\r\r\r\n"));
    Serial.println(F("---------------------------------------------------------------------------"));
    Serial.println(F("--- EEProm_Safe_Wear_Level Demo Start:  Log Migration,  Single Instance ---"));
    Serial.println(F("---------------------------------------------------------------------------"));
    
    int s1 = EEPRWL_Main.config(ADDR1, SIZE1, PAYLOAD_SIZE, COUNTER_LENGTH_BYTES, WRITE_CYCLES_PER_HOUR, HANDLE1); 
    int s2 = EEPRWL_Main.config(ADDR2, SIZE2, PAYLOAD_SIZE, COUNTER_LENGTH_BYTES, WRITE_CYCLES_PER_HOUR, HANDLE2); 
    // EEPRWL_Main.initialize(forceFormat, HANDLE1);
    EEPRWL_Main.initialize(forceFormat, HANDLE2);
   
    if (!s1||!s2) Serial.println(F("Partition#1 config ERROR!"));
    else {
      Serial.print(F("Existing Partition#1 counter: "));
      Serial.println(s1);
    }
    
    Serial.print(F("Latest logical sector Partition#1: "));  
    Serial.println(EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1));
    Serial.print(F("Next physical sector Partition#1: "));
    Serial.println(EEPRWL_Main.getCtrlData(nextPhysicalSector, HANDLE1));
} 

void readPartition(int handle) {
              EEPRWL_Main.read(actual,device_name,handle,PAYLOAD_SIZE);
              Serial.print(F("\n\nReading data at logical sector #"));
              Serial.print(EEPRWL_Main.getCtrlData(currentLogicalCounter, handle));
              Serial.print(F(":  "));
              Serial.println(device_name);

          for (byte h = 0; h < 4; h++) {
              EEPRWL_Main.read(next,device_name,handle,PAYLOAD_SIZE);
              Serial.print(F("\nReading next sector, data at logical sector #"));
              Serial.print(EEPRWL_Main.getCtrlData(currentLogicalCounter, handle));
              Serial.print(F(":  "));
              Serial.print(device_name);
          }
}


// ----------------------------------------------------
// --- 3. LOOP ---
// ----------------------------------------------------

void loop() {
    
    while (loopCounter > 0) {

        int id =  EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1);
        int z = (id / 10) % 10;
        int e = id % 10;        
        snprintf(device_name, PAYLOAD_SIZE, "device%d%d", z, e);
        int s1 = EEPRWL_Main.write(device_name, HANDLE1);

        Serial.print(F("Writing Data: "));
        Serial.print(device_name);
              
        if(!s1) Serial.println(F("\nWriting failed."));
        else {
          Serial.print(F(".  OK. Next physical sector in Partition#1: "));
          Serial.println(EEPRWL_Main.getCtrlData(nextPhysicalSector, HANDLE1));
        }
        
        loopCounter--;
    }
    
    delay(2500); 

    Serial.println(F("\n\n--- Data migration to second partition ---"));
    Serial.println(F("migrateData(uint8_t handle1, uint8_t handle2, uint16_t count)"));
    if (EEPRWL_Main.migrateData(HANDLE1, HANDLE2, 5)==false) Serial.println("Error");
    else {
        Serial.println(F("\n\n-----  Partition #1  -----"));
        int s1 = EEPRWL_Main.findOldestData(HANDLE1);
        if(!s1) Serial.println(F("\nfailed."));
        else {
              Serial.print(F("findOldestData 4 records: OK. Next physical sector in Partition#1: "));
              Serial.print(EEPRWL_Main.getCtrlData(nextPhysicalSector, HANDLE1));
              readPartition(HANDLE1);
        }

        Serial.println(F("\n\n-----  Partition #2  -----"));
        s1 = EEPRWL_Main.findOldestData(HANDLE2);
        if(!s1) Serial.println(F("\nfailed."));
        else {
              Serial.print(F("findOldestData 4 records: OK.  Next physical sector in Partition#2: "));
              Serial.print(EEPRWL_Main.getCtrlData(nextPhysicalSector, HANDLE2));
              readPartition(HANDLE2);
        }
    }

    while (0==0){delay(1000);}
}
// END OF CODE

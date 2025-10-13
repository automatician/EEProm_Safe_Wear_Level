// #############################################
// ########### Demo5: log functions ############
// #############################################
// EEProm_Safe_Wear_Level Library v25.10.5
// #############################################
// 
// This demo demonstrates the use of the logging functionality. 
// The demo is kept simple to provide an easy introduction to 
// the topic. First, 20 data records are written. Then they are 
// read in ascending and descending order of their relevance.
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
#define PART_CNT 1                      //Two logical partitions 

// --- Write cycles for Write Load Management
#define WRITE_CYCLES_PER_HOUR 32

// Unique handle IDs for the ONE instance
// consecutive numbering
#define HANDLE1  0

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
#define SIZE1 256

// Payload Sizes
#define PAYLOAD_SIZE 9          //length of device_name
// Data
char device_name[9];            // String Array (safe)

// Control variable
byte loopCounter = 20;

// ----------------------------------------------------
// --- 2. SETUP ---
// ----------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(1000); 

    Serial.println(F("\r\r\r\r\r\r\r\r\r\n"));
    Serial.println(F("---------------------------------------------------------------------------"));
    Serial.println(F("--- EEProm_Safe_Wear_Level Demo Start:  Log-Functions,  Single Instance ---"));
    Serial.println(F("---------------------------------------------------------------------------"));
    
    int s1 = EEPRWL_Main.config(ADDR1, SIZE1, PAYLOAD_SIZE, COUNTER_LENGTH_BYTES, WRITE_CYCLES_PER_HOUR, HANDLE1); 
   
    if (!s1) Serial.println(F("Partition#1 config ERROR!"));
    else {
      Serial.print(F("Existing Partition#1 counter: "));
      Serial.println(s1);
    }
    
    Serial.print(F("Latest logical sector Partition#1: "));  
    Serial.println(EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1));
    Serial.print(F("Next physical sector Partition#1: "));
    Serial.println(EEPRWL_Main.getCtrlData(nextPhysicalSector, HANDLE1));
} 


// ----------------------------------------------------
// --- 3. LOOP ---
// ----------------------------------------------------

#define actual 0
#define next 1
#define previous 2
#define oldest 3
#define newest 4

void loop() {
    
    while (loopCounter > 0) {

        int id =  EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1);
        int z = (id / 10) % 10;
        int e = id % 10;        
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

    Serial.println(F("\n\n--- Log is writing, navigate to the log beginning (oldest sector in the log) ---"));
    Serial.println(F("\n--- Finding the oldest data and forward iteration ---"));

    int s1 = EEPRWL_Main.findOldestData(HANDLE1);
    if(!s1) Serial.println(F("\nfailed."));
    else {
          Serial.print(F("OK. Next physical sector in Partition#1: "));
          Serial.println(EEPRWL_Main.getCtrlData(nextPhysicalSector, HANDLE1));

              Serial.print(F("\n\nReading data at logical sector #"));
              Serial.print(EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1));
              Serial.print(F(":  "));
              EEPRWL_Main.read(actual,device_name,HANDLE1,PAYLOAD_SIZE);
              Serial.println(device_name);

          for (byte h = 0; h < 20; h++) {
              EEPRWL_Main.read(next,device_name,HANDLE1,PAYLOAD_SIZE);
              Serial.print(F("\nReading next sector, data at logical sector #"));
              Serial.print(EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1));
              Serial.print(F(":  "));
              Serial.print(device_name);
          }

        Serial.println(F("\n\n\n--- Finding the newest data and backward iteration ---"));
        EEPRWL_Main.findNewestData(HANDLE1);
              EEPRWL_Main.read(actual,device_name,HANDLE1,PAYLOAD_SIZE);
              Serial.print(F("\nReading newest sector, data at logical sector #"));
              Serial.print(EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1));
              Serial.print(F(":  "));
              Serial.print(device_name);

          for (byte h = 0; h < 19; h++) {
              EEPRWL_Main.read(previous,device_name,HANDLE1,PAYLOAD_SIZE);
              Serial.print(F("\nReading next sector, data at logical sector #"));
              Serial.print(EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1));
              Serial.print(F(":  "));
              Serial.print(device_name);
          }
    }

    while (0==0){delay(1000);}
   
}

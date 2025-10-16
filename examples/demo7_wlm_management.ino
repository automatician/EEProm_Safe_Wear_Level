// #############################################
// ########## Demo7: WLM management ############
// #############################################
// EEProm_Safe_Wear_Level Library v25.10.x
// #############################################
// 
// Control of a partition's WLM (Write Load Management) account
// and its impact.
//
// This demo builds on previous ones. Please understand that a
// basic understanding from those other demos is a prerequisite.
//

#include <EEProm_Safe_Wear_Level.h>

//Offset Definitions for PartitionsData
#define currentLogicalCounter 0 // Counter for the total number of sectors written
#define nextPhysicalSector 4    // Physical sector that will be written to next
#define startAddress 6          // Start address of the partition in the EEPROM
#define payloadSize 8           // Size of the pure payload data (EepromData)
#define numberOfSectors 10      // Total number of available sectors in the partition
#define counterByteLength 11    // Size of a single sector (Payload + Metadata)
#define status 14               // Status Byte
#define checksum 15             // Checksum for the control data

// ----------------------------------------------------
// --- DEFINITIONS AND INSTANCES ---
// ----------------------------------------------------

#define COUNTER_LENGTH_BYTES  3 // WARNING Max is 4

// --- HANDLE DEFINITIONS ---
#define PART_CNT 1                      //One logical partitions 

// --- Write cycles for Write Load Management
#define WRITE_CYCLES_PER_HOUR 1

// Unique handle IDs for the ONE instance
// consecutive numbering
#define HANDLE1  0

// The ControlData structure must provide space for 2 partitions (2 * 16 Bytes = 64 Bytes)
typedef struct {
    uint8_t data[16 * PART_CNT];  
} __attribute__((aligned(8))) AlignedArray_t;
AlignedArray_t PartitionsData;

//Instanz von EEProm_Safe_Wear_Level erzeugen
EEProm_Safe_Wear_Level EEPRWL_Main(PartitionsData.data); 

// --- ADDRESSES AND SIZES ---
// Partition#1: 0 - 255
#define ADDR1 0
#define SIZE1 1024

// Payload Sizes
#define PAYLOAD_SIZE 1          //length of loopCounter

// Control variable
byte loopCounter = 144;

// ----------------------------------------------------
// --- 2. SETUP ---
// ----------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(1000); 

    Serial.println(F("\r\r\r\r\r\r\r\r\r\n"));
    Serial.println(F("---------------------------------------------------------------------------"));
    Serial.println(F("--- EEProm_Safe_Wear_Level Demo Start:   WLM management                 ---"));
    Serial.println(F("---------------------------------------------------------------------------"));
    
    int s1 = EEPRWL_Main.config(ADDR1, SIZE1, PAYLOAD_SIZE, COUNTER_LENGTH_BYTES, WRITE_CYCLES_PER_HOUR, HANDLE1); 
   
    if (!s1) Serial.println(F("Partition#1 config ERROR!"));
    else {
      Serial.print(F("Existing Partition#1 counter: "));
      Serial.println(s1);
    }
    
    Serial.print(F("\nLatest logical sector Partition#1: "));  
    Serial.println(EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1));
    Serial.print(F("Next physical sector Partition#1: "));
    Serial.println(EEPRWL_Main.getCtrlData(nextPhysicalSector, HANDLE1));
} 

// ----------------------------------------------------
// --- 3. LOOP ---
// ----------------------------------------------------

void loop() {
    
    while (loopCounter > 0) {

        int s1 = EEPRWL_Main.write(loopCounter, HANDLE1);

        Serial.print(F("Writing Data: "));
        Serial.print(loopCounter);
              
        if(!s1){
          Serial.println(F("\nWriting failed."));
        }
        
        Serial.print(F("\nLoop Counter: "));
        Serial.println(loopCounter);

        Serial.print(F("\nWLM Write Account Balance: "));
        Serial.println(EEPRWL_Main.getWrtAccBalance(HANDLE1));
        
        Serial.print(F("\nSticky Status Byte: "));
        Serial.println(EEPRWL_Main.getCtrlData(14,HANDLE1));
        Serial.println(F("---------------------------------------------------------------------------"));
       
        loopCounter--;
    }
 }
//END OF CODE

// #############################################
// ########### Demo6: log migration ############
// #############################################
// EEProm_Safe_Wear_Level Library v25.10.5
// #############################################
// 
// Demonstrates the migration of the LATEST 5 sectors to a 
// second partition starting with a new logical counter.
//
// This demo focuses on demonstrating the successful migration and 
// data integrity, minimizing complex Ring Buffer output.
//

#include <EEProm_Safe_Wear_Level.h>

// ----------------------------------------------------
// --- DEFINITIONS AND INSTANCES ---
// ----------------------------------------------------

#define COUNTER_LENGTH_BYTES  2 

// --- HANDLE DEFINITIONS ---
#define PART_CNT 2                       // Two logical partitions 

// --- Write cycles for Write Load Management
#define WRITE_CYCLES_PER_HOUR 64

// Unique handle IDs for the TWO instances
#define HANDLE1  0
#define HANDLE2  1

// The ControlData structure must provide space for 2 partitions (2 * 16 Bytes)
typedef struct {
    uint8_t data[16 * PART_CNT];  
} __attribute__((aligned(8))) AlignedArray_t;
AlignedArray_t PartitionsData;

//Offset Definitions for PartitionsData (nur die für die Initialisierung benötigten)
#define currentLogicalCounter 0
#define nextPhysicalSector 4 

//Instanz von EEProm_Safe_Wear_Level erzeugen
EEProm_Safe_Wear_Level EEPRWL_Main(PartitionsData.data); 

// --- ADDRESSES AND SIZES ---
#define ADDR1 0
#define ADDR2 128
#define SIZE1 128
#define SIZE2 128

// Payload Sizes
#define PAYLOAD_SIZE 9           // Length of device_name (e.g., "deviceXX\0")
// Data
char device_name[9];             // String Array

// Control variable: Set to 5 for consistent write/migrate/read count
byte loopCounter = 5; 

// ----------------------------------------------------
// --- 2. SETUP ---
// ----------------------------------------------------
#define actual 0
#define next 1

void setup() {
    Serial.begin(115200);
    delay(1000); 

    Serial.println(F("\r\r\r\r\r\r\r\r\r\n"));
    Serial.println(F("---------------------------------------------------------------------------"));
    Serial.println(F("--- EEProm_Safe_Wear_Level Demo Start:  Log Migration,  Single Instance ---"));
    Serial.println(F("---              WRITES, MIGRATES AND READS 5 RECORDS                   ---"));
    Serial.println(F("---------------------------------------------------------------------------"));
    
    // Konfiguration beider Partitionen
    int s1 = EEPRWL_Main.config(ADDR1, SIZE1, PAYLOAD_SIZE, COUNTER_LENGTH_BYTES, WRITE_CYCLES_PER_HOUR, HANDLE1); 
    int s2 = EEPRWL_Main.config(ADDR2, SIZE2, PAYLOAD_SIZE, COUNTER_LENGTH_BYTES, WRITE_CYCLES_PER_HOUR, HANDLE2); 
    
    // Partition 2 wird formatiert (um einen sauberen Migrationsstart zu gewährleisten)
    EEPRWL_Main.initialize(1, HANDLE2); // forceFormat=1
    
    if (!s1 || !s2) Serial.println(F("Partition config ERROR!"));
    else {
      Serial.print(F("Existing Partition#1 counter: "));
      Serial.println(s1);
    }
    
    Serial.print(F("Latest logical sector Partition#1: "));  
    Serial.println(EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1));
    Serial.print(F("Next physical sector Partition#1: "));
    Serial.println(EEPRWL_Main.getCtrlData(nextPhysicalSector, HANDLE1));
} 

// ************ VERIFIZIERUNGSFUNKTION ************
void readAndVerifyPartition(int handle, uint8_t countToRead, const char* label) {
    // 1. Setze auf den ältesten Sektor (Startpunkt der logischen Historie)
    int s = EEPRWL_Main.findOldestData(handle);
    if (!s) { Serial.print(label); Serial.println(F(" Reading failed (findOldestData).")); return; }
    
    // 2. Lese den ältesten (ersten) migrierten/verbliebenen Record
    EEPRWL_Main.read(0, device_name, handle, PAYLOAD_SIZE); // read(actual)
    Serial.print(label);
    Serial.print(F(" Log START (Oldest): "));
    Serial.print(device_name);
    Serial.print(F(" (Log #"));
    Serial.print(EEPRWL_Main.getCtrlData(currentLogicalCounter, handle));
    Serial.println(F(")"));

    // 3. Spule zum neuesten (letzten) Record vor
    for (byte h = 0; h < countToRead - 1; h++) {
        EEPRWL_Main.read(next, device_name, handle, PAYLOAD_SIZE);
    }

    // 4. Lese den neuesten Record (Ende der Historie)
    Serial.print(label);
    Serial.print(F(" Log END (Newest): "));
    Serial.print(device_name);
    Serial.print(F(" (Log #"));
    Serial.print(EEPRWL_Main.getCtrlData(currentLogicalCounter, handle));
    Serial.println(F(")"));
}
// ********************************************

// ----------------------------------------------------
// --- 3. LOOP ---
// ----------------------------------------------------

void loop() {
    uint16_t recordsToOperate = 5; // Konsistente Anzahl

    // 1. SCHREIBEN (5 Sektoren)
    while (loopCounter > 0) {
        // Logik zur Erstellung eines eindeutigen device_name basierend auf dem logischen Zähler
        int id =  EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1);
        int z = (id / 10) % 10;
        int e = id % 10;          
        snprintf(device_name, PAYLOAD_SIZE, "device%d%d", z, e); 
        
        int s1 = EEPRWL_Main.write(device_name, HANDLE1);

        Serial.print(F("Writing Data: "));
        Serial.print(device_name);
              
        if(!s1) Serial.println(F(". ERROR!"));
        else Serial.println(F(". OK.")); // Vereinfachte Ausgabe
        
        loopCounter--;
    }
    
    delay(2500);  

    Serial.println(F("\n\n--- DATA MIGRATION ---"));
    Serial.print(F("Migrating "));
    Serial.print(recordsToOperate);
    Serial.print(F(" LATEST records from HANDLE1 to HANDLE2... "));
    
    // 2. MIGRATION & Ergebnisprüfung
    if (EEPRWL_Main.migrateData(HANDLE1, HANDLE2, recordsToOperate) == false) {
         Serial.println(F("ERROR: Migration Failed!"));
    } else {
         Serial.println(F("SUCCESS: Migration Complete."));
         
         // 3. VERIFIZIERUNG DES ERGEBNISSES (Konzentriert)
         Serial.println(F("\n--- VERIFICATION: Log Integrity Check ---"));
         
         // P#1 soll die ÄLTESTEN 5 Records zeigen (die nicht migriert wurden)
         readAndVerifyPartition(HANDLE1, recordsToOperate, "[P#1 Old Data]:");
         
         // P#2 soll die NEUESTEN 5 Records zeigen (die migriert wurden)
         readAndVerifyPartition(HANDLE2, recordsToOperate, "[P#2 New Data]:");
    }

    while (0==0){delay(1000);}
}
//END OF CODE
// END OF CODE

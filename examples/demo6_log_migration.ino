// #############################################
// ########### Demo6: log migration ############
// #############################################
// EEProm_Safe_Wear_Level Library v25.10.x
// #############################################
// 
// Demonstrates the migration of the LATEST 5 sectors and 
// highlights the WLM (Write Load Management) budget effect.
//

#include <EEProm_Safe_Wear_Level.h>

// ----------------------------------------------------
// --- DEFINITIONS AND INSTANCES ---
// ----------------------------------------------------

#define COUNTER_LENGTH_BYTES  2 

// --- HANDLE DEFINITIONS ---
#define PART_CNT 2
#define WRITE_CYCLES_PER_HOUR 6
#define HANDLE1  0
#define HANDLE2  1

// WLM-OFFSET DEFINITION (Corrected to Offset 12 for _buckCyc)
#define WLM_BUCK_CYC 12         // The counter for the current write cycles (_buckCyc)
#define currentLogicalCounter 0
#define nextPhysicalSector 4  
#define PAYLOAD_SIZE 9            

// The ControlData structure must provide space for 2 partitions (2 * 16 Bytes)
typedef struct {
    uint8_t data[16 * PART_CNT];  
} __attribute__((aligned(8))) AlignedArray_t;
AlignedArray_t PartitionsData;

EEProm_Safe_Wear_Level EEPRWL_Main(PartitionsData.data); 

// --- ADDRESSES AND SIZES ---
#define ADDR1 0
#define ADDR2 128
#define SIZE1 128
#define SIZE2 128

char device_name[9];
byte loopCounter = 5; 

// ----------------------------------------------------
// --- 2. SETUP ---
// ----------------------------------------------------
#define actual 0
#define next 1

void setup() {
    Serial.begin(115200);
    delay(1000); 

    Serial.println(F("\n\n\n"));
    Serial.println(F("---------------------------------------------------------------------------"));
    Serial.println(F("--- EEProm_Safe_Wear_Level Demo Start: Log Migration & WLM Focus ---"));
    Serial.println(F("--- WRITES, MIGRATES (5 RECORDS) AND SHOWS BUCK CYCLES CHANGE ---"));
    Serial.println(F("---------------------------------------------------------------------------"));
    
    int s1 = EEPRWL_Main.config(ADDR1, SIZE1, PAYLOAD_SIZE, COUNTER_LENGTH_BYTES, WRITE_CYCLES_PER_HOUR, HANDLE1); 
    int s2 = EEPRWL_Main.config(ADDR2, SIZE2, PAYLOAD_SIZE, COUNTER_LENGTH_BYTES, WRITE_CYCLES_PER_HOUR, HANDLE2); 
    
    EEPRWL_Main.initialize(1, HANDLE2); // forceFormat=1
    
    Serial.print(F("Existing Partition#1 counter: "));
    Serial.println(s1);
    
    Serial.print(F("Latest logical sector Partition#1: "));  
    Serial.println(EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1));

    // WLM Initial Status (New) - Must be read BEFORE the writes
    Serial.println(F("\n--- WLM INITIAL WLM ACCOUNT ---"));
    // Read the Buck Cycle Counter (Offset 12)
    Serial.print(F("P#1 WLM Account (Init): "));
    Serial.println(EEPRWL_Main.getWrtAccBalance(HANDLE1));
    Serial.print(F("P#2 WLM Account (Init): "));
    Serial.println(EEPRWL_Main.getWrtAccBalance(HANDLE2));
    Serial.println("");
} 

// ************ VERIFICATION FUNCTIONS (Unchanged) ************
void readAndVerifyPartition(int handle, uint8_t countToRead, const char* label) {
    int s = EEPRWL_Main.findOldestData(handle);
    if (!s) { Serial.print(label); Serial.println(F(" Reading failed (findOldestData).")); return; }
    
    EEPRWL_Main.read(0, device_name, handle, PAYLOAD_SIZE);
    Serial.print(label);
    Serial.print(F(" Log START (Oldest): "));
    Serial.print(device_name);
    Serial.print(F(" (Log #"));
    Serial.print(EEPRWL_Main.getCtrlData(currentLogicalCounter, handle));
    Serial.println(F(")"));

    for (byte h = 0; h < countToRead - 1; h++) {
        EEPRWL_Main.read(next, device_name, handle, PAYLOAD_SIZE);
    }

    Serial.print(label);
    Serial.print(F(" Log END (Newest): "));
    Serial.print(device_name);
    Serial.print(F(" (Log #"));
    Serial.print(EEPRWL_Main.getCtrlData(currentLogicalCounter, handle));
    Serial.println(F(")"));
}

void readNewestData(int handle) {
    EEPRWL_Main.read(actual, device_name, handle, PAYLOAD_SIZE); 
    Serial.print(F("[P#2 Direct Access]: Newest Record is "));
    Serial.print(device_name);
    Serial.print(F(" (Log #"));
    Serial.print(EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE2));
    Serial.println(F(")"));
}
// ********************************************

// ----------------------------------------------------
// --- 3. LOOP ---
// ----------------------------------------------------

void loop() {
    uint16_t recordsToOperate = 5; 

    // 1. WRITING (5 sectors)
    while (loopCounter > 0) {
        int id =  EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1);
        int z = (id / 10) % 10;
        int e = id % 10;          
        snprintf(device_name, PAYLOAD_SIZE, "device%d%d", z, e); 
        
        int s1 = EEPRWL_Main.write(device_name, HANDLE1);

        Serial.print(F("Writing Data: "));
        Serial.print(device_name);
        if(!s1) Serial.println(F(". ERROR!"));
        else Serial.println(F(". OK."));
        
        loopCounter--;
    }
    
    delay(2500);  

    Serial.println(F("\n\n--- DATA MIGRATION ---"));
    Serial.print(F("Migrating "));
    Serial.print(recordsToOperate);
    Serial.print(F(" LATEST records from HANDLE1 to HANDLE2... "));
    
    // 2. MIGRATION & Result Check
    if (EEPRWL_Main.migrateData(HANDLE1, HANDLE2, recordsToOperate) == false) {
         Serial.println(F("ERROR: Migration Failed!"));
    } else {
         Serial.println(F("SUCCESS: Migration Complete."));
         
         // 3. VERIFICATION OF RESULT
         Serial.println(F("\n--- VERIFICATION: Log Integrity Check ---"));
         readAndVerifyPartition(HANDLE1, recordsToOperate, "[P#1 Old Data]:");
         readAndVerifyPartition(HANDLE2, recordsToOperate, "[P#2 New Data]:");
         
         // 4. PROOF OF DIRECT ACCESS
         Serial.println(F("\n--- DIRECT ACCESS PROOF (read actual) ---"));
         readNewestData(HANDLE2);
    }

    // 5. FINAL WLM BUCK CYCLES (Correction: getWrtAccBalance() to trigger accounting)
    
    // Calling getWrtAccBalance() should trigger the internal WLM accounting.

    Serial.println(F("\n--- FINAL WLM ACCOUNT ---"));
    
    // P#1 should show 5 cycles
    Serial.print(F("P#1 Final WLM Account: "));
    Serial.println(EEPRWL_Main.getWrtAccBalance(HANDLE1));
    
    // P#2 should show 5 cycles
    Serial.print(F("P#2 Final WLM Account: "));
    Serial.println(EEPRWL_Main.getWrtAccBalance(HANDLE2));
    
    // If P#2 shows 0 even though getWrtAccBalance() was called, the error is in the migrateData function.

    while (0==0){delay(1000);}
}
// END OF CODE

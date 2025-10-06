*******************************************************************************
* EEProm_Safe_Wear_Level Library v25.10.5 
* Copyright (C) 2025, Torsten Frieser / automatician
*******************************************************************************
* This library is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published
* by the Free Software Foundation, either version 2.1 of the License.
*******************************************************************************

WHAT IT IS

* A complete data management subsystem with integrated error handling,
  multi-partition support, logging functionality, and comprehensive
  auditability.

* Meets the high requirements for robustness and failure safety
  needed in commercial or critical projects:

  * Detection of data corruption / security within the EEPROM itself.

  * Prevention of overwriting program memory in case of faulty
    application.

  * Securing control data structures (RAM Handle Structure)
    against corruption (overwriting / data modification).

  * CRC checksums with an increased detection rate.

-------------------------------------------------------------------------------
1. PURPOSE & KEY FEATURES (ROBUSTNESS)
-------------------------------------------------------------------------------

The EEProm_Safe_Wear_Level library serves as a **complete DATA MANAGEMENT
SUBSYSTEM**. Its strategic goal is not only to maximize the lifespan of the
EEPROM (**WEAR-LEVELING**), but also to GUARANTEE **data integrity** under
critical conditions.

-------------------------------------------------------------------------------
2. KEY FEATURES OF v25.10.5
-------------------------------------------------------------------------------

* **TYPE-SAFE I/O:** Templated Write/Read for **Structs** and primitive
  types in a single call.

* **FAILURE SAFETY:** Each data record is secured with a checksum.
  Corrupted sectors are ignored.

* **WRITE SAFETY: ** Checking and error feedback for write operations.

* **LONG LIFESPAN:** Wear-Leveling via a **Ring Buffer**.

* **CONFIGURABLE COUNTERS: ** Precisely adapt the health functionality
  to your program requirements and control the data overhead in the EEPROM,
  between 2 bytes and 5 bytes.

* **MULTI-PARTITION:** Independent management of multiple logical
  data areas (Handles 0, 1, 2, ...).

* **DIAGNOSTICS:** Detailed error codes (0x01 to 0x05) allow a
  targeted response.

* **LOGGING FUNCTIONALITY: ** Loading physical data records (sectors) and their
  control possibility (log end, log beginning).

* **MEMORY-SAVING DATA MIGRATION: ** Transfer of log entries to a
  second partition, in case of logical saturation of the original partition, so
  that it can be reset (no log loss).

-------------------------------------------------------------------------------
3. INSTALLATION (MANUAL)
-------------------------------------------------------------------------------

Installation is done manually by downloading the release archive.

Manual Installation Method:
1. Download the repository's release ZIP file.
2. Unzip the file (e.g., `EEProm_Safe_Wear_Level`).
3. Rename the folder to `EEProm_Safe_Wear_Level`.
4. Copy the folder to your **Arduino Library Directory**
   (`Documents/Arduino/libraries/`).
5. Restart the Arduino IDE.


+----------------+----------+----------------------------------------------+
| 5. FULL API OVERVIEW (Redundancy-Free Functions)                         |
+----------------+----------+----------------------------------------------+
| FUNCTION       | RETURN   | ROLE                                         |
+----------------+----------+----------------------------------------------+
| config()       | uint16_t | Defines Partition. Returns Override Counter. |
| setVersion()   | bool     | Restores state / formats anew.               |
| write()        | uint16_t | **Templated Write** (Sector ID > 0 if OK).   |
| read()         | bool     | **Templated Read** (TRUE if OK).             |
| getCtrlData()  | uint32_t | Central query for metadata (status, counter).|
| healthCycles() | uint32_t | Estimated remaining write cycles.            |
| healthPercent()| uint8_t  | Remaining lifespan in percent.               |
| getVersion()   | uint16_t | Retrieves the current version value.         |
| loadPhysSector | uint32_t | Diagnostic: Loads physical sector into cache.|
+----------------+----------+----------------------------------------------+
| NOTE: config() is an exception; it does not set the error status byte.   |
+----------------+----------+----------------------------------------------+


+--------------------------------------------------------------------------+
| 6. ERROR DIAGNOSTICS (Status Codes)                                      |
+--------------------------------------------------------------------------+
|                                                                          |
| The current error status (1 byte) of the last read/write operation       |
| (TRUE/FALSE) must be queried.                                            |
|                                                                          |
| **Recommended:** The **direct RAM Handle query** (`PartitionsData[14]`)  |
| is the **most reliable method** and should always be used, as it also    |
| works in case of critical metadata errors (Code 0x05).                   |
|                                                                          |
+----------+---------------------------------------------------------------+
| Hex Code | Meaning                                                       |
+----------+---------------------------------------------------------------+
| 0x00     | All OK. Partition is valid and ready for operation.           |
| 0x01     | CRC checksum of the sector was invalid (data corrupted).      |
| 0x02     | Write attempt rejected: Data type > PayloadSize.              |
| 0x03     | Write attempt rejected: Max logical counter reached.          |
| 0x04     | Initialization error: Magic ID/Version missing.               |
| 0x05     | CRITICAL ERROR: PARTITION control data destroyed.             |
+----------+---------------------------------------------------------------+


+--------------------------------------------------------------------------+
| 4. QUICKSTART / APPLICATION EXAMPLE (Arduino INO Syntax)                 |
+--------------------------------------------------------------------------+

#include "EEProm_Safe_Wear_Level.h"

// Define the partitions (number is freely selectable):
#define PARTITIONS 1

// Regarding the number of partitions, please keep RAM in mind! Each partition
// requires 16 bytes of administrative data.

// The library requires a RAM handle for metadata management, in two steps.

// First step:

typedef struct {
  uint8_t data[16 * PARTITIONS];
} __attribute__((aligned(8))) administrative_control_structure;

// You define a new structure (struct) here without a name (anonymous).
// The typedef assigns the alias name 'administrative_control_structure'
// to this structure type.
//
// The __attribute__((aligned(8))) applies the alignment rule to the type
// 'administrative_control_structure'.
// Alignment describes the rule according to which data must be stored in
// computer memory so that the processor (CPU) can process it efficiently.
// Essentially, data of a certain type (e.g., a 32-bit integer) must
// start at a memory address that is an integer multiple of that data type's size.
//
// Why is alignment important? 
// Speed (Performance): The CPU can read perfectly aligned data
// in a single memory operation. If data is unaligned,
// the CPU must perform two separate read operations and then
// painstakingly reassemble the parts. This drastically slows down processing.
//
// Alignment with Structures (structs)
// Alignment becomes especially relevant when organizing data in structures.
// The compiler must ensure that every element in the struct is correctly
// aligned.
// In the case of EEProm_Safe_Wear_Level, alignment is only needed for the start
// of the data in memory.
// Multiple administrative data sets for multiple partitions are then automatically
// aligned, as they are a multiple of 16 bytes.

// Second step (The Variable):

administrative_control_structure PARTITIONS_DATA;

// You declare a variable of type 'administrative_control_structure'.
// The name of this variable is PARTITIONS_DATA.


// Instantiation: Links the RAM handle administrative data with the code of the
// EEProm_Safe_Wear_Level library.
// Simultaneously, an instance is created, which can then be addressed
// with EEPRWL_Main.
// IMPORTANT: The pointer to the data array in the struct is passed
// (PARTITIONS_DATA.data).
EEProm_Safe_Wear_Level EEPRWL_Main(PARTITIONS_DATA.data); 

// The library now always knows the memory address where the
// administrative data can be found.

// All calls to the EEProm_Safe_Wear_Level API interface
// must take place in Setup() or Loop().


// The length of the payload in bytes:

#define PAYLOAD_SIZE 12
char MY_DATA[PAYLOAD_SIZE];

// 12 bytes for a short string in the example. 

// Separate counter for the loop:
int loopCounter = 0; 


void setup() {

  // Initialize serial interface for output 
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("EEPROM Quickstart started."));

  // Defines the start address in the EEPROM:

  #define ADDR_START      0x0000

  // Theoretically, an address between 0 and the maximum size
  // of the EEPROM is possible. Choose it carefully!

  // You need the length of a partition's data.
  // We chose 1 partition at the beginning. 

  #define PART_LENGTH     144

  // The partition will have this length in the example.
  // The size of the partition (here 144 bytes) determines how many 
  // data records (sectors) fit into it.
  // 10 data records, each with 12 bytes payload and 2 bytes metadata
  // (1 byte logical counter and 1 byte checksum). In addition, each
  // partition has 4 bytes at the beginning for identification and for an
  // additional counter (2 bytes = 16 bit). This counter is set with
  // setVersion(). 

  // The Counter Length in bytes determines how large the Logical Counter
  // for sectors can become:

  #define COUNTER_LEN  1

  // For this example, only one byte is necessary. This allows 256 sectors
  // to be written before the partition needs to be reset. 
  // Ideally, this logical counter should exceed the lifespan of the EEPROM
  // so that a partition reset is not necessary during operation.
  // However, such a large counter is associated with a large overhead (extra data)
  // compared to what is necessary for storing the data itself.
  // You make a trade-off here between this overhead and the
  // practical suitability for your INO code.

  
  // Definition of handles to access each individual partition
  
  #define HANDLE1  0

  // 1. Configuration of Partition #1 (HANDLE1)
  // The config() method does NOT store an error status in the RAM handle, but
  // it returns a success status: TRUE or FALSE (1 or 0)

  int STATUS = EEPRWL_Main.config(ADDR_START, PART_LENGTH, PAYLOAD_SIZE,
                                    COUNTER_LEN, HANDLE1);

  // The config() method calculates various values and, if necessary, sets up
  // a partition in the EEPROM. The calculated values are stored in the
  // administrative data structure. This saves time and memory space for
  // redundant code calls during subsequent function calls. 

  // Corrected if/else block:
  if(STATUS > 0){ // Status > 0 means the version number is set
    Serial.println(F("Configuration successful."));
    Serial.print(F("Current Partition Override Counter: "));
    Serial.println(STATUS);
  } else { // 0 means error or empty partition
    Serial.println(F("Configuration not successful!"));
  }


  // 2. Restore state / reformat
  
  EEPRWL_Main.setVersion(1, HANDLE1);

  // The Handle ID (partition number) to which the operation refers.
  // It must match the ID previously used in the config() function
  // for this partition (e.g., HANDLE1).
  // The version number that is written to the partition must be specified.
  // Changing this number reformats the partition, overwriting all old data,
  // and logically declaring it invalid.
  // setVersion() also searches for the latest valid data record/sector
  // in a partition. This can then be read.
  //
  // Function of the version number during formatting:
  // The version number (which you set via setVersion()) is the
  // master counter of the partition. It controls when a partition is logically
  // considered "empty" or "new".
  //
  // Reactivation and Reset:
  // If a partition enters the state of logical saturation (Error 0x03),
  // it can no longer write data, even though it is physically still intact.
  // To reactivate it, you must call setVersion() and increment the
  // version number (e.g., from 1 to 2).
  // You find the current number as the return value of config(), if the
  // number > 0, or with the function: getVersion(HANDLE1), to which you
  // must pass the handle for the partition number.
  // This process is equivalent to a logical formatting:
  // The library overwrites all old data and the internal
  // wear-leveling counter starts over.
  //
  // Prevention of Redundancy:
  // The internal, automatically incremented Logical Counter serves for
  // wear-leveling and is incremented with every Write. The version number,
  // however, is a separate 16-bit counter that is only incremented
  // when the partition is logically saturated or you manually want to discard
  // all old data (e.g., during a firmware update).

  // 3. Read the last valid value
  // Returns TRUE (1) on success.
  
  if (EEPRWL_Main.read(MY_DATA, HANDLE1, PAYLOAD_SIZE)) {
    Serial.print(F("Valid data read: "));
    Serial.println(MY_DATA);
  }else{
    Serial.print(F("No data available yet: "));
    Serial.println(PARTITIONS_DATA.data[13], HEX);
  }

  // read() serves to read the last valid data record from a specific
  // partition in the EEPROM and copy it into a designated
  // memory structure in RAM.
  // The function can read various data types. It returns a boolean
  // value indicating whether the read operation was successful.
  //
  // Arguments:
  //
  // MY_DATA: This is the target structure or buffer in RAM (the
  // main memory of your microcontroller). The bytes read from the
  // EEPROM are written directly to this memory area. 
  // It must be ensured that this buffer is large enough to accommodate the
  // payload.
  //
  // HANDLE1: Identifies the partition to read from.
  // Role: This is the Handle ID value (e.g., 0, 1, 2...) that selects the
  // specific control data structure (RAM Handle) belonging to this partition.
  // This is crucial for multi-partition support.
  //
  // PAYLOAD_SIZE: Specifies how many bytes maximum should be read from the
  // EEPROM and transferred to the target buffer (Function 2 and 1).
  // Role: This serves as an explicit upper limit and is the primary
  // safety mechanism to prevent RAM buffer overflows in the
  // target array.
  // Ensure that the buffer is large enough to hold the specified
  // PAYLOAD_SIZE!
  // This function: read(MY_DATA, PAYLOAD_SIZE, HANDLE1) cannot
  // process string objects (std::string)!
  // It is optimized for the safe management of **structs** (Plain Old
  // Data) and **C-Arrays** (char*), as these offer the best basis to
  // plan the data volume at design time and to hard-code it.   
}

void loop() {
  // Change data: Creates a string in the buffer (e.g., "MSG_001")
  loopCounter++;
  snprintf(MY_DATA, PAYLOAD_SIZE, "MSG_%03d", loopCounter);

  // --- 4. Write Data (Char* Array) ---
  // The correct signature is: (data buffer, handle ID)
  bool error = EEPRWL_Main.write(MY_DATA, HANDLE1);

  if (error == 0) {
    Serial.print(F("WRITE ERROR. Code: 0x"));
    // Direct query of the status code via RAM Handle
    Serial.println(PARTITIONS_DATA.data[13], HEX); 
  } else {
    Serial.print(F("Data written to logical sector "));
    Serial.print(EEPRWL_Main.getCtrlData(0, HANDLE1)); 
    Serial.print(F(". String: "));
    Serial.print(MY_DATA);
    Serial.print(F(", Next physical sector: "));
    Serial.println(EEPRWL_Main.getCtrlData(4, HANDLE1)); 
  }

  delay(5000);
}

// -------------------------------------------------------------------------------
// 
// This library is released under the terms of the **GNU Lesser General Public
// License v2.1 (LGPL-2.1)**.
//
// -------------------------------------------------------------------------------
// END README

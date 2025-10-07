 # **EEProm_Safe_Wear_Level Library v25.10.5** 
* **💎 EEProm_Safe_Wear_Level: Gold Standard for Industrial Data Integrity**<br>
This library is a **fool-proof solution** for the persistent storage of critical data on microcontrollers. It has been developed to meet the strict requirements for reliability and robustness demanded by the industry.
* **📖 Design Conformity with Industry Standards**
The architecture of this library follows the highest design requirements from the fields of functional safety and software quality:
* **IEC 61508 / ISO 26262 (Functional Safety):** The design ensures the safety of critical data through **automatic error skipping** (Status Code 1) and comprehensive **CRC validation**. The entire wear-leveling logic is aimed at preventing hardware failure, which is a direct measure to increase reliability.
* **ISO/IEC 5055 (Reliability & Maintainability):** The internal encapsulation of complexity, the clear `read(0)/read(1)` API pattern, and the **8 detailed status codes** (e.g., Code 7 for Log End) ensure maximum transparency and maintainability, which are the cornerstones of this quality standard.
**Conclusion:** If you are looking for a library that **guarantees data integrity** and **longevity** across the entire lifecycle of your embedded system, this is the **Gold Standard**.
**Copyright (C) 2025, Torsten Frieser / automatician**
 
 ***This library is free software: you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published
 by the Free Software Foundation, either version 2.1 of the License.***

 ### It covers the entire application spectrum, from simple storage in the EEPROM in the Arduino µC to verifiability data at the commercial level.
📖 The complete manual with public functions, parameters and return values in [API Manual](MANUAL.md).

📖 Starts directly with detailed example codes that will get you to your goal quickly:
 * [Demo1: String in an array](Examples/demo1_type_char_array.ino)
 * [Demo2: Store pure states or measured values ​​(numbers, Booleans)](Examples/demo2_type_int.ino)
 * [Demo3: Store logical grouping of different but related data in structures](Examples/demo3_type_struct.ino)
## **WHAT IT IS**

 * A complete data management subsystem with integrated error handling,
  multi-partition support, logging functionality, and comprehensive
  auditability.

* Meets the high requirements for robustness and failure safety
  needed in commercial or critical projects:

  *  Detection of data corruption / security within the EEPROM itself.

  *  Prevention of overwriting program memory in case of faulty
    application.

  *  Securing control data structures (RAM Handle Structure)
    against corruption (overwriting / data modification).

  *  CRC checksums with an increased detection rate.

-------------------------------------------------------------------------------
## 1. PURPOSE & KEY FEATURES (ROBUSTNESS)
-------------------------------------------------------------------------------

The EEProm_Safe_Wear_Level library serves as a **complete DATA MANAGEMENT
SUBSYSTEM**. Its strategic goal is not only to maximize the lifespan of the
EEPROM (**WEAR-LEVELING**), but also to GUARANTEE **data integrity** under
critical conditions.

-------------------------------------------------------------------------------
## 2. KEY FEATURES OF v25.10.5
-------------------------------------------------------------------------------

* **TYPE-SAFE I/O:** Generic templates for structs and all primitive types. 
C-Strings are additionally supported via a dedicated overload: write(const char*)

* **FAILURE SAFETY:** Each data record is secured with a checksum.
  Corrupted sectors are ignored.

* **WRITE SAFETY:** Checking and error feedback for write operations.

* **LONG LIFESPAN:** Wear-Leveling via a **Ring Buffer**.

* **CONFIGURABLE COUNTERS:** Precisely adapt the health functionality
  to your program requirements and control the data overhead in the EEPROM,
  between 2 bytes and 5 bytes.

* **MULTI-PARTITION:** Independent management of multiple logical
  data areas (Handles 0, 1, 2, ...).

* **DIAGNOSTICS:** Detailed error codes (0x01 to 0x05) allow a
  targeted response.

* **LOGGING FUNCTIONALITY:** Loading physical data records (sectors) and their
  control possibility (log end, log beginning).

* **MEMORY-SAVING DATA MIGRATION:** Transfer of log entries to a second partition
   in case of logical saturation of the original partition so that it can be reset
   (no log loss) in just a few steps.

-------------------------------------------------------------------------------
## 3. INSTALLATION
-------------------------------------------------------------------------------

Installation is done manually by downloading the release archive.

Manual Installation Method:
1. Download the repository's release ZIP file.
2. Unzip the file (e.g., `EEProm_Safe_Wear_Level`).
3. Rename the folder to `EEProm_Safe_Wear_Level`.
4. Copy the folder to your **Arduino Library Directory**
   (`Documents/Arduino/libraries/`).
5. Restart the Arduino IDE.


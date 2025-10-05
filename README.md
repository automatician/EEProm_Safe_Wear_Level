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

* **MEMORY-SAVING DATA MIGRATION:** Transfer of log entries to a
  second partition, in case of logical saturation of the original partition, so
  that it can be reset (no log loss).

-------------------------------------------------------------------------------
3. INSTALLATION
-------------------------------------------------------------------------------

Installation is done manually by downloading the release archive.

Manual Installation Method:
1. Download the repository's release ZIP file.
2. Unzip the file (e.g., `EEProm_Safe_Wear_Level`).
3. Rename the folder to `EEProm_Safe_Wear_Level`.
4. Copy the folder to your **Arduino Library Directory**
   (`Documents/Arduino/libraries/`).
5. Restart the Arduino IDE.

-------------------------------------------------------------------------------
4. COMPLETE MANUAL
-------------------------------------------------------------------------------

The complete instructions can be found in readme.txt in the root directory.
This file contains sample code, explanations, and some tables in ASCII format 
for easy reference.


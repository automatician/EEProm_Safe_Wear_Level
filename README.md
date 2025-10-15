# Industrial Data Integrity with Log and Write Load Management
## EEProm_Safe_Wear_Level 

**Library Version: v25.10.5**

This library provides a reliable and permanent storage solution for critical data on Arduino-compatible μCs. It covers the entire application spectrum, from simple EEPROM storage in the μC to the verifiable data logging required at the commercial level. This library was developed to combine wear leveling with the security of data integrity and an operational guarantee. Write management is a welcome feature and answers the question: "How long will my EEPROM last?" This uncertainty is eliminated by effectively transforming the question of longevity into a quantifiable guarantee. Premature failures are replaced by planning in the software design and active management of the planned parameters.

---

## 🚀 Get Started
The complete manual with public functions, parameters, and return values can be found in the **API Manual** [klick](/MANUAL.md).
* Jumpstart your project with detailed examples:
    * [Demo1](/Examples/demo1_type_char_array.ino): String in an array
    * [Demo2](/Examples/demo2_type_int.ino): Store pure states or measured values (numbers, Booleans)
    * [Demo3](/Examples/demo3_type_struct.ino): Store logical grouping of different but related data in structures
    * [Demo4](/Examples/demo4_multi_partition.ino): Multi-partition reading and writing
    * [Demo5](/Examples/demo5_log_functions.ino): Demonstrates iterative navigation and reading using read(), findNewestData() and findOldestData().
    * [Demo6](/Examples/Demo6_log_migration.ino): Demonstrates the migration of sectors to a second partition starting with a new logical counter.
    * [Demo7](/Examples/demo7_wlm_management.ino): Shows the change in the write load account and the change in the resulting status to show when and why **Write Shedding** occurs

### Manual Installation Method:
1. Download the repository's release ZIP file.
2. Unzip the file (e.g., `EEProm_Safe_Wear_Level-vX.X.X`).
3. Rename the folder to `EEProm_Safe_Wear_Level`.
4. Copy the folder to your **Arduino Library Directory** (`Documents/Arduino/libraries/`).
5. Restart the Arduino IDE.

---

## 🛡️ Design Conformity with Industry Standards

The architecture of this library follows the highest design requirements from the fields of functional safety and software quality:

### IEC 61508 / ISO 26262 (Functional Safety Principles)
The design ensures the safety of critical data through **comprehensive CRC validation** and **error reporting**. The entire wear-leveling logic is aimed at preventing hardware failure, which is a direct measure to increase system reliability and longevity.

### ISO/IEC 5055 (Reliability & Maintainability)
The library achieves maximum transparency and maintainability. The internal encapsulation of complexity and the **clear separation of logical reading and error reporting** ensure maximum transparency, which are the cornerstones of this quality standard.

---

## What It Is: Complete Data Management Subsystem

`EEProm_Safe_Wear_Level` is a **complete data management subsystem** with integrated error handling, multi-partition support, logging functionality, and comprehensive auditability. Its strategic goal is to **GUARANTEE data integrity** under critical conditions while maximizing the lifespan of the EEPROM (WEAR-LEVELING).

It meets the high requirements for **robustness and failure safety** needed in commercial or critical projects.

## Write Load Management
The library implements an innovative Write Load Management system for protecting EEPROM lifespan, extending beyond conventional wear-leveling.

### Write Budgeting
This is the library's strategic function. It describes the proactive approach to mapping the entire physical lifespan (Endurance) of the EEPROM to the planned product lifespan, and time-based allocation of the write budget.

### Write Shedding 
This is the library's short-term, reactive protection mechanism, which is triggered only when the write frequency is overloaded (analogous to Load Shedding in the power grid). It is the response to a worst-case scenario (such as a software bug) that would damage the EEPROM outside of your planning. Since this mechanism actively throttles write cycles by temporarily preventing them, your application software should evaluate its status and respond proactively.

---

## Key Features of v25.10.5

| Feature | Benefit | References |
| :--- | :--- | :--- |
| **WRITE LOAD MANAGEMENT** | Guaranteed longevity through strategic write budgeting (time-based allocation) and **Write Shedding** (protection against overload). |The time base is provided via oneTickPassed() or idle(); the current credit balance can be queried using getWrtAccBalance().|
| **FAILURE SAFETY** | Each data record is secured with a checksum and is verified after saving. Corrupted sectors are reported (see Diagnostics). |The status of the last operation is provided in the getCtrlData() status flag for diagnostic purposes.|
| **CRC with High Detection Rate** | Enhanced CRC checksums for robust detection of data corruption within the EEPROM itself. ||
| **LOGGING FUNCTIONALITY** | Enables secure chronological data logging with API access to the newest/oldest entries and with record navigation. ||
| **LONG LIFESPAN (Wear-Leveling)**| Utilizes a **Ring Buffer** to spread write cycles evenly, significantly increasing EEPROM endurance. ||
| **TYPE-SAFE I/O** | Generic templates for structs, all primitive types, and C-Strings via dedicated overloads. ||
| **MULTI-PARTITION** | Independent management of multiple logical data areas (Handles 0, 1, 2, ...). ||
| **RAM Handle Protection** | Securing control data structures (RAM Handle Structure) against corruption. ||
| **CONFIGURABLE COUNTERS** | Adapt health functionality and control data overhead (between 2 and 5 bytes). ||
| **DATA MIGRATION** | Memory-saving transfer of log entries to a second partition to prevent log loss upon saturation. |Controlled by migrateData(). Queries with getOverwCounter(), healthCycles(), and healthPercent().|
| **DIAGNOSTICS** | Detailed **8 Status Codes** (0x00 to 0x07) allow a targeted response to errors and log states. |The status of Write Budget Management, Shedding active, Credit Status, and other fields of the status byte can be queried with getCtrlData(). Health statistics: Cycles with healthCycles() and percentages with healthPercent().|

---

## 📜 License and Copyright

**Conclusion:** If you are looking for a library that **guarantees data integrity** and **longevity** across the entire lifecycle of your embedded system, this is the **Gold Standard**.

* Copyright (C) 2025, Torsten Frieser / automatician
* This library is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 2.1 of the License.

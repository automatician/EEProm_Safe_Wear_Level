# EEProm_Safe_Wear_Level: for Industrial Data Integrity with Log-Management

**Library Version: v25.10.5**

This library provides a **fool-proof solution** for the persistent, reliable storage of critical data on Arduino-compatible microcontrollers. It covers the entire application spectrum, from simple EEPROM storage in the $\mu\text{C}$ to verifiable data logging required at a commercial level.

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

---

## Key Features of v25.10.5

| Feature | Benefit |
| :--- | :--- |
| **FAILURE SAFETY** | Each data record is secured with a checksum and is verified after saving. Corrupted sectors are reported (see Diagnostics). |
| **CRC with High Detection Rate** | Enhanced CRC checksums for robust detection of data corruption within the EEPROM itself. |
| **LOGGING FUNCTIONALITY** | Enables secure chronological data logging with API access to the newest/oldest entries and with record navigation. | 
| **LONG LIFESPAN (Wear-Leveling)**| Utilizes a **Ring Buffer** to spread write cycles evenly, significantly increasing EEPROM endurance. |
| **TYPE-SAFE I/O** | Generic templates for structs, all primitive types, and C-Strings via dedicated overloads. |
| **MULTI-PARTITION** | Independent management of multiple logical data areas (Handles 0, 1, 2, ...). |
| **RAM Handle Protection** | Securing control data structures (RAM Handle Structure) against corruption. |
| **CONFIGURABLE COUNTERS** | Adapt health functionality and control data overhead (between 2 and 5 bytes). |
| **DATA MIGRATION** | Memory-saving transfer of log entries to a second partition to prevent log loss upon saturation. |
| **DIAGNOSTICS** | Detailed **8 Status Codes** (0x00 to 0x07) allow a targeted response to errors and log states. |

---

## 🚀 Get Started

* The complete manual with public functions, parameters, and return values can be found in the **API Manual**.
* Starts directly with detailed example codes that will get you to your goal quickly:
    * [Demo1](/Examples/demo1_type_char_array.ino): String in an array
    * [Demo2](/Examples/demo2_type_int.ino): Store pure states or measured values (numbers, Booleans)
    * [Demo3](/Examples/demo3_type_struct.ino): Store logical grouping of different but related data in structures
    * [Demo4](/Examples/demo4_multi_partition.ino): Multi-partition reading and writing

### Manual Installation Method:
1. Download the repository's release ZIP file.
2. Unzip the file (e.g., `EEProm_Safe_Wear_Level-vX.X.X`).
3. Rename the folder to `EEProm_Safe_Wear_Level`.
4. Copy the folder to your **Arduino Library Directory** (`Documents/Arduino/libraries/`).
5. Restart the Arduino IDE.

---

## 📜 License and Copyright

**Conclusion:** If you are looking for a library that **guarantees data integrity** and **longevity** across the entire lifecycle of your embedded system, this is the **Gold Standard**.

* Copyright (C) 2025, Torsten Frieser / automatician
* This library is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 2.1 of the License.

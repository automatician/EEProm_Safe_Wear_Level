*******************************************************************************
* EEProm_Safe_Wear_Level Library v25.10.5 - v25.10.6
* Copyright (C) 2025, Torsten Frieser / automatician
*******************************************************************************
* This library is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published
* by the Free Software Foundation, either version 2.1 of the License.
*******************************************************************************

# Industrial Data Integrity with Log and Write Load Management
## EEProm_Safe_Wear_Level

Library Version: v25.10.6 (Patch Release)

This library provides a reliable and permanent storage solution for critical data on
Arduino-compatible microcontrollers. It transcends simple EEPROM storage by combining
advanced Wear-Leveling with rigorous Data Integrity Guarantees and proactive Write Load
Management. This approach eliminates the uncertainty of EEPROM longevity, transforming
it into a quantifiable, actively managed parameter.

---

Used extern standard libraries:

EEPROM.h, Arduino.h, stddef.h, string.h, stdint.h

---

## Key Features & Robustness
The library implements a complete data management subsystem designed for high functional
safety and reliability (inspired by IEC 61508 / ISO 26262 principles).

| Feature                 | Benefit                                                    |
| :---------------------- | :--------------------------------------------------------- |
| WRITE LOAD MANAGEMENT   | Guaranteed longevity through strategic Write Budgeting and |
|                         | Write Shedding (protection against write overload).          |
| FAILURE SAFETY          | Each data record is secured with Enhanced CRC checksums.     |
| LOGGING FUNCTIONALITY   | Enables secure chronological data logging.                   |
| LONG LIFESPAN (Wear-Leveling) | Utilizes a Ring Buffer to spread write cycles evenly.    |
| RAM Handle Protection   | Secures control data structures against corruption.          |
| DIAGNOSTICS             | Detailed 8 Status Codes for targeted diagnostics.            |

---

## Get Started

The complete manual with public functions, parameters, and return values can be found
in the API Manual (https://github.com/automatician/EEProm_Safe_Wear_Level).

### Detailed Examples:
* [Demo1: String/Array](/examples/demo1_type_char_array.ino)
* [Demo2: Primitive Types](/examples/demo2_type_int.ino)
* [Demo3: Structs](/examples/demo3_type_struct.ino)
* [Demo4: Multi-Partition](/examples/demo4_multi_partition.ino)
* [Demo5: Log Navigation](/examples/demo5_log_functions.ino)
* [Demo7: WLM Management](/examples/demo7_wlm_management.ino) - Demonstrates Write Shedding.

### Manual Installation (If not using Library Manager):
1. Download the repository's release ZIP file.
2. Unzip and rename the folder to EEProm_Safe_Wear_Level.
3. Copy the folder to your Arduino Library Directory (Documents/Arduino/libraries/).
4. Restart the Arduino IDE.

---

## Write Load Management Explained

This management system proactively maps the EEPROM's physical lifespan (Endurance) to
your planned product lifespan.

* Write Budgeting: The strategic function that allocates the total physical write
  lifespan (Endurance) over the planned product lifespan.
* Write Shedding: The short-term, reactive protection mechanism that actively throttles
  write cycles when the frequency is overloaded (e.g., due to a software bug), thereby
  preventing premature hardware failure.

---

## License and Copyright

Conclusion: If you are looking for a library that guarantees data integrity and longevity
across the entire lifecycle of your embedded system, this is the Gold Standard.

* Copyright (C) 2025, Torsten Frieser / automatician
* This library is free software: you can redistribute it and/or modify it under the
  terms of the GNU Lesser General Public License as published by the Free Software
  Foundation, either version 2.1 of the License.

// END README

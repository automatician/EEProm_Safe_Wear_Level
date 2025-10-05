# EEProm_Safe_Wear_Level API Manual
This document provides a concise reference for all public functions of the EEProm_Safe_Wear_Level library, which was developed to manage wear-leveling in EEPROM memory.
Note: All functions use a handle parameter (uint8_t), which is reserved for future support of multiple partitions. Currently, it should always be set to 0.

## 1. Initialization and Configuration

### EEProm_Safe_Wear_Level(uint8_t* ramHandlePtr)
Description: The standard constructor for the class. It requires a pointer to a pre-allocated RAM buffer (uint8_t*) that the library uses as its internal I/O cache for data and control information.
| Parameter | Type | Description |

| :--- | :--- | :--- |

|ramHandlePtr |uint8_t* | Pointer to the beginning of the RAM buffer/cache. The required size is determined by PayloadSize and internal metadata.|

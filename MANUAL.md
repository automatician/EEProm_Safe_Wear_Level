# EEProm_Safe_Wear_Level API Manual
This document provides a concise reference for all public functions of the EEProm_Safe_Wear_Level library, which was developed to manage wear-leveling in EEPROM memory.
Note: All functions use a handle parameter (uint8_t), which is reserved for future support of multiple partitions. Currently, it should always be set to 0.

## 1. Initialization and Configuration

### EEProm_Safe_Wear_Level(uint8_t* ramHandlePtr)
Description: The standard constructor for the class. It requires a pointer to a pre-allocated RAM buffer (uint8_t*) that the library uses as its internal I/O cache for data and control information.
| Parameter | Type | Description |
| :--- | :--- | :--- |
|ramHandlePtr |uint8_t* | Pointer to the beginning of the RAM buffer/cache. The required size is determined by PayloadSize and internal metadata.|
### config(uint16_t startAddress, uint16_t totalBytesUsed, uint16_t PayloadSize, uint8_t cntLengthBytes, uint8_t handle)
Description: Initializes and configures the EEPROM wear-leveling partition. This function must be called once before any read/write operations. It automatically finds the latest valid data or reformats the partition if necessary.
| Parameter | Type | Description |
| :--- | :--- | :--- |
|startAddress|uint16_t|The starting address of the partition in the physical EEPROM.|
|totalBytesUsed|uint16_t|The total number of EEPROM bytes allocated to this partition (must be large enough for at least two sectors).|
|PayloadSize|uint16_t|The size (in bytes) of the actual payload data to be stored. This determines the overall sector size.|
|cntLengthBytes|uint8_t|The number of bytes used for the wear-level counter (e.g., 4 for a 32-bit counter, max 4).|
|handle|uint8_t|Partition handle (use 0).|
|Return|uint16_t|Status code: 0 (Error), > 0 Partition Version / Override Counter 1 to 65535.|


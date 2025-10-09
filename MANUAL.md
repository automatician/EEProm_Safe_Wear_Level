# EEProm_Safe_Wear_Level - API Manual
This document provides a concise reference for all public functions of the EEProm_Safe_Wear_Level library, which was developed to manage wear-leveling in EEPROM memory.
The functions **migrateData()** and **getCtrlData()** are optional functions. They are only loaded into Flash memory by the compiler - and thus only occupy space - if they are explicitly called by the user in their code.
* Zero Overhead for Basic Users: Users who only utilize the core functions do not pay a memory price for these advanced features.
* Maximum Efficiency: Users requiring advanced maintenance, backup strategies, or deep diagnostics receive this complex logic without having to implement it themselves in an error-prone manner. This results in a "Zero Application Overhead" in the user's sketch for these functions.

The **Log Management Functions** can be found under point **4. Advanced functions**.
## 1. Initialization and Configuration
### EEProm_Safe_Wear_Level(uint8_t* ramHandlePtr)
Description: The standard constructor for the class. It requires a pointer to a pre-allocated RAM buffer (uint8_t*) that the library uses as its internal I/O cache for data and control information.
| Parameter | Type | Description |
| :--- | :--- | :--- |
|ramHandlePtr |uint8_t* | Pointer to the beginning of the RAM buffer/cache. The required size is determined by PayloadSize and internal metadata.|
### config(uint16_t startAddress, uint16_t totalBytesUsed, uint16_t PayloadSize, uint8_t cntLengthBytes, uint8_t handle)
Description: Initializes and configures the EEPROM wear-leveling partition. This function must be called when the microcontroller is rebooted to create a partition. It formats the partition if the configuration data has changed.
| Parameter | Type | Description |
| :--- | :--- | :--- |
|startAddress|uint16_t|The starting address of the partition in the physical EEPROM.|
|totalBytesUsed|uint16_t|The total number of EEPROM bytes allocated to this partition (must be large enough for at least two sectors).|
|PayloadSize|uint16_t|The size (in bytes) of the actual payload data to be stored. This determines the overall sector size.|
|cntLengthBytes|uint8_t|The number of bytes used for the wear-level counter (e.g., 4 for a 32-bit counter, max 4).|
|handle|uint8_t|Partition handle.|
|Return|uint16_t|Status code: =0 Error, >0 Partition Version / Override Counter 1 to 65535|
## 2. Reading and Writing Data (Templated Functions)
These are the primary functions for interacting with the stored data. They use templates for maximum flexibility.
### write(const T& value, uint8_t handle)
Description: Writes a structured data type (T) to the EEPROM. The wear-level counters are automatically incremented.
| Parameter | Type | Description |
| :--- | :--- | :--- |
|value|const T&|A reference to the data structure or variable to be stored. sizeof(T) must be less than or equal to the configured PayloadSize.|
|handle|uint8_t|Partition handle.|
|Return|bool|*true* on success, *false* on error (e.g., logical counter limit reached, internal error).
### read(uint8_t readMode, T& value, uint8_t handle, size_t maxSize)
Description: Reads data into a variable or data structure (T). If the readMode parameter is set to the default value 0, the function reads the data from the currently valid sector (Current Sector). This mode is used for normal operation to always retrieve the last saved state of the current data from a partition.
| Parameter | Type | Description |
| :--- | :--- | :--- |
|readMode|uint8_t|0: reads data from the current sector|
|value|T&|A reference to the target variable or structure where the data will be loaded.|
|handle|uint8_t|Partition handle.|
|maxSize|size_t|To limit the number of bytes read. Can be used to read partial data. Recommended for correct reading of character strings|
|Return|bool|*true* if valid data was successfully read and loaded, otherwise *false* (e.g., if the IO-Buffer is empty or invalid).|
### Explicit Overloads for C-Strings
For character arrays (char*), specific, non-templated overloads are available to correctly handle null termination:
 * bool write(const char* value, uint8_t handle)
 * bool read(char* value, uint8_t handle, size_t maxSize) //maxSize isnecessary
## 3. Versioning and Health Monitoring
### getVersion(uint8_t handle)
Description: Retrieves the user-defined version number / override counter stored in the control data.
| Parameter | Type | Description |
| :--- | :--- | :--- |
|handle|uint8_t|Partition handle.|
|Return|uint16_t|The current version number of the stored data (the current override counter 0-65535)
### setVersion(uint16_t value, uint8_t handle)
Description: Sets a new user-defined version number. If the version number / counter number passed is different from the one already stored, the partition will be formatted (all data will be lost) and the sector counters will be set to 0.
| Parameter | Type | Description |
| :--- | :--- | :--- |
|value|uint16_t|The new version number to store.|
|handle|uint8_t|Partition handle.|
|Return|bool|*true* = latest sector found / *false* = no latest sector found|
### healthCycles(uint8_t handle)
Description: Calculates the number of remaining write cycles based on the maximum logical counter capacity (_maxLgcCnt) and the current counter value (_curLgcCnt) (is set in *config()*).
This function returns the remaining write cycles. To achieve this, the counter width in bytes must be selected so that the logical sector number covers the entire lifetime of the EEPROM. You can interpret the value differently by limiting the logical sector number to fewer bytes (e.g., 1 byte or 2 bytes) to reduce overhead. Three or four bytes are recommended to track the entire lifetime of the EEPROM. The maximum is four bytes.
| Parameter | Type | Description |
| :--- | :--- | :--- |
|handle|uint8_t|Partition handle.|
|Return|uint32_t|The estimated number of remaining write cycles.|
### healthPercent(uint8_t handle)
Description: Calculates the remaining EEPROM health as a percentage.
| Parameter | Type | Description |
| :--- | :--- | :--- |
|handle|uint8_t|Partition handle.|
|Return|uint8_t|The remaining health percentage (0-100).|
## 4. Advanced Functions - Log Management
### read(uint8_t readMode, T& value, uint8_t handle, size_t maxSize)
Description: Reads data into a variable or data structure (T). The function uses the readMode parameter to control the read destination within the ring buffer. While readMode = 0 always returns the most recent status (the default mode), modes 1, 2, and 3 are used to read the data sequentially or from the beginning of the ring buffer. In this context, the wear leveling structure can be utilized as a cyclic, readable data logger. These extended readMode options allow you to use the wear leveling structure not only to store the latest state, but also to function as a fully navigable log file data buffer.
| Parameter | Type | Description |
| :--- | :--- | :--- |
|readMode|uint8_t| 1: reads data from the next sector <br> 2: reads data from the previous sector <br> 3: reads the oldest sector (log beginning) |
|value|T&|A reference to the target variable or structure where the data will be loaded.|
|handle|uint8_t|Partition handle.|
|maxSize|size_t|To limit the number of bytes read. Can be used to read partial data. Recommended for correct reading of character strings|
|Return|bool|*true* if valid data was successfully read and loaded, otherwise *false* (e.g., if the IO-Buffer is empty or invalid).|migrateData(uint8_t handle, uint8_t targetHandle, uint16_t count)
### migrateData(uint8_t source, uint8_t target, uint16_t count)
The migrateData() function is a special tool for data transfer and maintenance between two separate storage areas (partitions) of your wear-leveling structure. It allows you to copy a specific amount of data from one defined partition (source handle) to another partition (destination handle). The main purpose of this function is to consolidate data and handle version updates in the EEPROM.
#### Backup and Restore
In more complex systems, this function could serve as the basis for a manual backup routine, copying the contents of a critical handle to a separate, less frequently used handle.
#### Logically Exhausted Partitions
The *migrateData()* function addresses the issue where a partition's logical counter has reached its maximum, making the partition logically "full" or "exhausted" from the wear-leveling algorithm's perspective, thus preventing further writes.
#### 1. Releasing the Partition
The function works by reading the last valid data record (the current state) from the logically exhausted partition (sourceHandle) and writing it to a new, freshly configured partition (targetHandle).
#### 2. Restoring Write Cycles
The new partition (target handle) immediately regains its maximum available number of write cycles (e.g., 65,535+) because its logical counter starts at zero. This process effectively allows the user to retire the old, exhausted partition, and potentially reuse or reformat the underlying EEPROM memory space, significantly extending the overall endurance and longevity of the EEPROM storage.
| Parameter | Type | Description |
| :--- | :--- | :--- |
| source | uint8_t | Handle of the source partition. | 
| target | uint8_t | Handle of the target partition. | 
| count | uint16_t | The counter, how many last log entries (newest sectors) are transferred to the target partition. |
**WARNING:** The migrateData() function does not automatically format the source partition (sourceHandle) upon successful migration, as this is a deliberate design choice to prevent the immediate deletion of data, thus supporting backup and data recovery strategies.
### loadPhysSector(uint16_t physSector, uint8_t handle)
Description: Loads the payload and control data of a specific physical EEPROM sector (identified by physSector) into the RAM cache. This function allows direct access to any sector, useful for reading historical data, for example. This function does not deliver the data directly to a user variable, but makes it accessible in the cache for subsequent internal operations (e.g., for checking the metadata or a subsequent read() operation).
 * The *loadPhysSector()* function always requires the next physical sector to be written to derive the sector to be read. It achieves this by decrementing the passed sector index by 1.
 * The *loadPhysSector()* function checks the passed sector for an overflow and handles this overflow correctly, meaning the user does not have to deal with it.
 * If any arbitrary, freely determined physical sector must be read, it must be passed to the *loadPhysSector()* function incremented by +1.
 * The *loadPhysSector()* function calculates the correct value for the next sector to be written from the handed over sector number (taking sector overflow into account). This function can therefore also be used to restore the next sector to be written (Ring Buffer).
   
| Parameter | Type | Description |
| :--- | :--- | :--- |
|physSector|uint16_t|The physical sector to be loaded + 1|
|handle|uint8_t|Partition handle.|
|Return|uint16_t| 0: error / >0: ok (reserved)|
## 5. Controll Data (Advanced)
### getCtrlData(int offs, int handle)
Description: Reads a 32-bit value (4 bytes) from a specific offset within the ControlData structure of the currently loaded partition data.
| Parameter | Type | Description |
| :--- | :--- | :--- |
|offs|int|The byte offset within the control data (e.g., 0 for curLgcCnt).|
|handle|int|Partition handle.|
|Return|uint32_t|The 32-bit value at the specified offset.|
### Offset table
| Offset | Size | Type | Description |
| :--- | :--- | :--- | :--- |
|0|4|uint32_t|ACT. LOGICAL SECTOR (Dynamic)|
|4|2|uint16_t|NEXT PHYSICAL SECTOR (Dynamic) FOR WRITING|
|6|2|uint16_t|THIS PARTITION START ADDRESS IN EEPROM|
|8|2|uint16_t|PAYLOAD SIZE IN BYTES|
|10|2|uint16_t|NUMBER OF SECTORS IN THIS PARTITION|
|12|1|uint8_t|LOGICAL SECTOR COUNTER LENGTH 1 to 4 (e.g., 3 Bytes)|
|13|1|uint8_t|*STATUS FLAG* (0x00=OK, etc. see next table)|
|14|2|uint16_t|CHECKSUM (of this Control Block)|
### Status Flag at Offset 13
| Code | Meaning |
| :--- | :--- |
|0|OK| All OK. Partition is valid and ready for operation.|
|1|CRC checksum of the last read sector was invalid.|
|2|Write attempt: The passed string is > payload size.|
|3|Write attempt rejected: Maximum logical counter reached.|
|4|Initialization error (e.g., Magic ID/Version missing).|
|5|Critical error: Control data corrupted (CRC fails).|
|6|Write attempt: missing *maxSize* for reading string.|
|7|The last read sector is empty / not used.|


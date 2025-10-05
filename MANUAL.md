# EEProm_Safe_Wear_Level API Manual
This document provides a concise reference for all public functions of the EEProm_Safe_Wear_Level library, which was developed to manage wear-leveling in EEPROM memory.
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
|Return|uint16_t|Status code: =0 Error, >0 Partition Version / Override Counter 1 to 65535|
## 2. Reading and Writing Data (Templated Functions)
These are the primary functions for interacting with the stored data. They use templates for maximum flexibility.
### write(const T& value, uint8_t handle)
Description: Writes a structured data type (T) to the EEPROM. The wear-level counters are automatically incremented.
| Parameter | Type | Description |
| :--- | :--- | :--- |
|value|const T&|A reference to the data structure or variable to be stored. sizeof(T) must be less than or equal to the configured PayloadSize.|
|handle|uint8_t|Partition handle (use 0).|
|Return|bool|*true* on success, *false* on error (e.g., logical counter limit reached, internal error).
### read(T& value, uint8_t handle, size_t maxSize)
Description: Reads the latest available data from the IO-Buffer into a variable or data structure (T).
| Parameter | Type | Description |
| :--- | :--- | :--- |
|value|T&|A reference to the target variable or structure where the data will be loaded.|
|handle|uint8_t|Partition handle.|
|maxSize|size_t|Optional parameter to limit the number of bytes read (default is sizeof(T)). Can be used to read partial data.|
|Return|bool|*true* if valid data was successfully read and loaded, otherwise *false* (e.g., if the IO-Buffer is empty or invalid).|
### Explicit Overloads for C-Strings
For character arrays (char*), specific, non-templated overloads are available to correctly handle null termination:
 * bool write(const char* value, uint8_t handle)
 * bool read(char* value, uint8_t handle)
 * bool read(char* value, uint8_t handle, size_t maxSize) //maxSize is recommended
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
|Return|bool|*true* on success.|
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
## 4. Log-Function (Advanced)
### loadPhysSector(uint16_t physSector, uint8_t handle)
Description: Loads the payload and control data of a specific physical EEPROM sector (identified by physSector) into the RAM cache. This function allows direct access to any sector, useful for reading historical data, for example.
| Parameter | Type | Description |
| :--- | :--- | :--- |
|physSector|uint16_t|The index of the physical sector to be loaded.|
|handle|uint8_t|Partition handle.|
|Return|uint16_t|The physical index of the sector that will be used in the next write operation (write()). This value reflects the internal subsequent state of the wear-leveling mechanism after loading.|
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
| Code | Enum Class Name | Meaning |
| :--- | :--- | :--- |
|0|OK| All OK. Partition is valid and ready for operation.|
|1|EEPRWL_STATUS_CRC_ERROR|CRC checksum of the last read sector was invalid.|
|2|EEPRWL_STATUS_STRING_TOO_LONG|Write attempt: The passed string is > payload size.|
|3|EEPRWL_STATUS_MAX_CYCLES_REACHED| Write attempt rejected: Maximum logical counter reached.|
|4|EEPRWL_STATUS_CONFIG_ERROR|Initialization error (e.g., Magic ID/Version missing).|
|5|EEPRWL_STATUS_CTRL_DATA_CORRUPT| Critical error: Control data corrupted (CRC fails).|

# EEProm_Safe_Wear_Level - API Manual
This document provides a concise reference for all public functions of the EEProm_Safe_Wear_Level library, which was developed to manage wear-leveling in EEPROM memory. The functions can be divided into five main categories, with the Write Load Management (WLM) logic being controlled by three functions: **constructor**, **config()**, **oneTickPassed()/idle()** and **getWrtAccBalance()**. <br>

This functions are optional: **migrateData()**, **getCtrlData()**, **getWrtAccBalance()**, **oneTickPassed()**, **idle()**, **findNewestData()**, **findOldestData**.
They are only loaded into Flash memory by the compiler - and thus only occupy space - if they are explicitly called by the user in their code.
* Zero Overhead for Basic Users: Users who only utilize the core functions do not pay a memory price for these advanced features.
* Maximum Efficiency: Users requiring advanced maintenance, backup strategies, or deep diagnostics receive this complex logic without having to implement it themselves in an error-prone manner. This results in a "Zero Application Overhead" in the user's sketch for these functions.

The **Log Management Functions** can be found under point **4. Advanced functions**.

## Table of functions
| 1. Initialization / configuration | 2. Reading & Writing | 3. Health & Statistics / WLM |
| :--- | :--- | :--- |
| [EEProm\_Safe\_Wear\_Level(...)](#eeprom_safe_wear_leveluint8_t-ramhandleptr-uint16_t-seconds) | [write(const T& value, ...)](#writeconst-t-value-uint8_t-handle) | [getOverwCounter()](#getoverwcounteruint8_t-handle) |
| [config(...)](#configuint16_t-startaddress-uint16_t-totalbytesused-uint16_t-payloadsize-uint8_t-cntlengthbytes-uint8_t-budgetcycles-uint8_t-handle) | [read(0, T& value, ...)](#readuint8_t-readmode-t-value-uint8_t-handle-size_t-maxsize) | [initialize(...)](#initializebool-forceformat-uint8_t-handle) |
| [oneTickPassed()](#onetickpassed) | [write(const char\* value, ...)](#explicit-overloads-for-c-strings) | [healthCycles()](#healthcyclesuint8_t-handle) |
| [idle()](#idle) | [read(readMode, char\* value, ...)](#explicit-overloads-for-c-strings) | [healthPercent()](#healthpercentuint32_t-cycles-uint8_t-handle) |
| [getWrtAccBalance()](#getwrtaccbalanceuint8_t-handle) | [read(readMode, T& value, ...)](#readuint8_t-readmode-t-value-uint8_t-handle-size_t-maxsize-1) | [getCtrlData()](#getctrldataint-offs-int-handle) |
| [loadPhysSector()](#loadphyssectoruint16_t-physsector-uint8_t-handle) | [findNewestData() / findOldestData()](#findoldestdatauint8_t-handle--findnewestdatauint8_t-handle) | [migrateData()](#migratedatauint8_t-source-uint8_t-target-uint16_t-count) |

## Security, Integrity and Partial Reformatting
The library implements a three-level security policy to ensure the structural integrity of each partition and prevent unnoticed data corruption. It uses targeted (partial) reformatting without overwriting intact, compatible partitions. Each partition is checked during initialization based on the following criteria. If a check fails, the partition is automatically reformatted.
* Library Compatibility (Magic ID)
  * The stored Magic ID (1 byte) serves as a fingerprint of the internal library structure. If it differs, the sector management logic or the library's data format was changed.
* Overwrite Counter
  * Indicates how many times a partition has been formatted. Once the logical counter has reached its limit, it can be reset by formatting. The write cycles can be calculated as follows: **Total write cycles = (Overwrite counter × Logical counter capacity) + Logical counter**. Logical counter capacity is the maximum value (e.g., 255 or 65535) that the logical counter can reach.
* Configuration Integrity (Control Hash)
  * The 1-byte Control Hash (CRC-8) checks the physical properties of this specific partition.
### Automatic Reformatting and Partial Advantage
If any of the three previously mentioned checks fail, the automatic partial reformat is triggered.
* Corruption Prevention: The CRC-8 control hash ensures that any change to the partition's configuration parameters (made in the ino code) is detected. This prevents old data from conflicting with the incorrect, new structure.
* Partial Advantage: Because each partition stores and verifies its own control hash and magic ID, a configuration change is limited to the affected partition. All other correctly configured partitions in the EEPROM remain unaffected and functional.
  
## 1. Initialization and Configuration
### EEProm_Safe_Wear_Level(uint8_t* ramHandlePtr, uint16_t seconds)
Description: The standard constructor for the class. It requires a pointer to a pre-allocated RAM buffer (uint8_t*) that the library uses as its internal I/O cache for data and control information. A time specification for the tick interval for write budgeting.
| Parameter | Type | Description |
| :--- | :--- | :--- |
| ramHandlePtr |uint8_t* | Pointer to the beginning of the RAM buffer/cache. The required size is determined by PayloadSize and internal metadata.|
| seconds |uint16_t | Seconds after which the **oneTickPassed()** function is called. *oneTickPassed()* is used for write budgeting. |
### config(uint16_t startAddress, uint16_t totalBytesUsed, uint16_t PayloadSize, uint8_t cntLengthBytes, uint8_t budgetCycles, uint8_t handle)
Description: Initializes and configures the EEPROM wear-leveling partition. This function must be called when the microcontroller is rebooted to specify a partition. It formats the partition if the configuration data has changed. Write cycles per hour must be specified here because they are assigned per partition (the maximum value is 255).
| Parameter | Type | Description |
| :--- | :--- | :--- |
|startAddress|uint16_t|The starting address of the partition in the physical EEPROM.|
|totalBytesUsed|uint16_t|The total number of EEPROM bytes allocated to this partition (must be large enough for at least two sectors).|
|PayloadSize|uint16_t|The size (in bytes) of the actual payload data to be stored. This determines the overall sector size.|
|cntLengthBytes|uint8_t|The number of bytes used for the wear-level counter (e.g., 4 for a 32-bit counter, max 4).|
|budgetCycles|uint8_t | Budget write cycles per hour. |
|handle|uint8_t|Partition handle.|
|Return|uint16_t|Status code: =0 Error, >0 Partition Version / Overwrite Counter 1 to 65535|
## 1.5 Write Load Management (WLM)
**Purpose**: It ensures that the EEPROM write cycles are not prematurely and unnoticed used up by a constantly too high average usage rate. <br>

**Write Budgeting**

It is the strategic function of this library. It describes the proactive approach to mapping the entire physical lifetime of the EEPROM to the planned product lifespan.

**Write Shedding**

It is the library's short-term, reactive protection mechanism that is only activated when the write frequency is overloaded. It is the response to a worst-case scenario that would damage the EEPROM beyond your planning. Since this mechanism throttles write cycles by temporarily preventing them. Your application software should evaluate its status and respond proactively.
   
The following functions provide the necessary time base to maintain the Write Load Management for the entire system. One of the two functions must be called in your software.
### oneTickPassed()
Description: This function must be called regularly by the external timer or interrupt handler at intervals (seconds, as configured in the constructor). It is designed for precise timekeeping and uses a logical counter and a remainder accumulator to ensure that not a single second is lost in the timekeeping, even in the event of large overflows (≥3600 s). You use this function as **an alternative to the idle()** function; sharing it is redundant and unnecessary. The compiler only integrates the function code if you use it. <br>
| Parameter | Type | Description |
| :--- | :--- | :--- |
| no | void | no return value |

**Warning:** If this function is called uncontrollably outside of a fixed interval, the safety provided by budgeting is lost.
### idle()
Description: This is an **alternative function to oneTickPassed()**, which should be called within the main loop when used. The frequency of the call is not critical, but should occur more than once per hour. For compatibility reasons, it uses the internal millis() timebase, so it is not hardware-dependent and does not consume valuable interrupts in your code. You use this function as an alternative to the oneTickPassed() function; sharing it is redundant and unnecessary. The compiler only integrates the function code if you use it.
| Parameter | Type | Description |
| :--- | :--- | :--- |
| no | void | no return value |

## 1.5.1 Operating Mode: Fixed Budget (Without Tick Functions)
If you intentionally omit the calls to *oneTickPassed()* or *idle()*: the system operates in a **Fixed Budget Mode**.

The WLM does not save the current, reduced state of the Write Credit Bucket to EEPROM during runtime.
Upon every system reboot, the Write Credit Bucket is fully reset and initialized with an initial credit calculated from the config() parameter.

### Initial Budget Calculation:

The maximum available write credit in this mode is calculated by multiplying the budgetCycles value from config() with the initial internal bucket constant (143):
<div align="center"> <h4><i>Initial Budget = budgetCycles &#215; 143</i></h4> </div>
BudgetCycles acts as a multiplication factor for the credit bucket upon initialization.

To set a desired maximum number of write operations (MaxWrites) after a reboot, you must pass the following calculated value to config():
<div align="center"> <h4><i>budgetCycles = MaxWrites &divide; 143</i></h4> </div>
Since the budgetCycles parameter is a uint8_t (max. 255), you should ensure that the result of the division does not exceed this value.

<b><i>Advantage for Prototyping/Testing:</i></b> This guarantees a fixed, high maximum number of write operations upon every reset. This is ideal for testing where the Arduino is connected to a PC, providing an automatic safeguard against excessive EEPROM wear if the device writes frequently and is left running unattended.

**Limitation:** This mode eliminates the continuous, time-based wear leveling control, making it unsuitable for long-term production use where consistent write rate control is required.

## 1.5.2 Write Load  &mdash;  Distinction
The library's wear-leveling mechanism is controlled by the config() parameters and the regular execution of the tick functions. Correctly calculating these parameters is key to determining and ensuring the overall product operating lifetime.

**1. Classic Wear-Leveling (Physical Distribution)**

Most EEPROM libraries focus on Classic Wear-Leveling. 
* Goal: To distribute the TotalCycles per EEPROM cell (e.g. 100.000 cycles per cell) evenly across all available sectors of a partition.
* Mechanism: By using a ring buffer (Sectors of the partition) and a logical counter, it's ensured that each physical cell is only written to again after N write operations (where N=Sectors 
Partition).
* Result: The lifespan is extended by the Wear-Leveling Multiplier (Sectors per Partition), but the write frequency (how often per second/minute data is written) remains uncontrolled.

**2. Write Load**

The Write Load is the time frequency (or rate) of write operations exerted on the storage medium, regardless of spatial distribution. 
* Definition: Write Load describes the number of write() calls per unit of time (e.g. cycles per hour).
* Problem: An overly high Write Load can lead to the Classic Wear-Leveling functioning correctly, but the product's overall lifespan (in years or month) still ending prematurely. If you allow 100.000 cycles/hour, the memory will be exhausted after fewer hours, regardless of whether you use wear-leveling or not.

**3. The next level: Write Load Management (WLM)**

The EEProm_Safe_Wear_Level library introduces Write Load Management (WLM) to solve this Write Load problem. 
 * Goal: To actively control the system's write frequency per hour/day and map it to the planned product lifespan.
 * Mechanism: It uses a Write Budgeting (credit system) that throttles write operations (Write Shedding) based on a fixed budgetCycles (cycles per hour).
 * Result: WLM ensures that the temporal wear aligns with the planned lifespan (5 years, 10 years, etc.), providing protection against excessive Write Load in addition to physical distribution. Furthermore, WLM can be used to comfortably and effectively throttle excessive user inputs in interactive or menu-driven systems, preventing uncontrolled writes to the EEPROM. It offers a convenient way to utilize the advantages of a non-volatile memory while simultaneously protecting it from rapid exhaustion.

**Development Cycle Safety & Controlled Risk**

Every piece of software has a development cycle. When working with EEPROM, which only has a limited lifespan, errors during this phase can have fatal consequences. In the worst case, the memory can be destroyed within minutes or hours by uncontrolled write loops. To prevent this, you can calculate the intended write load and configure it via this library. **Write Shedding** will protect the EEPROM by rejecting writes exceeding the budget. With queryable parameters (via *getCtrlData()* and *getWrtAccBalance()*), write load can be actively controlled at runtime. The inherent limitation of EEPROM write cycles is thus transformed from a passive liability into an **actively controlled risk**.

## 1.5.3 Calculation for WLM and Lifetime Optimization

The total operating lifetime of the system depends on three critical, interdependent parameters. Correct configuration requires rearranging the original equation for the unknown quantity (usually **budgetCycles** or **SectorsPartition**). In addition to increasing the partition size (SectorsPartition), reducing the metadata overhead (header) is the most effective way to optimize memory usage and extend the lifespan of the EEPROM partition. The logical sector counter occupies most of the sector header. It can be between 1 and 4 bytes in size. The smaller this counter is, the more often a partition must be reset. You lose one write cycle of TotalCyclesEEPROM per reset. Each byte saved in the header is multiplied by the total number of sectors in the partition (SectorsPartition), thus contributing to the creation of new sectors.

The original equation for calculating the operating life in years is:
<div align="center"><h3>$$\text{OperatingLifetime (Years)} = \frac{\text{TotalCyclesEEPROM} \times \text{SectorsPartition}}{\text{budgetCycles} \times \text{HoursPerYear}}$$</h3></div>

### Calculating the required budgetCycles (WLM configuration)
This conversion is required to find the minimum WLM parameter (budgetCycles) that ensures a planned lifetime (OperatingLifetime) for a given partition.
<div align="center"><h3>$$\text{budgetCycles} = \frac{\text{TotalCyclesEEPROM} \times \text{SectorsPartition}}{\text{OperatingLifetime (Years)} \times \text{HoursPerYear}}$$</h3></div>

### Calculating the required SectorsPartition (Wear-Leveling Multiplier)
This conversion is required to find the minimum partition size (SectorsPartition) necessary to achieve the planned lifetime for a given write load (budgetCycles).<div align="center"><h3>$$\text{SectorsPartition} = \frac{\text{budgetCycles} \times \text{OperatingLifetime (Years)} \times \text{HoursPerYear}}{\text{TotalCyclesEEPROM}}$$</h3></div>

### Calculation of the maximum permissible TotalCyclesEEPROM (datasheet check)
This conversion is needed to verify which EEPROM specification (Total Cycles) is required to achieve the planned lifetime with a specified partition size and WLM setting.
<div align="center"><h3>$$\text{TotalCyclesEEPROM} = \frac{\text{budgetCycles} \times \text{OperatingLifetime (Years)} \times \text{HoursPerYear}}{\text{SectorsPartition}}$$</h3></div>

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
 * bool read(uint8_t readMode, char* value, uint8_t handle, size_t maxSize) //maxSize isnecessary
## 3. Health Monitoring
### getOverwCounter(uint8_t handle)
Description: Retrieves the overwrite counter stored in the partition.
| Parameter | Type | Description |
| :--- | :--- | :--- |
| handle | uint8_t | Partition handle. |
| Return | uint16_t | The current overwrite counter 0-65535) |
### initialize(bool forceFormat, uint8_t handle)
Description: The partition will be formatted, if forceFormat = 1 (all data will be lost) and the sector counters will be set to 0. Otherwise, the function checks the format or version of the partitions and only formats them if there are deviations to ensure the data structure.
| Parameter | Type | Description |
| :--- | :--- | :--- |
|forceFormat|bool| *1 / true* forces formatting of the partition specified by handle|
|handle|uint8_t|Partition handle.|
|Return|bool|*true* = latest sector found / *false* = no latest sector found|
### healthCycles(uint8_t handle)
Description: Calculates the number of remaining write cycles based on the maximum logical counter capacity (_maxLgcCnt) and the current counter value (_curLgcCnt) (is set in *config()*).
This function returns the remaining write cycles. To achieve this, the counter width in bytes must be selected so that the logical sector number covers the entire lifetime of the EEPROM. You can interpret the value differently by limiting the logical sector number to fewer bytes (e.g., 1 byte or 2 bytes) to reduce overhead. Three or four bytes are recommended to track the entire lifetime of the EEPROM. The maximum is four bytes.
| Parameter | Type | Description |
| :--- | :--- | :--- |
|handle|uint8_t|Partition handle.|
|Return|uint32_t|The estimated number of remaining write cycles.|
### healthPercent(uint32_t cycles, uint8_t handle)
Description: Calculates the remaining EEPROM health as a percentage. Related to the entire lifetime.
| Parameter | Type | Description |
| :--- | :--- | :--- |
|cycles|uint32_t|write cycles of the EEPROM specification (e.g. 100000 at ATmega).|
|handle|uint8_t|Partition handle.|
|Return|uint8_t|The remaining health percentage (0-100).|
## 4. Advanced Functions
## 4.1 Log Management
### read(uint8_t readMode, T& value, uint8_t handle, size_t maxSize)
Description: Reads data into a variable or data structure (T). The function uses the readMode parameter to control the read destination within the ring buffer. While readMode = 0 always returns the most recent status (the default mode), modes 1, 2, and 3 are used to read the data sequentially or from the beginning of the ring buffer. In this context, the wear leveling structure can be utilized as a cyclic, readable data logger. These extended readMode options allow you to use the wear leveling structure not only to store the latest state, but also to function as a fully navigable log file data buffer.
| Parameter | Type | Description |
| :--- | :--- | :--- |
|readMode|uint8_t| 1: reads data from the next sector <br> 2: reads data from the previous sector <br> 3: reads the oldest sector (log beginning) <br> 4: reads the newest sector (log ending)|
|value|T&|A reference to the target variable or structure where the data will be loaded.|
|handle|uint8_t|Partition handle.|
|maxSize|size_t|To limit the number of bytes read. Can be used to read partial data. Recommended for correct reading of character strings|
|Return|bool|*true* if valid data was successfully read and loaded, otherwise *false* (e.g., if the IO-Buffer is empty or invalid).|migrateData(uint8_t handle, uint8_t targetHandle, uint16_t count)
### findOldestData(uint8_t handle) / findNewestData(uint8_t handle)
These functions search for the logically newest or oldest sector available. Unlike *read()*, the data is not loaded directly into the user code. However, the references to the current logical and physical sector are updated. This is important for reading data, but especially for writing data starting at a specific position. The data can then be read with **read()** and new data can write with **write()**.
| Parameter | Type | Description |
| :--- | :--- | :--- |
|handle|uint8_t|Partition handle.|
|Return|bool|*true* or *false* if no sector is found.|
### migrateData(uint8_t source, uint8_t target, uint16_t count)
The migrateData() function is a special tool for data transfer and maintenance between two separate storage areas (partitions) of your wear-leveling structure. It allows you to copy a specific amount of data from one defined partition (source handle) to another partition (destination handle). The main purpose of this function is to consolidate data and handle version updates in the EEPROM.
#### Backup and Restore
In more complex systems, this function could serve as the basis for a manual backup routine, copying the contents of a critical handle to a separate, less frequently used handle.
#### Logically Exhausted Partitions
The *migrateData()* function addresses the issue where a partition's logical counter has reached its maximum, making the partition logically "full" or "exhausted" from the wear-leveling algorithm's perspective, thus preventing further writes.
#### Releasing the Partition
The function works by reading the last valid data records from the logically exhausted partition (sourceHandle) and writing it to a empty partition (targetHandle). The destination partition must be formatted and identical in structure to the source partition. The destination partition must fit at least the sectors to be copied. Make sure there's enough space! After migration, the source partition can be formatted.
| Parameter | Type | Description |
| :--- | :--- | :--- |
| source | uint8_t | Handle of the source partition. | 
| target | uint8_t | Handle of the target partition. | 
| count | uint16_t | The counter, how many last log entries (newest sectors) are transferred to the target partition. |

**WARNING:** The migrateData() function does not automatically format the source partition (sourceHandle) upon successful migration, as this is a deliberate design choice to prevent the immediate deletion of data, thus supporting backup and data recovery strategies.
## 4.2. Physical Sectors
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
## 4.3. Write Budgeting
Write control management is deeply integrated into the library. Write shedding is controlled statistically using credit and balance. To allow your program to respond, the statistical values ​​are communicated via the sticky status byte in the partition control data (*getCtrlData()* function) after the **write()** command did not have a physical error. To query the exact statistical value of a partition's **write account balance**, use this function:
### getWrtAccBalance(uint8_t handle)
| Parameter | Type | Description |
| :--- | :--- | :--- |
|handle|uint8_t|Partition handle.|
|Return|uint8_t|Write account balance. (statistical value: 0 - 255)|

**1. The Statistical Nature of the Write Account Balance**

* **Based on Averages:** The WLM credit (Account Balance) is calculated based on the average value (budgetCycles) set in *config()* over an assumed period (one hour). It does not represent the currently available storage capacity (like bytes), but rather the statistically permitted frequency of write operations.
* **Represents Rate, Not State:** The value serves to control the write rate over the entire product lifespan. It is an indicator of whether the statistical usage is within the acceptable range, not a direct counter of actual EEPROM cycles.
* **Credited Over Time:** The credit is allocated over time (via the tick functions) and acts as a statistical equalization mechanism.

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
|8|2|uint16_t|NUMBER OF SECTORS IN THIS PARTITION|
|10|1|uint8_t|PAYLOAD SIZE IN BYTES|
|11|1|uint8_t|LOGICAL SECTOR COUNTER LENGTH 1 to 4 (e.g., 3 Bytes)|
|12|2|uint16_t|for internal use|
|14|1|uint8_t|*STICKY STATUS Byte* (0x00=OK, etc. see next table)|
|15|1|uint8_t|CHECKSUM (of this Control Block)|
### Sticky Status Byte at Offset 14
| Code | Meaning |
| :--- | :--- |
|0|OK| All OK. Partition is valid and ready for operation.|
|1|CRC checksum of the last read sector was invalid.|
|2|Write attempt: The passed string is > payload size.|
|3|Write attempt rejected: Maximum logical counter reached.|
|4|Partition formatted due to: library version conflict, partition format conflict, or forced formatting.|
|5|Critical error: Control data corrupted (CRC fails).|
|6|Write attempt: missing *maxSize* for reading string.|
|7|The last read sector is empty / not used.|
|8|After write(). Budget manager: To excessive use, Write Shedding is active.|
|9|After write(). Budget manager: Lost credit rating. Fewer write cycles are necessary.|
|10|After write(). Budget manager: Credit given.|
|11|After write(). Budget manager: Credit still available (normal condition).|

## The Sticky Status Byte (Offset 14): Independence and Control
The Status Byte serves as the primary register for the result and state of the last executed operation (e.g. read(), write()). Due to its placement and architectural design, it offers two key advantages for your application code:

**1. Direct Query (Efficiency)**

The Status Byte is located at Offset 14 within the Partition ControlData.
You can query the current status via the getCtrlData(14, handle) function. However, you can also query the Status Byte directly, which is faster and saves a function call.

**2. Manual Reset**

The Status Byte is NOT included in the calculation of the Control Hash. <br>

**Consequence:** You have full control over this value. Your code can set the Status Byte to 0 at any time or overwrite it with other values. The Status Byte is not a control byte for the library, but a pure **notification status** intended to assist you in controlling your code.

**Advantage for Programming (Atomic Check)**

If the Status Byte is not changed by your code, it will only be overwritten by the library with a new status if one is applicable, and otherwise left as is (behaving as a "Sticky Status").

**Note on use**

You generally **do not** need to change the sticky status code. This isn't necessary for normal functionality, because the API functions return an error status (as documented) that is either *true* or *false* if necessary. Depending on the situation, query the status byte for the code relevant to the last called function.

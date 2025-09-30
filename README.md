/******************************************************************************
 * EEProm_Safe_Wear_Level.h - ARDUINO LIBRARY DESCRIPTION
 * LIBRARY VERSION: v25.09.30
 * * PURPOSE AND FUNCTION
 *
 * The EEProm_Safe_Wear_Level library serves to maximize the **lifespan** of the 
 * internal EEPROM memory (Wear-Leveling) and ensure **data integrity** on 
 * microcontrollers.
 *
 * ****************************************************************************
 * 1. WEAR-LEVELING (WEAR PROTECTION) VIA RING BUFFER
 * ****************************************************************************
 *
 * The entire assigned EEPROM partition is treated as a **ring buffer**. 
 * Write operations are sequentially distributed across all sectors to ensure 
 * even wear of the memory cells.
 *
 * -----------------------------------------------------
 * WRITING (write(Data)): Ring Buffer Mechanism
 * -----------------------------------------------------
 * 1. CRC CALCULATION: The standard **CRC-8** is calculated over the payload.
 * 2. SECTOR SELECTION: The library selects the **next sequential sector** * in the ring buffer.
 * 3. COUNTER UPDATE: The **wear-leveling counter** of the chosen sector is 
 * incremented by one.
 * 4. RETURN VALUES:
 * * **0:** The write operation failed for one of the following reasons:
 * - **Partition Error:** **Total sectors in partition < 2**
 * - **End of Life:** The **maximum number of write cycles** has been 
 * reached.
 * - **General Error:** Consistency check failed after writing.
 * => In case of error, the state must be evaluated using the **public 
 * getters** (`getNextWriteSector`, `getMaxSectorNumber`, etc.) 
 * for diagnosis.
 * * **1:** The write operation was successful.
 * 5. STORAGE: The payload, the incremented counter, and the CRC are written 
 * to this next sector.
 *
 * -----------------------------------------------------
 * READING (read(Buffer)): Return Current Data Record
 * -----------------------------------------------------
 * The read method returns the latest, validated data record. Internally, 
 * upon restart, the sector with the highest counter value is searched and 
 * validated for integrity via the CRC-8 checksum.
 *
 * RETURN VALUE:
 * * **true:** The **current and valid data record** is copied into the 
 * buffer, and the function returns 'true' (Success).
 * * **false:** No valid data record could be found (e.g., CRC error, 
 * no data present, or read error).
 *
 * -----------------------------------------------------
 * LOG FUNCTION (loadSectorData(uint16_t physSector)): 
 * Load Data Record into Memory
 * -----------------------------------------------------
 * This read method loads a physical sector of the partition into memory.
 * The sector is loaded, the checksum is validated for integrity, and the 
 * internal pointers are updated. The next or previous physical sector 
 * (+1 or -1) should always be passed. This function thus provides a list 
 * of stored data in chronological order. This list can be as long as the 
 * partition has sectors.
 *
 * RETURN VALUE:
 * * **true:** The current data record was loaded and the function returns 
 * 'true' (Success).
 * * **false:** No valid data record could be loaded (e.g., CRC error, 
 * no data present, or read error).
 *
 * ****************************************************************************
 * 2. BASICS OF INTEGRATION AND INITIALIZATION
 * ****************************************************************************
 *
 * INSTANTIATION: One instance per independent partition is required.
 * EEProm_Safe_Wear_Level EEPRWL_Time;
 *
 * INITIALIZATION SEQUENCE ON RESTART (setup()):
 * ----------------------------------------------------------------------
 * 1. **config()**: Defines the physical partition.
 * 2. **setVersion()**: Restores the state or formats anew.
 * 3. **read()**: Copies the latest data record into the RAM cache.
 * 4. **write()**: Can now be used.
 * ----------------------------------------------------------------------
 *
 * CONFIGURATION (config()): Defines the working area:
 * byte config(int startAddress, int totalBytesUsed, size_t PayloadSize)
 *
 * RETURN VALUES:
 * * **0:** Configuration error (Partition too small).
 * * **> 0:** Success; returns the stored **Override Counter**.
 *
 * EEPRWL_Time.config(ADDR_Time, SIZE_Time, sizeof(Zeitstempel));
 *
 * ****************************************************************************
 * 3. STATE AND DIAGNOSTIC FUNCTIONS
 * ****************************************************************************
 *
 * uint8_t .getVersion()
 * --------------------
 * Returns the **version value (Override Counter) currently stored in the 
 * EEPROM**. This value is read from the EEPROM upon restart (via `config()` 
 * or `setVersion()`).
 *
 * int .getNumSectors()
 * -------------------------
 * Returns the **total number of sectors/data records** available in the 
 * configured partition (ring buffer).
 *
 * uint32_t .getNextWriteSector()
 * ------------------------------------
 * Returns the **index of the next sector** that will be written upon the 
 * next call to `write()` (0 to `getNumSectors()` - 1).
 *
 * uint32_t .getMaxSectorNumber()
 * ------------------------------------
 * Returns the maximum possible logical sector number. This is the number 
 * reached when the EEPROM memory cells have been written 100,000 times.
 *
 * uint32_t .getSectorNumber()
 * ------------------------------------
 * Returns the current logical sector number. Correlates with the data that 
 * can be read via .read().
 *
 * ****************************************************************************
 * 4. VERSIONING AND LIFESPAN OVERRIDE
 * ****************************************************************************
 *
 * bool .setVersion(uint8_t value)
 * -------------------------------
 * This method is used for version control and lifespan override.
 *
 * RETURN VALUES:
 * * **false:** The **number of sectors in the partition is less than 2** * (Wear-Leveling not possible).
 * * **true:** Success. The partition was either restored or newly formatted.
 *
 * * **Lifespan Override Mechanism:** If a new value for the **Override 
 * Counter** (`value`) is passed that **does not** match the stored value, 
 * the entire partition is **newly formatted** and the internal **sector 
 * counter is reset to 0**. This enables the **restart of the lifespan 
 * counting**.
 * * EEPROWL_Time.setVersion(PARTITION_VERSION);
 *
 *****************************************************************************/

## License

This library is released under the terms of the **GNU Lesser General Public License v2.1 (LGPL-2.1)**.

For detailed licensing terms and conditions, please refer to the **LICENSE** file included in this repository.

* **Author/Maintainer:** Torsten Frieser / automatician
/******************************************************************************************************
 * EEProm_Safe_Wear_Level Library v25.10.5
 * Copyright (C) 2025, Torsten Frieser / automatician
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library. If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************************************
 * EEProm_Safe_Wear_Level.cpp
 *
 * --- GLOSSARY OF ABBREVIATIONS (Internal Variables & Constants) ---
 *
 * MEMBER VARIABLES (State):
 * _startAddr  : startAddress
 * _totalLen   : totalLength
 * _numSecs    : numSectors (Number of physical sectors)
 * _secSize    : sectorSize (Size of one sector including overhead)
 * _pldSize    : payloadSize (Size of the pure user data)
 * _nextPhSec  : nextWriteSector (next physical sector)
 * _maxLgcCnt  : maxSectorNumber (maximum logical counter value)
 * _curLgcCnt  : currentCounter (current logical counter value)
 * _ioBuf      : ioBuffer (Data cache in RAM)
 * _EEPRWL_VER : _EEPRWL_VERSION
 *
 * STATIC CONSTANTS:
 * DEFAULT_PLD_SIZE : DEFAULT_PAYLOAD_SIZE
 * _cntLen          : counterLength (Number of counter bytes)
 * _ctlLen          : controlDataLength (Number of control data bytes)
 *
 * EEPROM MACROS:
 * e_r	: read
 * e_w	: write
 * e_c	: commit
 ******************************************************************************************************
 */
// =========================================================================
// RAM HANDLE STRUCTURE (16 BYTES PER PARTITION)
// =========================================================================
// The administrative data for each partition (I/O access).
// The array length = number of partitions * 16 bytes.
//
// +-----+------+------------+-----------+--------------------------------+
// | Byte| Size | Variable   | Type      | Description                    |
// +-----+------+------------+-----------+--------------------------------+
// | 0   | 4    | curLgcCnt  | uint32_t  | LOG. MAIN COUNTER (Dynamic)    |
// | 4   | 2    | nextPhSec  | uint16_t  | WRITE INDEX (Dynamic)          |
// | 6   | 2    | startAddr  | uint16_t  | EEPROM START OFFSET            |
// | 8   | 2    | pldSize    | uint16_t  | PAYLOAD SIZE                   |
// | 10  | 2    | numSecs    | uint16_t  | NUMBER OF SECTORS IN PARTITION |
// | 12  | 1    | cntLen     | uint8_t   | COUNTER LENGTH (e.g., 3 Bytes) |
// | 13  | 1    | status     | uint8_t   | STATUS FLAG (0x00=OK, etc.)    |
// | 14  | 2    | checksum   | uint16_t  | CHECKSUM (of the Control Block)|
// +-----+------+------------+-----------+--------------------------------+
// |TOTAL| 16   |            |           |                                |
// +-----+------+------------+-----------+--------------------------------+
//+----------+---------------------------------+------------------------------------------------------------------+
//| Hex Code | Enum Class Name                   | Meaning                                                        |
//+----------+---------------------------------+------------------------------------------------------------------+
//| 0x00     | NOT DEFINED                     | All OK. Partition is valid and ready for operation.              |
//| 0x01     | EEPRWL_STATUS_CRC_ERROR         | CRC checksum of the last read sector was invalid.                |
//| 0x02     | EEPRWL_STATUS_STRING_TOO_LONG   | Write attempt: The passed string is >= _pldSize.                 |
//| 0x03     | EEPRWL_STATUS_MAX_CYCLES_REACHED| Write attempt rejected: Maximum logical counter reached.         |
//| 0x04     | EEPRWL_STATUS_CONFIG_ERROR      | Initialization error (e.g., Magic ID/Version missing).           |
//| 0x05     | EEPRWL_STATUS_CTRL_DATA_CORRUPT | Critical error: Control data corrupted.                          |
//+----------+---------------------------------+------------------------------------------------------------------+

#include "EEProm_Safe_Wear_Level.h"
#include <EEProm_Safe_Wear_Level_Macros.h>

// ----------------------------------------------------------------------------------------------------
// --- DEFINITION OF STATIC CONSTANTS ---
// ATTENTION: The constant definitions here must match the abbreviated names
//            in the header file (.h)!
// ----------------------------------------------------------------------------------------------------
const uint16_t EEProm_Safe_Wear_Level::MAGIC_ID = 0x574c;
const size_t EEProm_Safe_Wear_Level::METADATA_SIZE = 4;
const size_t EEProm_Safe_Wear_Level::DEFAULT_PLD_SIZE = 1;
// ----------------------------------------------------------------------------------------------------
// --- CONSTRUCTOR ---
// ----------------------------------------------------------------------------------------------------
EEProm_Safe_Wear_Level::EEProm_Safe_Wear_Level(uint8_t* ramHandlePtr)
    : _ramStart(ramHandlePtr), // Stores the passed pointer
      _ioBufSize(0),
      _ioBuf(nullptr),
      _ctlLen(0),
      _secSize(0),
      _maxLgcCnt(0),
      _handle(0xFF),
      _handle1(0xFF),
      _usedSector(0)
{}
// ----------------------------------------------------------------------------------------------------
// --- PUBLIC API ---
// ----------------------------------------------------------------------------------------------------

// Returns: Override number of the partition
uint16_t EEProm_Safe_Wear_Level::config(uint16_t startAddress, uint16_t totalBytesUsed, uint16_t PayloadSize, uint8_t cntLengthBytes, uint8_t handle) {

      // Calculates the pointer to the start of the partition in the RAM Handle
      uint8_t* ramPtr = _ramStart + ((size_t)handle * CONTROL_STRUCT_SIZE);
      // Assignment of the calculated pointer to _controlCache (with casting)
      _controlCache = (ControlData*) ramPtr;
      _handle = handle;

    uint16_t success = 1; _startAddr = startAddress;
    _cntLen = min(cntLengthBytes, 4);
    _ctlLen = _cntLen + 1;

    // 1. Check and set payload size
    if (PayloadSize < DEFAULT_PLD_SIZE) {
        _pldSize = DEFAULT_PLD_SIZE;
    } else {
        _pldSize = PayloadSize;
    }

    // Calculation of the maximum possible physical sectors
    _numSecs = (totalBytesUsed - METADATA_SIZE) / (_pldSize + _ctlLen);

    // If at least 1 sector is not calculated, it must terminate with an
    // error!
    if (_numSecs < 2) {
        success = 0;
        _numSecs = 1;
    }

    // Ensures that the rollout is triggered exactly after a full number of
    // rotations. With adjustment for the sector counter size.
    uint32_t maxCapacity = (1UL << (_cntLen * 8)) - 1;
    _maxLgcCnt = (maxCapacity / _numSecs) * _numSecs;

    // Calculate the exact sector size
    _secSize = _pldSize + _ctlLen;

    // Write address initially unknown
    _nextPhSec = 0;

    if(_ioBufSize<_secSize){
        if(_ioBuf != nullptr) delete[] _ioBuf;
	_ioBuf = new uint8_t [_secSize];
        _ioBufSize = _secSize;
    }

    if (success > 0) {
        success = ((uint16_t)e_r(_startAddr + 3) << 8 ) | e_r(_startAddr + 2);
    }

    _checksum = chkSum();
    return success;
}

// ----------------------------------------------------------------------------------------------------

bool EEProm_Safe_Wear_Level::setVersion(uint16_t value, uint8_t handle) {
    _START_
    _EEPRWL_VER = value; bool success = 1;

    // 2. Check: config() must have been successful (at least 2 sectors)
    if (_numSecs < 2) {
        success = 0;
    }
    if (success == 1){
	    // --- 3. CHECKING METADATA (Magic ID and Version) ---

	    // Read Magic ID (High uint8_t -> Low Byte)
	    uint16_t magicID_read = (uint16_t)e_r(_startAddr) << 8;
	    magicID_read |= (uint16_t)e_r(_startAddr + 1);

	    // Read Version
	    uint16_t version_read = ( (uint16_t)e_r(_startAddr + 3) << 8 ) | e_r(_startAddr + 2);

	    // Check: Magic ID or Version incorrect?
	    if (magicID_read != MAGIC_ID || version_read != _EEPRWL_VER) {
	        _status = 4;
	        // Necessary: First use or version conflict -> Format!
	        formatInternal(handle);

	        e_w(_startAddr, (uint8_t)(MAGIC_ID >> 8));
	        e_w(_startAddr + 1, (uint8_t)MAGIC_ID);
	        e_w(_startAddr + 2, (uint8_t)(_EEPRWL_VER & 0xFF));
	        e_w(_startAddr + 3, (uint8_t)(_EEPRWL_VER >> 8));
	        e_c;

	        _nextPhSec = 0;
	        return true;
	    }

	    // --- 4. RESTORATION ---
	    // If the metadata is valid, find the latest sector.
	    findMarginalSector(handle,0);

	    // After findMarginalSector(), the state is set either to the latest sector
	    // or (if no sector was valid) to counter 0.
    }
    _END_
    return success;
}

// ----------------------------------------------------------------------------------------------------

uint16_t EEProm_Safe_Wear_Level::getVersion(uint8_t handle) {
    _START_
    // We read the 16-bit version from addresses _startAddr + 2 and + 3
    uint16_t version_read = ((uint16_t)e_r(_startAddr + 3) << 8) | e_r(_startAddr + 2);
    _END_
    return version_read; // Returns the correct 16-bit value.
}
// ----------------------------------------------------------------------------------------------------
// --- PUBLIC API (READ / WRITE / NAV) ---
// ----------------------------------------------------------------------------------------------------

bool EEProm_Safe_Wear_Level::read(uint8_t ReadMode, char* value, uint8_t handle, size_t maxSize) {
    _START_

    _read(ReadMode, handle);

    // Check the cache status
    // Only if the status uint8_t is 1, the data is valid.
    bool success = _ioBuf[_secSize - 1];
    if(success == 1){
	    // Copy data from _ioBuf to the destination variable
	    uint16_t i;
	    
	    if (maxSize > _pldSize) maxSize = _pldSize;
	    
            if (maxSize > 0){
                _status = 0;
	        for (i = 0; i < maxSize; i++) {
		        value[i] = _ioBuf[i];
	    	}
	    }else _status = 6;
    }
    _END_
    return success;
}
// ----------------------------------------------------------------------------------------------------
void EEProm_Safe_Wear_Level::_read(uint8_t ReadMode, uint8_t handle) {

    switch (ReadMode) {
        case 1:
            _nextPhSec++;
            _handle1 = -1;
            break;
            
        case 2:
            _nextPhSec--;
            _handle1 = -1;
            break;

        case 3:
            findMarginalSector(handle, 1);
            cli();
            _handle1 = handle;
            return;

        default:
            break;
    }
    
    if(_handle1 != handle){
	loadPhysSector(_nextPhSec, handle);
        cli();
        _handle1 = handle;
    }

    return;
}
// ----------------------------------------------------------------------------------------------------
bool EEProm_Safe_Wear_Level::migrateData(uint8_t handle, uint8_t targetHandle, uint16_t count){
    _START_
   
    bool success = 0; uint16_t count1 = 0;

    // find newest sector at source partition 
    // migration starts when a sector is found, with searching backward

    if(findMarginalSector(handle,0)){
	size_t actPhysSec = _nextPhSec;
        _nextPhSec--;
	while (count > 0 && _nextPhSec != actPhysSec) {
        	success = loadPhysSector(_nextPhSec, handle); cli();
        	if (success == true) { count--; count1++; } 
	        _nextPhSec--;
	}
    }

    // now migrate a sector to target partition
    while (count1 > 0){
	    _start(targetHandle); uint16_t i=1;
	    while (write(_ioBuf,targetHandle) == 0 && i < _numSecs){ i++; }
            if(i==_numSecs) count1 = 1;
	    if (count1 > 1){
            	_start(handle); success = 0;
		    while (success == 0){
		        _nextPhSec++;
		    	success = loadPhysSector(_nextPhSec,handle);
		        cli();
		    }
	    }
	    count1--;
    }
    
    _END_
    return success;
}
// ----------------------------------------------------------------------------------------------------

uint16_t EEProm_Safe_Wear_Level::loadPhysSector(uint16_t physSector, uint8_t handle) {
    _START_
    uint16_t success;  uint16_t x; _usedSector = 0;

    if (physSector == -1) {
        physSector = _numSecs-1;
    }else if (physSector > _numSecs) {
        	physSector = 0;
    	  }
 
    _nextPhSec = physSector;

    if (physSector == 0) physSector = _numSecs-1;
    else physSector--;

    physSector *= _secSize;
    physSector += _startAddr + METADATA_SIZE;

    for (x = 0; x < (_secSize - 1); x++) {
        _ioBuf[x] = e_r(physSector + x);
    }

    // Read sector checksum from EEPROM
    uint8_t crc = e_r(physSector + x);

    // Calculate CRC based on the data read into _ioBuf
    uint8_t crc1 = calculateCRC(_ioBuf, _secSize - 1);

    if (crc == crc1) {
        _curLgcCnt = 0;
	_curLgcCnt = readLE(&_ioBuf[_pldSize], _cntLen);
        success = _nextPhSec;
    } else {
        _status = 1;
        if (_usedSector == 0)_status = 7;
        success = 0;
    }

	_END_
    return success;
}

// ----------------------------------------------------------------------------------------------------

bool EEProm_Safe_Wear_Level::write(const char* value, uint8_t handle) {
    _START_
    bool success;
    // Consistency check
    if (_numSecs < 2 || _curLgcCnt > _maxLgcCnt) {
        success = 0;
        if(_curLgcCnt > _maxLgcCnt) _status = 3;
    }else success = 1;

    uint16_t slen = strlen(value);

    if (success == 1){
    	if (slen > _pldSize) _status = 2;
    	uint16_t i;

    	for (i = 0; i < _pldSize; i++) {
    	    if (i < slen) {
    	        _ioBuf[i] = value[i];
    	    } else {
    	        _ioBuf[i] = 0;
    	    }
    	}

    	success = _write(handle);
    }
	_END_
    return success;
}

// ----------------------------------------------------------------------------------------------------

bool EEProm_Safe_Wear_Level::_write(uint8_t handle) {
    uint16_t c; bool success = 1;
    if(_curLgcCnt == _maxLgcCnt){
    	_status = 3;
	    _ioBuf[_secSize - 1] = 0;
        success = 0;
    }
    if(success == 1){
    	_curLgcCnt += 1; _handle1 = handle;

    	writeLE(&_ioBuf[_pldSize], _curLgcCnt, _cntLen);

    	_ioBuf[_secSize - 1] = calculateCRC(_ioBuf, _secSize - 1);

    	// Write data
    	uint32_t Adress = _startAddr + METADATA_SIZE + (_nextPhSec * _secSize);
    	for (uint16_t x = 0; x < _secSize; x++) {
    	    e_w(Adress + x, _ioBuf[x]);
    	}
    	e_c;

    	uint16_t sek = _nextPhSec; _nextPhSec += 1;
    	if (_nextPhSec >= _numSecs) {
    	    _nextPhSec = 0;
    	}

    	// Compare data
    	Adress = _startAddr + METADATA_SIZE + (sek * _secSize);
    	for (c = 0; c < _secSize; c++) {
    	    uint8_t buf = e_r(Adress + c);
    	    if (buf != _ioBuf[c]) {
    	        success = false;
    	        break;
    	    }
    	}
    }
    _ioBuf[_secSize - 1] = success;

    return success;
}

// ----------------------------------------------------------------------------------------------------
// --- GETTERS FOR STATE AND METADATA ---
// Remaining cycles
uint32_t EEProm_Safe_Wear_Level::healthCycles(uint8_t handle){
    _START_
    uint32_t success = 1;
    _END_
    if (_curLgcCnt > _maxLgcCnt){_status = 3; success = 0;}
    if(success==1)success = _maxLgcCnt - _curLgcCnt;
    return success;
}

// Remaining cycles in percent
uint8_t EEProm_Safe_Wear_Level::healthPercent(uint8_t handle){
    _START_
    _END_
    uint32_t success = 1;
    uint32_t maxCycles = _maxLgcCnt;
    uint32_t current = _curLgcCnt;
    if (maxCycles == 0) success = 0;
    if(success == 1){
    	uint32_t usedPercent = (uint32_t)(((uint64_t)current * 100) / maxCycles);
    	success = 100 > usedPercent ? 100 - usedPercent : 0;
    }

    return (uint8_t)success;
}
// ----------------------------------------------------------------------------------------------------
bool EEProm_Safe_Wear_Level::findNewestSector(uint8_t handle) {
    bool success = 1;
    _START_
    success = findMarginalSector(handle,0);
    _END_
    return success;
}
// ----------------------------------------------------------------------------------------------------
// --- PRIVATE HELPER ---
// ----------------------------------------------------------------------------------------------------

bool EEProm_Safe_Wear_Level::findMarginalSector(uint8_t handle, uint8_t margin) {
    _curLgcCnt = 0; _nextPhSec = 0; bool success = false;

    // Search all sectors
    // i MUST be uint16_t to support > 255 sectors (e.g., with 2KB EEPROM)
    for (uint16_t i = 0; i < _numSecs; i++) {
        uint32_t cC = 0;
        uint8_t crc, crc1, x, c;
        uint32_t Address = _startAddr + METADATA_SIZE + (i * _secSize);

        // --- 1. Read sector into _ioBuf and check CRC ---

        // Read data bytes and sector counter. The loop condition MUST be
        // (_secSize - 1) because the last uint8_t (CRC) is read separately.
        for (x = 0; x < (_secSize - 1); x++) {
            _ioBuf[x] = e_r(Address + x);
        }

        // Read sector checksum from EEPROM
        crc = e_r(Address + x);

        // Calculate CRC based on the data read into _ioBuf
        crc1 = calculateCRC(_ioBuf, _secSize - 1);

        if (crc == crc1) {
            // The counter bytes are read starting from position _pldSize from the ioBuf
            // (Little Endian).
            cC = readLE(&_ioBuf[_pldSize], _cntLen);
            // --- 2. Find the highest counter value ---
            
            if ((margin == 0 && _curLgcCnt <= cC) || (margin != 0 && _curLgcCnt >= cC)) {
            // These assignments are identical in both successful cases
                _curLgcCnt = cC;        // log. sector
                _nextPhSec = i + 1;     // ph. sector
                success = true;
            }
        }
    }

    // --- 3. Copy the found sector to the cache and set status ---
    if (success == true) {
        // c must be uint16_t to store the index of the found sector
        uint16_t c = _nextPhSec - 1;

        // Read the sector (data and counter) with the highest counter value AGAIN
        // into the ioBuf.
        for (uint16_t x = 0; x < (_secSize - 1); x++) {
            _ioBuf[x] = e_r(_startAddr + METADATA_SIZE +
                                        (c * _secSize) + x);
        }

        // Set the last uint8_t of the ioBuf as a RAM-internal status flag (1 = valid).
        _ioBuf[_secSize - 1] = 1;
    } else {
        // Set the status flag (0 = invalid) if no valid sector was found.
        // _ioBuf[_secSize - 1] = 0; <-- This line is not in the original, but the logic should be:
         _ioBuf[_secSize - 1] = 0;
    }

    // Handle overflow: If the counter has reached the end of the partition,
    // start again at 0
    if (_nextPhSec > _numSecs) {
        _nextPhSec = 0;
    }
    return success;
}

// ----------------------------------------------------------------------------------------------------

uint8_t EEProm_Safe_Wear_Level::calculateCRC(const uint8_t *data, size_t length) {
    uint8_t crc = 0x00; // Initialwert 0 (Oft auch 0xFF, hier 0x00 für Einfachheit/Kompaktheit)
    for (size_t i = 0; i < length; i++) {
        if (data[i] > 0) _usedSector = 1;
        crc ^= data[i]; // XOR mit dem nächsten Daten-Byte
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) { // Prüfen, ob das MSB gesetzt ist
                // Shift und XOR mit dem Polynom
                crc = (crc << 1) ^ 7; // Standard CRC-8 Polynom x^8 + x^2 + x^1 + 1 (0x07)
            } else {
                // Nur Shift
                crc <<= 1;
            }
        }
    }
    return crc;
}

// ----------------------------------------------------------------------------------------------------

void EEProm_Safe_Wear_Level::formatInternal(uint8_t handle) {
    // Iterate through all sectors
    for (uint16_t i = 0; i < _numSecs; i++) {
        uint16_t x;

        // **Optimization 1: Address calculation**
        // Calculate the base address of the current sector once (Speed/Readability)
        uint16_t baseAddr = _startAddr + METADATA_SIZE + (i * _secSize);

        // 1. Data bytes & counter (initialize with 0x00)
        for (x = 0; x < _pldSize + _cntLen; x++) {
            // **Optimization 2: EEPROM Wear-Leveling**
            // Only write if the value in EEPROM != 0x00.
            if (e_r(baseAddr + x) != 0x00) {
                e_w(baseAddr + x, (uint8_t)0x00);
            }
        }

        // 2. Checksum (initialize with 0xF0)
        // x is now at the correct index for the CRC byte
        // **Optimization 2: EEPROM Wear-Leveling**
        if (e_r(baseAddr + x) != 0xF0) {
            e_w(baseAddr + x, (uint8_t)0xF0);
        }

        e_c;
    }
    return;
}

// ----------------------------------------------------------------------------------------------------
/**
 * Loads the control data of the specified partition from the RAM handle into the cache.
 * Disables interrupts (cli). Replaces _START_.
 */
bool EEProm_Safe_Wear_Level::_start(uint8_t handle) {
    cli(); bool success = 1;
    if (_handle != handle){
	// Calculates the pointer to the start of the partition in the RAM Handle
	size_t offset = (size_t)handle * CONTROL_STRUCT_SIZE;
	_controlCache = (ControlData*)(_ramStart + offset);
    	_handle = handle;
    	_ctlLen = _cntLen + 1;
    	_secSize = _pldSize + _ctlLen;
    	uint32_t maxCapacity = (1UL << (_cntLen * 8)) - 1;
    	_maxLgcCnt = (maxCapacity / _numSecs) * _numSecs;
    }
    if (_checksum != chkSum()) {_status = 0x05; sei(); success = 0;}
    return success;
}
void EEProm_Safe_Wear_Level::_end(){
    _checksum = chkSum(); sei();
    return;
}

// ----------------------------------------------------------------------------------------------------
// END OF CODE

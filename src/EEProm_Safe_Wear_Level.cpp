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
// | 8   | 2    | numSecs    | uint16_t  | NUMBER OF SECTORS IN PARTITION |
// | 10  | 1    | pldSize    | uint16_t  | PAYLOAD SIZE                   |
// | 11  | 1    | cntLen     | uint8_t   | COUNTER LENGTH (e.g., 3 Bytes) |
// | 12  | 2    | buckCyc    | uint16_t  | bucket cycles                  |
// | 14  | 1    | status     | uint8_t   | STATUS FLAG (0x00=OK, etc.)    |
// | 15  | 1    | checksum   | uint16_t  | CHECKSUM (of the Control Block)|
// +-----+------+------------+-----------+--------------------------------+
// |TOTAL| 16   |            |           |                                |
// +-----+------+------------+-----------+--------------------------------+

#include "EEProm_Safe_Wear_Level.h"
#include <EEProm_Safe_Wear_Level_Macros.h>

// ----------------------------------------------------------------------------------------------------
// --- DEFINITION OF MACRO CONSTANTS ---
// ATTENTION: The constant definitions here must match the abbreviated names
//            in the header file (.h)!
// ----------------------------------------------------------------------------------------------------
#define DEFAULT_PLD_SIZE  1
// Meta Data (size: magic-id(1) + config-hash(1) + Overwrite-counter(2)):
#define METADATA_SIZE  4
#define MAGIC_ID  0x49
// ----------------------------------------------------------------------------------------------------
// --- CONSTRUCTOR ---
// ----------------------------------------------------------------------------------------------------

EEProm_Safe_Wear_Level::EEProm_Safe_Wear_Level(uint8_t* ramHandlePtr, uint16_t seconds)
    : _ramStart(ramHandlePtr), // Stores the passed pointer
      _ioBufSize(8),
      _ioBuf(new uint8_t [8]),
      _buckPerm(new uint8_t [8]),
      _budgetCycles(new uint8_t [8]),
      _buckTime(millis()),
      _bucketStartAddr(),
      _ctlLen(0),
      _secSize(0),
      _maxLgcCnt(0),
      _handle(0xFF),
      _handle1(0xFF),
      _usedSector(0),
      _tbCnt((3600/seconds)|1),
      _tbCntLong(seconds)
{
	_bucketStartAddr = EEPROM.length() - 9; 
        for (uint8_t i = 0; i < 8; i++){ 
		_buckPerm[i] = e_r(_bucketStartAddr+i);
                _budgetCycles[i] = 0;
	}
}
// ----------------------------------------------------------------------------------------------------
// --- PUBLIC API ---
// ----------------------------------------------------------------------------------------------------
// Returns: Overwrite number of the partition
uint16_t EEProm_Safe_Wear_Level::config(uint16_t startAddress, uint16_t totalBytesUsed, uint8_t PayloadSize, uint8_t cntLengthBytes, uint8_t budgetCycles, uint8_t handle) {
     
     if (startAddress+totalBytesUsed >= _bucketStartAddr) totalBytesUsed = _bucketStartAddr-startAddress;

     // Calculates the pointer to the start of the partition in the RAM Handle
     uint8_t* ramPtr = _ramStart + ((size_t)handle * CONTROL_STRUCT_SIZE);
     // Assignment of the calculated pointer to _controlCache (with casting)
     _controlCache = (ControlData*) ramPtr;
     _handle = handle;

     _buckCyc = budgetCycles;
     //_buckPerm[handle>>5] = 60;   // for testing

    uint16_t success = 1; _startAddr = startAddress;
    _cntLen = min(cntLengthBytes, 4);
    _ctlLen = _cntLen + 1;

    // 1. Check and set payload size
    _pldSize = (PayloadSize < DEFAULT_PLD_SIZE) ? DEFAULT_PLD_SIZE : PayloadSize;

    // Calculation of the maximum possible physical sectors
    _numSecs = (totalBytesUsed - METADATA_SIZE) / (_pldSize + _ctlLen);

    // If at least 1 sector is not calculated, it must terminate with an
    // error!
    if (_numSecs < 1) { success = 0; _numSecs = 1; }

    // Ensures that the rollout is triggered exactly after a full number of
    // rotations. With adjustment for the sector counter size.
     #define maxCapacity (uint32_t)(1UL << (_cntLen * 8)) - 1
    _maxLgcCnt = ((maxCapacity) / _numSecs) * _numSecs;

    // Calculate the exact sector size
    _secSize = _pldSize + _ctlLen;

    // Write address initially unknown
    _nextPhSec = 0;

    if(_ioBufSize<_secSize){
        delete[] _ioBuf;
	    _ioBuf = new uint8_t [_secSize];
        _ioBufSize = _secSize;
    }

    if (success > 0) {
        _checksum = chkSum();
        initialize(false, handle);
        success = ((uint16_t)e_r(_startAddr + 3) << 8 ) | e_r(_startAddr + 2);
    }

    _checksum = chkSum();
    
    return success;
}
// ----------------------------------------------------------------------------------------------------

bool EEProm_Safe_Wear_Level::initialize(bool forceFormat, uint8_t handle) {
    _START_
    // 2. Check: config() must have been successful (at least 2 sectors)
    bool success =  (_numSecs < 1) ? 0 : 1;

    if (success == 1){
	    // --- 3. CHECKING METADATA (Magic ID and Version) ---

        trans16(_startAddr, &_ioBuf[0]);
	    trans16(_pldSize, &_ioBuf[2]);
	    trans16(_numSecs, &_ioBuf[4]);
	    _ioBuf[6] = _cntLen;
	    uint8_t c_hash = calculateCRC(_ioBuf, 7);

	    // Read Magic ID 
	    uint8_t magicID_read = e_r(_startAddr);
	    // Read Config Hash
	    uint8_t c_hash_read = e_r(_startAddr+1);
   
	    // Check: Magic ID or Parameters incorrect?
        if (magicID_read != MAGIC_ID){
    	    e_w(_startAddr + 2,0x00);
    		e_w(_startAddr + 3,0x00);
            for (uint8_t f = 0; f < 4; f++){ 
		            _buckPerm[f]=143;
			}
            e_c;
			updateBuckets();
	    };
	    if (magicID_read != MAGIC_ID || forceFormat == true || c_hash != c_hash_read) {  
	        _status = 4;
	        // Necessary: First use or version conflict -> Format!
	        formatInternal(handle);
		    e_w(_startAddr + 0, MAGIC_ID);
		    e_w(_startAddr + 1, c_hash);
	        e_c;
	        _nextPhSec = 0;
	    }else {
	        // --- 4. RESTORATION ---
	        // If the metadata is valid, find the latest sector.
	        findMarginalSector(handle,0);
        }

	    // After findMarginalSector(), the state is set either to the latest sector
	    // or (if no sector was valid) to counter 0.
    }
    _END_
    return success;
}

// ----------------------------------------------------------------------------------------------------

uint16_t EEProm_Safe_Wear_Level::getOverwCounter(uint8_t handle) {
    _START_
    // We read the 16-bit version from addresses _startAddr + 2 and + 3
    union U16toB {uint16_t u16;uint8_t u8[2];};
    U16toB version_read;
    version_read.u8[0] = e_r(_startAddr + 2);
    version_read.u8[1] = e_r(_startAddr + 3);
    _END_
    return version_read.u16; // Returns the correct 16-bit value.
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
	            for (i = 0; i < maxSize; i++) { value[i] = _ioBuf[i]; }
	        } else _status = 6;
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
	     size_t actPhysSec = _nextPhSec; _nextPhSec--;
	
	     while (count > 0 && _nextPhSec != actPhysSec) {
             	success = loadPhysSector(_nextPhSec, handle); cli();
        	    if (success == true) { count--; count1++; } 
	            _nextPhSec--;
	     }
    }

    // now migrate a sector to target partition
    while (count1 > 0){
	    _start(targetHandle); uint16_t i=1;
	    while (write(_ioBuf,targetHandle) == 0 && i < _numSecs) { i++; }
        if (i==_numSecs) count1 = 1;
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

    if (physSector == -1) physSector = _numSecs-1; 
	else if (physSector > _numSecs) physSector = 0;
 
    _nextPhSec = physSector;

    if (physSector == 0) physSector = _numSecs - 1;
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
        success = 1;
        _status = 1;
        if (_usedSector == 0) _status = 7;
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
    if (_numSecs < 1 || _curLgcCnt > _maxLgcCnt) {
        success = 0;
        if(_curLgcCnt > _maxLgcCnt) _status = 3;
    } else success = 1;

    uint16_t slen = strlen(value);

    if (success == 1){
    	if (slen > _pldSize) _status = 2;
    	for (uint16_t i = 0; i < _pldSize; i++) {
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
    	uint8_t bI = handle >> 3; 
    	if (_budgetCycles[bI] > 0) _budgetCycles[bI]--;
    	else if (_buckPerm[bI] > 0) {
			      _budgetCycles[bI] = _buckCyc;
	              _buckPerm[bI]--;	
	     	 } else { 
			        success = 0; 
                    _status = (_budgetCycles[bI]==0) ? 8:(_budgetCycles[bI]<64) ? 9:(_budgetCycles[bI]>63) ? 10: 11;
		     }
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
    	if (_nextPhSec >= _numSecs) _nextPhSec = 0;
    	
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
    _END_
    uint32_t success = 1;
    if (_curLgcCnt >= _maxLgcCnt){_status = 3; success = 0;}
    if(success==1) success = _maxLgcCnt - _curLgcCnt;
    return success;
}

// Remaining cycles in percent
uint8_t EEProm_Safe_Wear_Level::healthPercent(uint32_t cycles, uint8_t handle){
    _START_
    _END_
    #define _cycles_ cycles/1000
    // 1. Calculate the absolute total lifetime (TL) (100% limit)
    // TL = Endurance per sector (_cycles_) * Total number of sectors (_numSecs)
    #define totalLifetime (uint32_t)_numSecs * _cycles_
    // 2. Calculate Consumed Cycles (CC) (current wear)
    // CC = (Overwrite Counter * Max Logical Counter) + Current Logical Counter
    #define consumedCycles (((uint32_t)getOverwCounter(handle) * _maxLgcCnt)+_curLgcCnt) / 1000UL
    // (Remaining Cycles * 100) / Total Life
    return (uint8_t)(( (totalLifetime) - (consumedCycles) ) * 100UL) / (totalLifetime);
}
// ----------------------------------------------------------------------------------------------------
// internal time management

void EEProm_Safe_Wear_Level::oneTickPassed(){
 	_tbCntN--; 
    if(_tbCntN == 0){
    	_tbCntN = _tbCnt;

		if (_tbCntLong < 3600) updateBuckets();
    	else _accumulatedTime += _tbCntLong;
        	
		while (_accumulatedTime > 3599){
       		_accumulatedTime -= 3600;
			updateBuckets();
		}
    }
}

void EEProm_Safe_Wear_Level::idle() {
    #define lastTime  (uint16_t)(millis() / 60000)
    if ((lastTime - _buckTime) > 60){
         _buckTime = lastTime;
         updateBuckets();
	}
}

// ----------------------------------------------------------------------------------------------------

uint8_t EEProm_Safe_Wear_Level::getWrtAccBalance(uint8_t handle) {
    return _buckPerm[handle>>5];
}

// ----------------------------------------------------------------------------------------------------
// --- PRIVATE HELPER ---
// ----------------------------------------------------------------------------------------------------

bool EEProm_Safe_Wear_Level::findNewestSector(uint8_t handle) {
    _START_
    _END_
    return findMarginalSector(handle,0);
}

// ----------------------------------------------------------------------------------------------------

void EEProm_Safe_Wear_Level::updateBuckets(){
  for (uint8_t i = 0; i < 8; i++){ 
       if (_buckPerm[i] < 255) _buckPerm[i]++;
       if (e_r(_bucketStartAddr+i) != ((_buckPerm[i] < 64) ? 0 : 127)){
               e_w(_bucketStartAddr+i, (_buckPerm[i] < 64) ? 0 : 127);
               e_c;
       }
  }
}

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
            _ioBuf[x] = e_r(_startAddr + METADATA_SIZE + (c * _secSize) + x);
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

	union U16toB {uint16_t u16;uint8_t u8[2];}; 
    U16toB __EEPRWL_VER; __EEPRWL_VER.u16 = _EEPRWL_VER; 
    __EEPRWL_VER.u8[0] = e_r(_startAddr + 2);
    __EEPRWL_VER.u8[1] = e_r(_startAddr + 3);
    __EEPRWL_VER.u16++;
    e_w(_startAddr + 2, __EEPRWL_VER.u8[0]);
    e_w(_startAddr + 3, __EEPRWL_VER.u8[1]);
    e_c;

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
    	_maxLgcCnt = (maxCapacity / _numSecs) * _numSecs;
    }
    
	if (_checksum != chkSum()) { _status = 5; sei(); success = 0; }
    
	return success;
}

void EEProm_Safe_Wear_Level::_end(){
    _checksum = chkSum(); sei();
}

// ----------------------------------------------------------------------------------------------------
// END OF CODE

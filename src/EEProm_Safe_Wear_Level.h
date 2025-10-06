/*****************************************************************************************************
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
 *****************************************************************************************************
 *
 * --- GLOSSARY OF ABBREVIATIONS (Internal Variables & Constants) ---
 *
 * MEMBER VARIABLES (State):
 * _startAddr  : startAddress
 * _totalBytesUsed : totalLength
 * _numSecs    : numSectors (Number of physical sectors)
 * _secSize    : sectorSize (Size of one sector including overhead)
 * _pldSize    : payloadSize (Size of the pure user data/payload)
 * _nextPhSec  : nextWriteSector (Next physical sector to write to)
 * _maxLgcCnt  : maxSectorNumber (Maximum logical counter value)
 * _curLgcCnt  : currentCounter (Current logical counter value)
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
 * -------------------------------------------------------------
 */
#ifndef EEPROM_WEAR_LEVEL_H
#define EEPROM_WEAR_LEVEL_H

// ----------------------------------------------------------------------------------------------------
// --- INCLUDES & MACROS ---
// ----------------------------------------------------------------------------------------------------
#include <EEPROM.h>
#include <Arduino.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <EEProm_Safe_Wear_Level_Macros.h>
// ----------------------------------------------------------------------------------------------------
// --- CLASS DEFINITION ---
// ----------------------------------------------------------------------------------------------------
class EEProm_Safe_Wear_Level {
    public:
      // Standard Constructor
      EEProm_Safe_Wear_Level(uint8_t* ramHandlePtr);

      // --- PUBLIC API (Implementation in .cpp) ---
      // Parameter names (startAddress, totalBytesUsed, PayloadSize) are deliberately NOT abbreviated.
      uint16_t config(uint16_t startAddress, uint16_t totalBytesUsed, uint16_t PayloadSize, uint8_t cntLengthBytes, uint8_t handle);
      uint16_t getVersion(uint8_t handle);
      bool setVersion(uint16_t value, uint8_t handle);
      
      // Remaining cycles
      uint32_t healthCycles(uint8_t handle);
      
      // Remaining cycles in percent
      uint8_t healthPercent(uint8_t handle);
      
      // Loads sector into the cache (Implementation in .cpp)
      uint16_t loadPhysSector(uint16_t physSector, uint8_t handle);
      
      // --- GENERIC TEMPLATE FUNCTIONS ---
      
      template <typename T>
      bool write(const T& value, uint8_t handle);
      template <typename T>
      bool read(T& value, uint8_t handle, size_t maxSize);
      template <typename T>
      uint32_t getCtrlData(int offs, int handle);
      
      // --- EXPLICIT OVERLOADS FOR C-STRINGS (Implementation in .cpp) ---
      
      bool write(const char* value, uint8_t handle);
      bool read(char* value, uint8_t handle);
      bool read(char* value, uint8_t handle, size_t maxSize);
      uint32_t getCtrlData(int offs, int handle);


    private:
      // --- INTERNAL STATE VARIABLES (Names adapted) ---      
      uint8_t * _ioBuf = nullptr;
      ControlData* _controlCache;            
      // Version control
      uint8_t _EEPRWL_VER = 0;
      bool _start(uint8_t handle);
      void _end();
      void _read(uint8_t handle);

      // Static inline function to encapsulate byte reconstruction
      // and allow the compiler to deduplicate the code.
      // 1. Little-Endian read logic (for 3x redundancy: load, find, getCtrlData)
        static inline uint32_t readLE(const uint8_t* buffer, uint8_t length) {
        uint32_t value = 0;
        for (uint8_t i = 0; i < length; i++) {
            value |= (uint32_t)buffer[i] << (i * 8);
        }
        return value;
      }
      // 2. Little-Endian write logic (for 1x redundancy: _write)
      static inline void writeLE(uint8_t* buffer, uint32_t value, uint8_t length) {
        for (uint8_t i = 0; i < length; i++) {
            buffer[i] = (uint8_t)(value >> (i * 8));
        }
      }
     // 3. We calculate the addition checksum over all bytes of the ControlData cache
     // from offset 0 up to the byte before the checksum (Byte 13: status).
     inline uint16_t chkSum() {
        const size_t CHECKSUM_RANGE = 13; uint8_t check = 0, check1 = 0;
        // We cast _controlCache (ControlData*) to uint8_t* to access byte by byte
        uint8_t* controlDataPtr = (uint8_t*)_controlCache;
        // 2. Calculate addition checksum
        for (size_t i = 0; i < CHECKSUM_RANGE; i++) {
            check += controlDataPtr[i];
            check1 += check;
        }
        return uint16_t (check << 8) | check1;
     }
      
      // --- PRIVATE HELPERS (Implementation in .cpp) ---
      bool findLatestSector(uint8_t handle);
      uint8_t calculateCRC(const uint8_t * buffer, size_t length);
      void formatInternal(uint8_t handle);
      bool _write(uint8_t handle);
      
      // --- INTERNAL CONSTANTS (Static, declaration adapted) ---
      // CRC_OVERHEAD, MAGIC_ID, and METADATA_SIZE remain for readability.
      static const size_t CRC_OVERHEAD;
      static const uint16_t MAGIC_ID;
      static const size_t METADATA_SIZE;
      static const size_t DEFAULT_PLD_SIZE;
      uint8_t  _ctlLen;     
      uint8_t* _ramStart;
      uint16_t _ioBufSize;
      uint16_t _secSize; 
      uint32_t _maxLgcCnt;
      uint8_t  _handle;
      uint8_t  _handle1;
};

// ----------------------------------------------------------------------------------------------------
// --- TEMPLATE IMPLEMENTATIONS (Names adapted) ---
// ----------------------------------------------------------------------------------------------------

template <typename T>
bool EEProm_Safe_Wear_Level::write(const T& value, uint8_t handle = 0) {     
      _START_
      bool success;
      // Consistency check
      if (_numSecs < 2 || _curLgcCnt >= _maxLgcCnt){
         success = 0;
         if(_curLgcCnt >= _maxLgcCnt) _status = 3;
      }else success = 1;
      if (success == 1){
         if (sizeof(T) > _pldSize) _status = 2;

         uint8_t * valuePtr = (uint8_t *)&value;
         uint16_t i;

         for(i = 0; i < _pldSize; i++){
              if(i < sizeof(T)) _ioBuf[i] = valuePtr[i];
              else _ioBuf[i] = 0;
         }
         success = _write(handle);
    }
    _END_
    return success;
}

// ----------------------------------------------------------------------------------------------------

template <typename T>
bool EEProm_Safe_Wear_Level::read(T& value, uint8_t handle = 0, size_t maxSize = 0) {
    _START_
      
    _read(handle);

      // Check the cache status
      // Only if the status uint8_t is 1, the data is valid.
      if (_ioBuf[_secSize - 1] == 0) return false;
      
      // Copy data from _ioBuf to the target variable
      uint8_t * valuePtr = (uint8_t *)&value;
      uint16_t size = sizeof(T);
      if (maxSize > _pldSize) {
         maxSize = _pldSize;
      }
      if(maxSize>0) size = maxSize;
      if(size > _pldSize) size = _pldSize;
      
      memcpy(valuePtr, _ioBuf, size); 

     _END_
      return true;
}
// ----------------------------------------------------------------------------------------------------
#endif // EEPROM_WEAR_LEVEL_H

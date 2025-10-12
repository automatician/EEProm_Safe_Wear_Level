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
      EEProm_Safe_Wear_Level(uint8_t* ramHandlePtr, uint16_t seconds = 8);

      // Check account balance for write cycles
      uint8_t getWrtAccBalance(uint8_t handle);

      // --- PUBLIC API (Implementation in .cpp) ---
      // Parameter names (startAddress, totalBytesUsed, PayloadSize) are deliberately NOT abbreviated.
      uint16_t config(uint16_t startAddress, uint16_t totalBytesUsed, uint8_t PayloadSize, uint8_t cntLengthBytes, uint8_t budgetCycles, uint8_t handle);
      uint16_t getOverwCounter(uint8_t handle);
      bool initialize(bool forceFormat, uint8_t handle);
      
      // Remaining cycles
      uint32_t healthCycles(uint8_t handle);
      
      // Remaining cycles in percent
      uint8_t healthPercent(uint32_t cycles, uint8_t handle);

      // Loads sector into the cache (Implementation in .cpp)
      uint16_t loadPhysSector(uint16_t physSector, uint8_t handle);
      bool migrateData(uint8_t sourceHandle, uint8_t targetHandle, uint16_t count);
      // ----------------------------------------------------------------------------------------------------
	  uint32_t getCtrlData(uint8_t offs, uint8_t handle){
      	    static uint8_t const leng[] = {4,0,0,0, 2,0, 2,0, 2,0, 1, 1, 2,0, 1, 1};
            _START_
	        int start_index = (handle * 16) + offs;
	        uint32_t value = 0;
	        uint8_t read_len = leng[offs];

	        if (read_len > 0 && read_len <= sizeof(uint32_t)) {
	            value = readLE(&_ramStart[start_index], read_len);
	        }
	        _END_
	        return value;
	  }
      // ----------------------------------------------------------------------------------------------------
      // internal time management
      void oneTickPassed();
	  void idle();
      // ----------------------------------------------------------------------------------------------------
      // --- GENERIC TEMPLATE FUNCTIONS ---
      
      template <typename T>
      bool write(const T& value, uint8_t handle);
      template <typename T>
      bool read(uint8_t ReadMode, T& value, uint8_t handle, size_t maxSize = 0);

      // --- EXPLICIT OVERLOADS FOR C-STRINGS (Implementation in .cpp) ---
      
      bool write(const char* value, uint8_t handle);
      bool read(uint8_t ReadMode, char* value, uint8_t handle, size_t maxSize = 0);
   
private:
      // --- INTERNAL STATE VARIABLES (Names adapted) ---      
      uint8_t * _ioBuf;
      uint8_t * _buckPerm;
      uint8_t * _budgetCycles;
      uint16_t  _buckTime = 0;
      uint16_t  _tbCnt,_tbCntN, _tbCntLong, _accumulatedTime = 0;
      uint16_t  _bucketStartAddr;
      ControlData* _controlCache;            

      // ----------------------------------------------------------------------------------------------------
      // Version control
      uint8_t _EEPRWL_VER = 0;
      // ----------------------------------------------------------------------------------------------------
      
      bool _start(uint8_t handle);
      void _end();
      void _read(uint8_t ReadMode, uint8_t handle);
      bool findNewestSector(uint8_t handle);

      // ----------------------------------------------------------------------------------------------------
      // internal time management
        void updateBuckets();
      // ----------------------------------------------------------------------------------------------------

      // ----------------------------------------------------------------------------------------------------
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
      inline uint8_t chkSum() {
        const size_t CHECKSUM_RANGE = 13; uint8_t check = 0, check1 = 0;
        // We cast _controlCache (ControlData*) to uint8_t* to access byte by byte
        uint8_t* controlDataPtr = (uint8_t*)_controlCache;
        // 2. Calculate addition checksum
        for (size_t i = 0; i < CHECKSUM_RANGE; i++) {
            check += controlDataPtr[i];
            check1 += check;
        }
        return check + check1;
      }
      inline void trans16(uint16_t value, uint8_t* target_ptr) {
         union U16toB {
             uint16_t u16;
             uint8_t u8[2];
         };
         U16toB converter;
         converter.u16 = value;
         *target_ptr = converter.u8[0];
         *(target_ptr + 1) = converter.u8[1]; 
      }
      // ----------------------------------------------------------------------------------------------------

      // --- PRIVATE HELPERS (Implementation in .cpp) ---
      bool findMarginalSector(uint8_t handle, uint8_t margin);
      uint8_t calculateCRC(const uint8_t * buffer, size_t length);
      void formatInternal(uint8_t handle);
      bool _write(uint8_t handle);
      
      // --- INTERNAL CONSTANTS (Static, declaration adapted) ---
      // CRC_OVERHEAD, MAGIC_ID, and METADATA_SIZE remain for readability.
      uint8_t  _ctlLen;     
      uint8_t* _ramStart;
      uint16_t _ioBufSize;
      uint16_t _secSize; 
      uint32_t _maxLgcCnt;
      uint8_t  _handle;
      uint8_t  _handle1;
      uint8_t  _usedSector;
};

// ----------------------------------------------------------------------------------------------------
// --- TEMPLATE IMPLEMENTATIONS (Names adapted) ---
// ----------------------------------------------------------------------------------------------------
/*
 * DESIGN RATIONALE: STACK RELIABILITY AND RUNTIME EFFICIENCY
 *
 * This implementation prioritizes **SRAM/Stack Reliability** and **Runtime Speed** over maximum 
 * minimizing the Flash Memory Footprint.
 *
 * ARCHITECTURE:
 * The entire read/write logic (EEPROM, wear leveling) is placed **DIRECTLY** within the template
 * bodies(leading to "Template Code Bloat"), thus eliminating the need for an internal wrapper 
 * functions.
 *
 * THE COMPROMISE (Cost vs. Benefit):
 *
 * 1. FLASH OVERHEAD (Cost):
 * This method increases **Program Memory (FLASH)** usage for a few bytes. However, the resulting
 * increase is relatively low.
 *
 * 2. STACK & RUNTIME BENEFIT (Gain):
 * Eliminating the wrapper function call overhead achieves faster execution and, critically, 
 * **lower Stack (SRAM) usage** at runtime. This reduction in Stack Depth minimizes the burden on the 
 * scarce SRAM (2 KB on ATmega devices) and significantly enhances the overall stability of 
 * the program.
 *
 * The ultimate priority is set on maximizing **SRAM/Stack reliability** by protecting this critical 
 * resource.
 *
 */
 template <typename T>
 bool EEProm_Safe_Wear_Level::write(const T& value, uint8_t handle) {     
      _START_
      bool success;
      // Consistency check
      if (_numSecs < 1 || _curLgcCnt >= _maxLgcCnt){
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
 bool EEProm_Safe_Wear_Level::read(uint8_t ReadMode, T& value, uint8_t handle, size_t maxSize) {
    _START_
      
    _read(ReadMode, handle);
    uint8_t success = _ioBuf[_secSize - 1];

    if (success > 0){      
      // Copy data from _ioBuf to the target variable
      uint8_t * valuePtr = (uint8_t *)&value;
      uint16_t size = sizeof(T);
      if (maxSize > _pldSize) {
         maxSize = _pldSize;
      }
      if(maxSize>0) size = maxSize;
      if(size > _pldSize) size = _pldSize;
      
      memcpy(valuePtr, _ioBuf, size); 
    }
     _END_
      return success;
 }

// ----------------------------------------------------------------------------------------------------
#endif // EEPROM_WEAR_LEVEL_H
// END OF CODE

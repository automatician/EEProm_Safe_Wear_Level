#ifndef EEPROM_SAFE_WEAR_LEVEL_MACROS_H
#define EEPROM_SAFE_WEAR_LEVEL_MACROS_H

// IMPORTANT: Adjust correct Handle size (19 Bytes) and field names!

// -----------------------------------------------------------
// 1. ADDR Base Macro (L-Value: Allows BOTH Read AND Write)
// -----------------------------------------------------------
#define ADDR(offset, type) \
    *(type*)(ramHandle + offset) 
    
// -----------------------------------------------------------
// 2. ONE Macro List for ALL Accesses
// -----------------------------------------------------------
#define _curLgcCnt          (*_controlCache).curLgcCnt
#define _nextPhSec          (*_controlCache).nextPhSec
#define _startAddr          (*_controlCache).startAddr
#define _pldSize            (*_controlCache).pldSize
#define _numSecs            (*_controlCache).numSecs
#define _cntLen             (*_controlCache).cntLen
#define _status             (*_controlCache).status
#define _checksum           (*_controlCache).checksum
#define CONTROL_STRUCT_SIZE 16

// -----------------------------------------------------------
// 3. SETUP Macro
// -----------------------------------------------------------
#define _START_ if(_start(handle)==0) return 0;
#define _END_ _end();

// The structure of the control data per partition
// '__attribute__((packed))' ensures tight packing for exact size.
typedef struct __attribute__((packed)) {
    uint32_t curLgcCnt;      // Offset 0 (4 B)
    uint16_t nextPhSec;      // Offset 4 (2 B)
    uint16_t startAddr;      // Offset 6 (2 B)
    uint16_t pldSize;        // Offset 8 (2 B)
    uint16_t numSecs;        // Offset 10 (2 B) 
    uint8_t  cntLen;         // Offset 12 (1 B)
    uint8_t  status;         // Offset 13 (1 B)
    uint16_t checksum;       // Offset 14 (2 B)
} ControlData; 
// Total size: 16 Bytes

#define e_w EEPROM.write
#define e_r EEPROM.read

#if defined(ESP8266) || defined(ESP32)
     #define e_c EEPROM.commit()
#else
     #define e_c do {} while(0)
#endif

#endif // EEPROM_SAFE_WEAR_LEVEL_MACROS_H

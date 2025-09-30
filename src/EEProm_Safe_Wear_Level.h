/* 
 * EEProm_Safe_Wear_Level.h
 ******************************************************************************
 * EEProm_Safe_Wear_Level Library v25.09.30
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
 ******************************************************************************/
#ifndef EEPROM_WEAR_LEVEL_H
#define EEPROM_WEAR_LEVEL_H

// --- INCLUDES ---
#ifndef __AVR__
  #include <EEPROM.h>
  #define EEPROMWL_USING_STANDARD_LIB 
#else
  #include <avr/io.h>         
  #include <avr/interrupt.h>  
#endif
#include <Arduino.h>      
#include <stddef.h>       
#include <string.h>       
#include <stdint.h>

#ifdef __AVR__
#define eeprom_write eeprwl_write
#define eeprom_read eeprwl_read
#else
#define eeprom_write EEPROM.write
#define eeprom_read EEPROM.read
#endif



class EEProm_Safe_Wear_Level {
public:
    // Standard-Konstruktor
    EEProm_Safe_Wear_Level();


    // --- ÖFFENTLICHE API (Implementierung in .cpp) ---
    byte config(int startAddress, int totalBytesUsed, size_t PayloadSize);
    uint8_t getVersion();	
    bool setVersion(uint8_t value);
    
    // Sektoren/Datensätze in Partition 	
    uint16_t getNumSectors() const { 
        return _numSectors; 
    }
    // nächster ph. Sektor zum Schreiben
    uint16_t getNextWriteSector() const {
        return nextWriteSector;
    }
    //max. logische Sektornummer 
    uint32_t getMaxSectorNumber() const {
        return maxSectorNumber;
    }
    //aktuelle logische Sektornummer 
    uint32_t getSectorNumber() const {
        return currentCounter;  //aktuelle logische Sektornummer
    }

    uint32_t healthCycles();
    uint8_t healthPercent();
    bool loadRelativeSector(uint16_t physSector);

    /**
     * @brief Generischer Template-Prototyp.
     */
    template <typename T>
    uint16_t write(const T& value);

    /**
     * @brief Generischer Template-Prototyp.
     */
    template <typename T>
    bool read(T& value);

    // --- Explizite Überladung für C-Strings/Char-Arrays (Implementierung MUSS in .cpp erfolgen) ---
    // Diese Funktionen verursachten den Fehler, da ihre Implementierung fälschlicherweise in einem Header war.
    // Sie bleiben als normale Prototypen, MÜSSEN aber in die EEProm_Safe_Wear_Level.cpp verschoben werden.
    uint16_t write(const char* value); 
    bool read(char* value); 
    bool read(char* value, size_t maxSize); // Bleibt, da es nicht sauber ins generische Template passt.


private:
    // --- INTERNE ZUSTANDSVARIABLEN ---
    int _startAddress = 0;
    int _totalLength = 0;
    int _numSectors = 0;
    size_t _sectorSize = 0;
    size_t _payloadSize = 0;
    uint16_t nextWriteSector = 0;  //aktueller nächster physikalischer Sektor
    uint32_t maxSectorNumber = 0;  //max. logische Sektornummer
    uint32_t currentCounter = 0;  //aktuelle logische Sektornummer

    uint8_t* _ioBuffer = nullptr; 
    
    // Zaehler-Logik
    uint32_t _currentCounter = 0;
    uint8_t _currentCounterSize_Bytes = 0;
    
    // Versionskontrolle
    uint8_t _EEPRWL_VERSION = 0; 

    // --- PRIVATE HELFER (Implementierung in .cpp) ---
    bool findLatestSector();
    uint8_t calculateCRC(const uint8_t* buffer, size_t length);
    uint8_t getMinBytesNeeded(uint32_t maxValue);
    void performCommit();
    void formatInternal();
#ifdef __AVR_ATmega328P__
    void eeprwl_write(unsigned int Address, unsigned char Data);
    unsigned char eeprwl_read(unsigned int Address);
#endif
    // --- INTERNE KONSTANTEN (Statisch, müssen in .cpp definiert werden) ---
    static const size_t CRC_OVERHEAD; 
    static const uint16_t MAGIC_ID; 
    static const size_t METADATA_SIZE; 
    static const size_t DEFAULT_PAYLOAD_SIZE; 
};

// ----------------------------------------------------------------------
template <typename T>
uint16_t EEProm_Safe_Wear_Level::write(const T& value) {

    uint16_t success = true;
    
    // Konsistenzprüfung
    if (_numSectors < 2 || currentCounter >= maxSectorNumber) {
        return false;
    }

	currentCounter++; const uint8_t* valuePtr = (const uint8_t*)&value;

	int i; int c;
	for(i=0; i < _payloadSize; i++){
		if(i < sizeof(T)) _ioBuffer[i]=valuePtr[i];
		else _ioBuffer[i]=0;
	}
	for (c=0; c < _currentCounterSize_Bytes; c++) {
   		_ioBuffer[i+c] = (uint8_t)(_currentCounter >> (c * 8));
	}
	_ioBuffer[i+c] = calculateCRC(_ioBuffer,_sectorSize-1);

	// Daten schreiben
	for(int x=0; x < _sectorSize; x++){
		eeprom_write(_startAddress+METADATA_SIZE+(nextWriteSector*_sectorSize)+x, _ioBuffer[x]);
	}
	
	uint16_t sek = nextWriteSector;
	nextWriteSector++;
	if(nextWriteSector > _numSectors) nextWriteSector = 0;

	// Daten vergleichen
	uint8_t bufByte;
	for(i=0; i < _sectorSize; i++){
		bufByte = eeprom_read(_startAddress+METADATA_SIZE+(sek*_sectorSize)+i);
		if(bufByte != _ioBuffer[i]){ success = false; break; }
	}

	if(success == true) {
		_ioBuffer[_sectorSize-1] = 1;
		success = nextWriteSector; 
		if(success==0)success=1;
	}
	else _ioBuffer[_sectorSize-1] = 0;

    return success;
}
// ----------------------------------------------------------------------
template <typename T>
bool EEProm_Safe_Wear_Level::read(T& value) {
    
    // Prüfe den Status des Caches
    // Nur wenn das Status-Byte 1 ist, sind die Daten gueltig.
    if (_ioBuffer[_payloadSize + _currentCounterSize_Bytes] == 0) {
        return false;
    }
    
    // Daten aus dem _ioBuffer in die Zielvariable kopieren
    uint8_t* valuePtr = (uint8_t*)&value;
    int size = sizeof(T);
    if(size > _payloadSize) size = _payloadSize;
    int i;

    for (i = 0; i < size; i++) {
        // Kopiere nur die Groesse des Zieltyps
        valuePtr[i] = _ioBuffer[i]; 
    }

    return true;
}


#endif // EEPROM_WEAR_LEVEL_H


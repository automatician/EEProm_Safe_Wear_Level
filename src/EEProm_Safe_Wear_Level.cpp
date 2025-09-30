/* 
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
 ******************************************************************************
 * EEProm_Safe_Wear_Level.cpp
 */

#include "EEProm_Safe_Wear_Level.h"

// --- DEFINITION DER STATISCHEN KONSTANTEN ---
// Diese Werte ermöglichen der config() Funktion, zu arbeiten, bevor wir sie mit Logik füllen.
const size_t EEProm_Safe_Wear_Level::CRC_OVERHEAD = 1; 
const uint16_t EEProm_Safe_Wear_Level::MAGIC_ID = 0x574C; 
const size_t EEProm_Safe_Wear_Level::METADATA_SIZE = 3; 
const size_t EEProm_Safe_Wear_Level::DEFAULT_PAYLOAD_SIZE = 1;

// ----------------------------------------------------------------------
// --- KONSTRUKTOR ---
// ----------------------------------------------------------------------
EEProm_Safe_Wear_Level::EEProm_Safe_Wear_Level() 
    : _startAddress(0), _totalLength(0), _numSectors(0), _sectorSize(0), _payloadSize(0),
      _currentCounterSize_Bytes(0), currentCounter(0),
      nextWriteSector(0),
      _EEPRWL_VERSION(0) 
{}

// ----------------------------------------------------------------------
// --- ÖFFENTLICHE API (Gerüste) ---
// ----------------------------------------------------------------------
// Rueckgabe: Override-Nummer der Partition

byte EEProm_Safe_Wear_Level::config(int startAddress, int totalBytesUsed, size_t PayloadSize) {

    	byte success = 1;
    	_startAddress = startAddress;
    
    	// 1. Payload-Groesse pruefen und setzen
    	if (PayloadSize < DEFAULT_PAYLOAD_SIZE) {
        	_payloadSize = DEFAULT_PAYLOAD_SIZE; 
    	} else {
        	_payloadSize = PayloadSize;
    	}

	// Berechnung der maximal möglichen Sektoren (für Zählergrößen-Bestimmung)
    	// Wir nehmen an, dass der Overhead nur 1 Byte (CRC) ist.
    	_numSectors = (totalBytesUsed - METADATA_SIZE) / (_payloadSize + CRC_OVERHEAD); 
	// Wenn nicht mindestens 1 Sektoren berechnet wird, muss mit einem
	// Fehler beendet werden!
    	if (_numSectors < 2){ success = 0; _numSectors = 1;}
	
	// Wir haben jetzt die Zahl der Sektoren in _numSectors
	// Daraus berechnen wir den Mindestzählerstand, bis zum Lebenszeitende

	// Anzahl benötigter Bytes für den Zähler
	maxSectorNumber = _numSectors * 100000L;
	_currentCounterSize_Bytes = getMinBytesNeeded(maxSectorNumber); 

	// Genaue Sektorgröße berechnen
	_sectorSize = _payloadSize + _currentCounterSize_Bytes + CRC_OVERHEAD;

	// Endgültige Sektorenzahl
	_numSectors = (totalBytesUsed - METADATA_SIZE) / _sectorSize; 
	if (_numSectors < 2){ success = 0; _numSectors = 1; }

	nextWriteSector = 0; // Schreibadresse zunächst unbekannt

	if (_ioBuffer != nullptr) delete[] _ioBuffer;
	_ioBuffer = new uint8_t[_sectorSize];
	
	if(success>0){success=eeprom_read(_startAddress + 2);}
	return success; 
}

// ----------------------------------------------------------------------
bool EEProm_Safe_Wear_Level::setVersion(uint8_t value) {
// 1. Interne Version setzen
    _EEPRWL_VERSION = value;
    
    // 2. Prüfung: config() muss erfolgreich gewesen sein (mindestens 2 Sektoren)
    if (_numSectors < 2) {
        return false;
    }

    // --- 3. PRÜFUNG DER METADATEN (Magic ID und Version) ---
    uint16_t magicID_read = 0;
    uint8_t version_read = 0;
    
    // Magic ID lesen (High Byte -> Low Byte)
    magicID_read = (uint16_t)eeprom_read(_startAddress) << 8;
    magicID_read |= (uint16_t)eeprom_read(_startAddress + 1);
    
    // Version lesen
    version_read = eeprom_read(_startAddress + 2);

    // Pruefung: Magic ID oder Version falsch?
    if (magicID_read != MAGIC_ID || version_read != _EEPRWL_VERSION) {
        // Notwendig: Erste Nutzung oder Versionskonflikt -> Formatierung!
        formatInternal();
	eeprom_write(_startAddress, (uint8_t)(MAGIC_ID>>8));		
	eeprom_write(_startAddress+1, (uint8_t)MAGIC_ID);
	eeprom_write(_startAddress + 2,_EEPRWL_VERSION);
        nextWriteSector = 0;
        return true; 
    }
    
    // --- 4. WIEDERHERSTELLUNG ---
    // Wenn die Metadaten gültig sind, finde den letzten Sektor.
    findLatestSector();
    
    // Nach findLatestSector() ist der Zustand entweder auf den letzten Sektor
    // oder (falls kein Sektor gültig war) auf Zähler 0 gesetzt.
    
    return true; 
}

// ----------------------------------------------------------------------
uint32_t EEProm_Safe_Wear_Level::healthCycles(){
    if (nextWriteSector >= maxSectorNumber) return 0;
    return maxSectorNumber - nextWriteSector;
}
uint8_t EEProm_Safe_Wear_Level::healthPercent(){
    if(maxSectorNumber==0)maxSectorNumber=1;
    return ((float)1-((float)nextWriteSector / (float)maxSectorNumber))*100;
}

// ----------------------------------------------------------------------
// --- PRIVATE HELPER (Gerüste) ---
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
bool EEProm_Safe_Wear_Level::findLatestSector() {

	uint32_t cC = 0;
	currentCounter = 0;
	nextWriteSector = 0;
	bool success = false;

	// Alle Sektoren durchsuchen
	// i MUSS uint16_t sein, um > 255 Sektoren zu unterstützen (z.B. bei 2KB EEPROM)
	for(uint16_t i=0; i < _numSectors; i++){ 
		byte crc; byte crc1; byte x; byte c; // Stack-optimiert: x und c bleiben byte
		
		// --- 1. Sektor in den _ioBuffer lesen und CRC prüfen ---
		
		// Datenbytes und Sektorzähler lesen. Die Schleifenbedingung MUSS (_sectorSize - 1) sein, 
        	// da das letzte Byte (CRC) separat gelesen wird.
		for(x=0; x < (_sectorSize-1); x++){
			_ioBuffer[x] = eeprom_read(_startAddress+METADATA_SIZE+(i*_sectorSize)+x);
		}
		
		// Prüfsumme des Sektors aus dem EEPROM lesen
		crc = eeprom_read(_startAddress+METADATA_SIZE+(i*_sectorSize)+x);
		// CRC basierend auf den gelesenen Daten im _ioBuffer berechnen
		crc1 = calculateCRC(_ioBuffer, _sectorSize-1);
		
		if(crc == crc1){
			// Die Zaehlerbytes werden ab Position _payloadSize aus dem ioBuffer gelesen (Little Endian).
			for (uint8_t k = 0; k < _currentCounterSize_Bytes; k++) {
			    // Lese das k-te Byte des Zählers
			    cC |= (uint32_t)_ioBuffer[_payloadSize + k] << (k * 8);
			}		
			
			// --- 2. Höchsten Zählerstand finden ---
			if(currentCounter <= cC) {
				currentCounter = cC;  //log. Sektor
				nextWriteSector = i+1;  //ph. Sektor
				success = true;
			}
		}
	}

	// --- 3. Den gefundenen Sektor in den Cache kopieren und Status setzen ---
	if(success == true){
		// c muss uint32_t sein, um den Index des gefundenen Sektors zu speichern
		uint32_t c = nextWriteSector-1;
        
		// Den Sektor (Daten und Zähler) mit dem höchsten Zählerstand ERNEUT in den ioBuffer lesen.
		for(byte x=0; x < (_sectorSize-1); x++){
			_ioBuffer[x] = eeprom_read(_startAddress+METADATA_SIZE+(c*_sectorSize)+x);
		}
        
		// Das letzte Byte des ioBuffers als RAM-internes Status-Flag (1 = gültig) setzen.
		_ioBuffer[_sectorSize-1] = 1;
	} else {
        // Das Status-Flag (0 = ungültig) setzen, wenn kein gültiger Sektor gefunden wurde.
        _ioBuffer[_sectorSize-1] = 0;
    }

	// Überlauf behandeln: Wenn der Zähler am Ende der Partition angekommen ist, wieder bei 0 beginnen
	if(nextWriteSector > _numSectors) nextWriteSector = 0;

    	return success;
}



// ----------------------------------------------------------------------
uint8_t EEProm_Safe_Wear_Level::calculateCRC(const uint8_t *data, size_t length) {
    // Standard-Startwert (häufig 0xFF oder 0x00)
    uint8_t crc = 0xFF; 
    
    // Das Polynom: 0x31 (Hexadezimalwert)
    const uint8_t polynomial = 0x31; 
    
    for (size_t i = 0; i < length; i++) {
        // 1. Das aktuelle Datenbyte mit der aktuellen CRC mischen (XOR)
        crc ^= data[i]; 
        
        // 2. Acht mal shiften und XORen (Polynomdivision im Galois-Feld)
        for (uint8_t j = 0; j < 8; j++) {
            // Prüfen, ob das höchstwertige Bit (MSB) gesetzt ist
            if (crc & 0x80) {
                // Wenn gesetzt: Links-Shift und XOR mit dem Polynom
                crc = (crc << 1) ^ polynomial;
            } else {
                // Wenn nicht gesetzt: Nur Links-Shift
                crc = crc << 1;
            }
        }
    }
    return crc;
}
// ----------------------------------------------------------------------
// --- PRIVATE HELPER: getMinBytesNeeded ---
// Bestimmt die minimale Anzahl von Bytes (1, 2, 3 oder 4), die benötigt werden,
// um den gegebenen maximalen Zählwert (maxValue) zu speichern.
// ----------------------------------------------------------------------

uint8_t EEProm_Safe_Wear_Level::getMinBytesNeeded(uint32_t maxValue) {
    if (maxValue <= 0xFF) {            
        return 1; // 8-bit (byte)
    } else if (maxValue <= 0xFFFF) {   
        return 2; // 16-bit (uint16_t)
    } else if (maxValue <= 0xFFFFFF) { 
        return 3; // 24-bit
    } else {
        return 4;                      // 32-bit (uint32_t)
    }
}

// ----------------------------------------------------------------------
void EEProm_Safe_Wear_Level::performCommit() {
    // Gerüst: Tut nichts
    return;
}

// ----------------------------------------------------------------------
void EEProm_Safe_Wear_Level::formatInternal() {
	 for(int i=0; i<_sectorSize;i++){_ioBuffer[i]=0;}
	// Alle Sektoren überschreiben
	for(int i=0; i < _numSectors; i++){
		int x; int c;
		// Datenbytes schreiben
		for(x=0; x < _payloadSize; x++){
			eeprom_write((int)_startAddress+METADATA_SIZE+(i*_sectorSize)+x, (uint8_t)0x00);
		}
		// Sektorzähler schreiben
		for(c=0; c < _currentCounterSize_Bytes; c++){
			eeprom_write((int)_startAddress+METADATA_SIZE+(i*_sectorSize)+x+c, (uint8_t)0x00);
		}
		// Prüfsumme Schreiben
		eeprom_write((int)_startAddress+METADATA_SIZE+(i*_sectorSize)+x+c, (uint8_t)0xF0);
	}

    	return;
}
// ----------------------------------------------------------------------
uint8_t EEProm_Safe_Wear_Level::getVersion() {
    // Gibt den intern gespeicherten Versionswert zurück, der in setVersion()
    // gesetzt und aus dem EEPROM gelesen wird.
    return eeprom_read(_startAddress + 2);
}

// ----------------------------------------------------------------------
uint16_t EEProm_Safe_Wear_Level::write(const char* value) {

    uint16_t success = true;
    
    // Konsistenzprüfung
    if (_numSectors < 2 || currentCounter >= maxSectorNumber) {
        return 0;
    }
	currentCounter++;
	int i; int c;
	for(i=0; i < _payloadSize; i++){
		if(i < strlen(value)) _ioBuffer[i]=value[i];
		else _ioBuffer[i]=0;
	}
	for (c=0; c < _currentCounterSize_Bytes; c++) {
   		_ioBuffer[i+c] = (uint8_t)(currentCounter >> (c * 8));
	}
	_ioBuffer[i+c] = calculateCRC(_ioBuffer,_sectorSize-1);

	// Daten schreiben
	for(int x=0; x < _sectorSize; x++){
		eeprom_write(_startAddress+METADATA_SIZE+(nextWriteSector*_sectorSize)+x, _ioBuffer[x]);
	}
	
	uint32_t sek = nextWriteSector;
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
bool EEProm_Safe_Wear_Level::read(char* value) {
    
    // Prüfe den Status des Caches
    // Nur wenn das Status-Byte 1 ist, sind die Daten gueltig.
    if (_ioBuffer[_sectorSize-1] == 0) {
        return false;
    }
    
    // Daten aus dem _ioBuffer in die Zielvariable kopieren
    int i;

    for (i = 0; i < _payloadSize; i++) {
        if(value[i]=='\0' && i>0) break;
	else if(_ioBuffer[i]=='\0' && i>0) break;
	else value[i] = _ioBuffer[i]; 
    }
    if(_ioBuffer[i]=='\0'){
	if(i<_payloadSize) value[i]='\0';
	else value[i-1]='\0';
    }

    return true;
}

// ----------------------------------------------------------------------
bool EEProm_Safe_Wear_Level::read(char* value, size_t maxSize) {
    
    // Prüfe den Status des Caches
    // Nur wenn das Status-Byte 1 ist, sind die Daten gueltig.
    if (_ioBuffer[_sectorSize-1] == 0) {
        return false;
    }
    
    // Daten aus dem _ioBuffer in die Zielvariable kopieren
    int i;
    if (maxSize > _payloadSize) maxSize = _payloadSize;
    for (i = 0; i < maxSize; i++) {
	value[i] = _ioBuffer[i]; 
    }

    return true;
}
// ----------------------------------------------------------------------
bool EEProm_Safe_Wear_Level::loadRelativeSector(uint16_t physSector) {

		bool success; uint16_t x;

		if(physSector >_numSectors) physSector=0;
		nextWriteSector = physSector;
		if(physSector==0) physSector=_numSectors;
		else  physSector--;
			
		uint16_t i=physSector;
		i*=_sectorSize;
		i+=_startAddress+METADATA_SIZE;

		for(x=0; x < (_sectorSize-1); x++){
			_ioBuffer[x] = eeprom_read(i+x);
		}
		
		// Prüfsumme des Sektors aus dem EEPROM lesen
		byte crc = eeprom_read(i+x);
		
		// CRC basierend auf den gelesenen Daten im _ioBuffer berechnen
		byte crc1 = calculateCRC(_ioBuffer, _sectorSize-1);
		
		if(crc == crc1){
			// Die Zaehlerbytes werden ab Position _payloadSize aus dem ioBuffer gelesen (Little Endian).
			for (uint8_t k = 0; k < _currentCounterSize_Bytes; k++) {
			    // Lese das k-te Byte des Zählers
			    currentCounter |= (uint32_t)_ioBuffer[_payloadSize + k] << (k * 8);
			}		
			success = true;
		}else success = false;

		return success;
}

// ----------------------------------------------------------------------
#ifdef __AVR_ATmega328P__
// ----------------------------------------------------------------------
void EEProm_Safe_Wear_Level::eeprwl_write(unsigned int Address, unsigned char Data) {
    // 1. Warten, bis der vorherige Schreibvorgang abgeschlossen ist
    while(EECR & (1 << EEPE));

    // -----------------------------------------------------------------
    // Interrupts temporär deaktivieren (cli)
    // -----------------------------------------------------------------
    cli(); // Deaktiviert alle Interrupts (Critical Section Start)

    // 2. Adresse und Daten in die Register schreiben
    EEAR = Address;
    EEDR = Data;

    // 3. Schreibvorgang ermöglichen (Setzen von EEMPE)
    EECR |= (1 << EEMPE);
    
    // 4. Schreibvorgang starten (Setzen von EEPE)
    EECR |= (1 << EEPE);

    // -----------------------------------------------------------------
    // KORREKTUR: Interrupts reaktivieren (sei)
    // -----------------------------------------------------------------
    sei(); // Reaktiviert alle Interrupts (Critical Section End)
}
// ----------------------------------------------------------------------
unsigned char EEProm_Safe_Wear_Level::eeprwl_read(unsigned int Address) {
    // 1. Warten, bis der vorherige Schreibvorgang abgeschlossen ist
    while(EECR & (1 << EEPE));
    
    // 2. Adresse in das Register schreiben
    EEAR = Address;
    
    // 3. Lesebefehl senden
    EECR |= (1 << EERE);
    
    // 4. Daten aus dem Datenregister zurückgeben
    return EEDR;
}
#endif
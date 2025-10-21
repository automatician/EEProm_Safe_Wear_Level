*******************************************************************************
* EEProm_Safe_Wear_Level Library v25.10.5 
* Copyright (C) 2025, Torsten Frieser / automatician
*******************************************************************************
* This library is free software: you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as published
* by the Free Software Foundation, either version 2.1 of the License.
*******************************************************************************

===============================================================================
# EEProm_Safe_Wear_Level (v25.10.5)
===============================================================================

**WAS ES IST**

* Ein vollständiges Datenmanagement-Subsystem mit integrierter Fehlerbehandlung
  und Multi-Partition-Unterstützung, Log-Funktionalität und umfassender 
  Auditfähigkeit.
* Erfüllt die hohe Anforderung an Robustheit und Ausfallsicherheit,
  die in kommerziellen oder kritischen Projekten benötigt wird:
      - Erkennung Datenkorruption / -sicherheit im EEPROM selbst
      - Vermeidung Überschreiben von Programmspeicher bei fehlerhafter
        Anwendung
      - Absicherung der Kontrolldatenstrukturen (RAM-Handle-Struktur)
        gegen Korruption (Überschreiben / Datenänderung)
      - CRC-Prüsummen mit erhöhtem Erkennungsgrad

-------------------------------------------------------------------------------
1. ZWECK & KERNMERKMALE (ROBUSTHEIT)
-------------------------------------------------------------------------------

Die EEProm_Safe_Wear_Level Bibliothek dient als **vollständiges
DATENMANAGEMENT-SUBSYSTEM**. Ihr strategisches Ziel ist es, nicht nur die
Lebensdauer des EEPROM zu maximieren (**WEAR-LEVELING**), sondern auch, 
die **Datenintegrität** unter kritischen Bedingungen zu GARANTIEREN.

-------------------------------------------------------------------------------
2. SCHLÜSSEL-FEATURES DER v25.10.5
-------------------------------------------------------------------------------

* **TYPSICHERE I/O:** Templated Write/Read für **Structs** und primitive
  Typen in einem Aufruf.
* **AUSFALLSICHERHEIT:** Jeder Datensatz wird mit einer Prüfsumme gesichert. 
  Korrumpierte Sektoren werden ignoriert.
* **SCHREIBSICHERHEIT: ** Prüfung und Fehlerrückmeldung bei Schreibvorgängen.
* **LANGE LEBENSDAUER:** Wear-Leveling über einen **Ring-Buffer**.
* **KONFIGURIERBARE ZAEHLER: ** Passen Sie die Health-Funktionalität präzise an 
  Ihre Programmanforderungen an und steuern Sie so den Datenoverhead im EEPROM,
  zwischen 2 Byte und 5 Byte.
* **MULTI-PARTITION:** Unabhängige Verwaltung mehrerer logischer
  Datenbereiche (Handles 0, 1, 2, ...).
* **DIAGNOSE:** Detaillierte Fehlercodes (0x01 bis 0x05) erlauben eine 
  zielgerichtete Reaktion
* **LOG-FUNKTIONALITAET: ** Laden physischer Datensätze (Sektoren) und deren 
  Kontrollmöglichkeit (Log-Ende, Log-Anfang)
* **SPEICHERSCHONENDE DATENMIGRATION: ** Übertragen von Log-Einträgen auf eine
  zweite Partition, im Fall logischer Sättigung der Ursprungspartition, so 
  dass diese zurückgesetzt werden kann (kein Log-Verlust).

-------------------------------------------------------------------------------
3. INSTALLATION (MANUELL)
-------------------------------------------------------------------------------

Die Installation erfolgt manuell über den Download des Release-Archives.

### Methode: Manuelle Installation
1. Laden Sie die Release-ZIP-Datei des Repositories herunter.
2. Entpacken Sie die Datei (z.B. `EEProm_Safe_Wear_Level`).
3. Benennen Sie den Ordner in `EEProm_Safe_Wear_Level` um.
4. Kopieren Sie den Ordner in Ihr **Arduino-Bibliotheksverzeichnis**
   (`Dokumente/Arduino/libraries/`).
5. Starten Sie die Arduino IDE neu.


+--------------------------------------------------------------------------+
| 5. VOLLE API-ÜBERSICHT (Redundanzfreie Funktionen)                       |
+--------------------------------------------------------------------------+
| FUNKTION       | RÜCKGABE | ROLLE                                        |
+----------------+----------+----------------------------------------------+
| config()       | uint16_t | Definiert Partition. Gibt Override-Counter.  |
| setVersion()   | bool     | Stellt Zustand her / formatiert neu.         |
| write()        | uint16_t | **Templated Write** (Sektor-ID > 0 bei OK).  |
| read()         | bool     | **Templated Read** (TRUE bei OK).            |
| getCtrlData()  | uint32_t | Zentrale Abfrage Metadaten (Status, Zaehler).|
| healthCycles() | uint32_t | Geschaetzte verbleibende Schreibzyklen.      |
| healthPercent()| uint8_t  | Verbleibende Lebensdauer in Prozent.         |
| getVersion()   | uint16_t | Ruft den aktuellen Versionswert ab.          |
| loadPhysSector | uint32_t | Diagnose: Laedt physischen Sektor in Cache.  |
+----------------+----------+----------------------------------------------+
|HINWEIS: config() ist eine Ausnahme; sie setzt das Fehlerstatusbyte nicht.|
+----------------+----------+----------------------------------------------+


+--------------------------------------------------------------------------+
|6. FEHLERDIAGNOSE (Status-Codes)                                          |
+--------------------------------------------------------------------------+
|                                                                          |
|Der aktuelle Fehlerstatus (1 Byte) der letzten Lese-/Schreiboperation     |
|(TRUE/FALSE) muss abgefragt werden.                                       |
|                                                                          |
|**Empfohlen:** Die **direkte RAM-Handle-Abfrage** (`PartitionsData[14]`)  |
|ist die **zuverlässigste Methode** und sollte immer verwendet werden, da  |
|sie auch bei kritischen Metadaten-Fehlern (Code 0x05) funktioniert.       |
|                                                                          |
+----------+---------------------------------------------------------------+
| Hex-Code | Bedeutung                                                     |
+----------+---------------------------------------------------------------+
| 0x00     | Alles OK. Partition ist gültig und betriebsbereit.            |
| 0x01     | CRC-Prüfsumme des Sektors war ungültig (Daten korrumpiert).   |
| 0x02     | Schreibversuch abgelehnt: Datentyp > PayloadSize.             |
| 0x03     | Schreibversuch abgelehnt: Max. logischer Zähler erreicht.     |
| 0x04     | Initialisierungsfehler: Magic ID/Version fehlt.               |
| 0x05     | SCHWERER FEHLER: Kontrolldaten der PARTITION zerstört.        |
+----------+---------------------------------------------------------------+


+--------------------------------------------------------------------------+
|4. QUICKSTART / ANWENDUNGSBEISPIEL (Arduino-INO-Syntax)                   |
+--------------------------------------------------------------------------+

#include "EEProm_Safe_Wear_Level.h"

// Definiere die Partitionen (Anzahl frei wählbar):
#define PARTITIONS 1

// Bei der Zahl der Partionen behalten Sie bitte den RAM im Auge! Jede Partition
// benötigt Verwaltungsdaten von 16 Byte. 

// Die Bibliothek erfordert ein RAM-Handle fuer die Metadaten-Verwaltung, in zwei
// Schritten.

// Erster Schritt:

typedef struct {
	uint8_t data[16 * PARTITIONS];
} __attribute__((aligned(8))) administrative_control_structure;

// Sie definieren hier eine neue Struktur (struct) ohne Namen (anonym).
// Der typedef weist diesem Strukturtyp den Aliasnamen
// 'administrative_control_structure' zu.
//
// Das __attribute__((aligned(8))) wendet die Ausrichtungsregel auf den Typ
// 'administrative_control_structure' an.
// Alignment (Ausrichtung) beschreibt die Regel, nach der Daten im
// Computerspeicher abgelegt werden müssen, 
// damit der Prozessor (CPU) sie effizient verarbeiten kann. Im Wesentlichen
// müssen dazu Daten eines bestimmten Typs (z. B. ein 32-Bit-Integer) an
// einer Speicheradresse beginnen, die ein ganzzahliges Vielfaches der
// Größe dieses Datentyps ist.
//
// Warum ist Alignment wichtig? 
// Geschwindigkeit (Performance): Die CPU kann perfekt ausgerichtete Daten 
// in einem einzigen Speichervorgang lesen. Wenn Daten nicht ausgerichtet sind,
// muss die CPU zwei separate Leseoperationen durchführen und die Teile dann
// mühsam zusammenfügen. Dies verlangsamt die Verarbeitung drastisch.
//
// Alignment bei Strukturen (structs)
// Alignment wird besonders relevant, wenn man Daten in Strukturen organisiert.
// Der Compiler muss sicherstellen, dass jedes Element im struct korrekt
// ausgerichtet ist.
// Im Fall von EEProm_Safe_Wear_Level ist eine Ausrichtung nur für den ANfang
// der Daten im Speicher nötig.
// Mehrere Verwaltungsdaten für mehrere Partitionen sind danach automatisch
// ausgerichtet, da sie ein Vielfaches von 16 Byte sind.

// Zweiter Schritt (Die Variable):

administrative_control_structure PARTITIONS_DATA;

// Sie deklarieren eine Variable vom Typ 'administrative_control_structure'.
// Der Name dieser Variablen ist PARTITIONS_DATA.


// Instanziierung: Verknüpft die RAM-Handle-Verwaltungsdaten mit dem Code der
// EEProm_Safe_Wear_Level Bibliothek.
// Gleichzeitig wird eine Instanz erstellt, die danach mit EEPRWL_Main
// angesprochen werden kann.
// WICHTIG: Es wird der Zeiger auf das Daten-Array im Struct übergeben
// (PARTITIONS_DATA.data).
EEProm_Safe_Wear_Level EEPRWL_Main(PARTITIONS_DATA.data); 

// Die Bibliothek weiß ab jetzt immer, an welcher Adresse im Speicher die
// Verwaltungsdaten zu finden sind.

// Alle Aufrufe der API-Schnittstelle von EEProm_Safe_Wear_Level
// müssen in Setup() oder Loop() stattfinden.


// Die Laenge der Nutzdaten in Byte:

#define PAYLOAD_SIZE 12
char MY_DATA[PAYLOAD_SIZE];

// 12 Byte für einen kurzen String im Beispiel.  

// Separater Zähler für den Loop:
int loopCounter = 0; 


void setup() {

	// Serielle Schnittstelle initialisieren zur Ausgabe  
	Serial.begin(115200);
	delay(1000);
	Serial.println(F("EEPROM Quickstart gestartet."));

	// Definiert die Startadresse im EEPROM:

	#define ADDR_START      0x0000

	// theoretisch ist eine Adresse zwischen 0 und der maximalen Größe
	// des EEPROM möglich. Waehlen Sie sie sorgfältig!

	// Sie benötigen die Länge der Daten einer Partition.
	// Wir haben zu Anfang 1 Partition gewählt.  

	#define PART_LENGTH     144

	// Die Partition wird im Beispiel diese Länge haben.
	// Die Groesse der Partition (hier 144 Byte) bestimmt, wieviele 
	// Datensaetze (Sektoren) darin Platz finden.
	// 10 Datensätze zu je 12 Byte Nutzdaten und 2 Byte Metadaten
	// (1 Byte logischer Zähler und 1 Byte Prüfsumme). Zudem hat jede
	// Partition am Anfang 4 Byte zur Erkennung und für einen zusätzlichen
	// Zähler (2 Byte = 16 Bit). Dieser Zähler wird mit setVersion()
	// festgelegt.      

	// Die Counter-Groesse in Byte legt fest, wie gross der Logische Zaehler
	// für Sektoren werden kann:

	#define COUNTER_LEN  1

	// Fuer dass Beispiel ist nur ein Byte notwendig. Damit können 256 Sektoren
	// geschrieben werden, bevor die Partition zurückgesetzt werden muss.  
	// Im Idealfall soll dieser logische Zähler die Lebensdauer des EEPROM
	// uebertreffen, damit kein Zurücksetzen der Partition im Betrieb notwendig
	// wird.
	// Ein so großer Zähler ist aber mit einem großen Overhead (Mehrdaten)
	// verbunden, als zur Speicherung der Daten selbst notwendig sind.
	// Sie treffen hier eine Abwägung zwischen diesem Overhead und der
	// Praxistauglichkeit für Ihren INO-Code.

	
	// Definition von Handles zum Zugriff auf jede einzelne Partition
	
	#define HANDLE1  0

	// 1. Konfiguration der Partition #1 (HANDLE1)
	// Die config()-Methode speichert KEINEN Fehlerstatus im RAM-Handle, aber
	// sie gibt einen Erfolgsstatus zurück: TRUE oder FALSE (1 oder 0)

	int STATUS = EEPRWL_Main.config(ADDR_START, PART_LENGTH, PAYLOAD_SIZE,
                                  COUNTER_LEN, HANDLE1);

	// Die config()-Methode berechnet verschiedene Werte und richtet im
	// Bedarfsfall eine Partition im EEPROM ein. Die berechneten Werte werden 
	// in der Verwaltungsdatenstruktur hinterlegt. Dies spart bei folgenden 
	// Funktionsaufrufen Zeit und Speicherplatz für redundante Code-Aufrufe. 

	// Korrigierter if/else-Block:
	if(STATUS > 0){ // Status > 0 bedeutet, dass die Versionsnummer gesetzt ist
		Serial.println(F("Konfiguration erfolgreich."));
		Serial.print(F("Aktueller Partitions-Override-Counter: "));
		Serial.println(STATUS);
	} else { // 0 bedeutet Fehler oder leere Partition
		Serial.println(F("Konfiguration nicht erfolgreich!"));
	}


	// 2. Zustand wiederherstellen/neu formatieren
	
	EEPRWL_Main.setVersion(1, HANDLE1);

	// Die Handle ID (Partitionsnummer), auf die sich der Vorgang bezieht.
	// Sie muss mit der ID übereinstimmen, die zuvor in der Funktion config() 
	// für diese Partition verwendet wurde (z. B. HANDLE1).
	// Die Versionsnummer, die in die Partition geschrieben wird, muss angegeben
	// werden. Durch Ändern dieser Nummer wird die Partition neu formatiert und 
	// alle alten Daten überschrieben, sowie logisch für ungültig erklärt.
	// setVersion() sucht auch den neuesten gültigen Datensatz / Sektor in
	// einer Partition. Der kann anschließend gelesen werden.
	//
	// Funktion der Versionsnummer bei Formatierung:
	// Die Versionsnummer (die Sie über setVersion() setzen) ist der
	// Master-Zähler der Partition. Sie steuert, wann eine Partition logisch
	// als "leer" oder "neu" betrachtet wird.
	//
	// Reaktivierung und Reset:
	// Wenn eine Partition in den Zustand der Logischen Sättigung (Fehler 0x03)
	// gerät, kann sie keine Daten mehr schreiben, obwohl sie physisch noch
	// intakt ist. Um sie zu reaktivieren, müssen Sie setVersion() aufrufen
	// und die Versionsnummer erhöhen (z. B. von 1 auf 2).
	// Sie erfahren die aktuelle Nummer als Rückgabe von config(), wenn die
	// Nummer > 0 ist, oder mit der Funktion: getVersion(HANDLE1), der sie
	// das Handle für die Partitionsnummer mitgeben müssen.
	// Dieser Vorgang ist gleichbedeutend mit einer logischen Formatierung:
	// Die Bibliothek überschreibt alle alten Daten und der interne
	// Wear-Leveling-Zähler beginnt von vorne.
	//
	// Verhinderung von Redundanz:
	// Der interne, automatisch hochgezählte Logische Zähler dient dem
	// Wear-Leveling und wird bei jedem Write erhöht. Die Versionsnummer
	// hingegen ist ein separater 16-Bit-Zaehler, der nur dann erhöht wird,
	// wenn die Partition logisch gesättigt ist oder Sie manuell alle alten
	// Daten verwerfen möchten (z. B. bei einem Firmware-Update).

	// 3. Letzten gueltigen Wert lesen
	// Rueckgabe TRUE (1) bei Erfolg.
	
	if (EEPRWL_Main.read(MY_DATA, HANDLE1, PAYLOAD_SIZE)) {
		Serial.print(F("Gueltige Daten gelesen: "));
		Serial.println(MY_DATA);
	}else Serial.println(F("Noch keine Daten vorhanden."));

	// read() dient dazu, den zuletzt gültigen Datensatz aus einer spezifischen
	// Partition im EEPROM auszulesen und in eine dafür vorgesehene
	// Speicherstruktur im RAM zu kopieren.
	// Die Funktion kann verschiedene Datentypen lesen. Sie gibt einen booleschen
	// Wert zurück, der signalisiert, ob der Lesevorgang erfolgreich war.
	//
	// Argumente:
	//
	// MY_DATA: Dies ist die Zielstruktur oder der Puffer im RAM (der
	// Hauptspeicher Ihres Mikrocontrollers). Die gelesenen Bytes aus dem
	// EEPROM werden direkt in diesen Speicherbereich geschrieben. 
	// Es muss sichergestellt sein, dass dieser Puffer groß genug ist, um die
	// Payload aufzunehmen.
	//
	// HANDLE1: Identifiziert die Partition, aus der gelesen werden soll.
	// Rolle: Dies ist der Handle-ID-Wert (z.B. 0, 1, 2...), der die spezifische
	// Kontrolldatenstruktur (RAM-Handle) auswählt, die zu dieser Partition
	// gehört. Dies ist entscheidend für Multi-Partition-Unterstuetzung.
	//
	// PAYLOAD_SIZE: Gibt an, wie viele Bytes maximal aus dem EEPROM gelesen und
	// in den Zielpuffer übertragen werden sollen (Funktion 2 und 1).
	// Rolle: Dies dient als explizite Obergrenze und ist der primäre
	// Sicherheitsmechanismus, um RAM-Pufferüberläufe (Buffer Overflows) im
	// Ziel-Array zu verhindern.
	// Stellen Sie sicher, dass der Puffer groß genug ist, um die angegebene
	// PAYLOAD_SIZE aufzunehmen!
	// Diese Funktion: read(MY_DATA, PAYLOAD_SIZE, HANDLE1) kann keine
	// String-Objekte (std::string) verarbeiten!
	// Sie ist optimiert für das sichere Management von **structs** (Plain Old
	// Data) und **C-Arrays** (char*), da diese die beste Grundlage bieten, um
	// die Datenmenge zur Entwurfszeit zu planen und im Code fest zu
	// verankern. 	
}

void loop() {
	// Daten ändern: Erstellt einen String im Puffer (z.B. "MSG_001")
	loopCounter++;
	snprintf(MY_DATA, PAYLOAD_SIZE, "MSG_%03d", loopCounter);

	// --- 4. Daten schreiben (Char* Array) ---
	// Hier wird Funktion (char* mit maxSize) verwendet.
	// Die korrekte Signatur ist: (Datenpuffer, Größe, Handle-ID)
	uint16_t sector = EEPRWL_Main.write(MY_DATA, PAYLOAD_SIZE, HANDLE1);

	if (sector == 0) {
		Serial.print(F("SCHREIB-FEHLER. Code: 0x"));
		// Direkte Abfrage des Status-Codes über RAM-Handle
		Serial.println(PARTITIONS_DATA.data[14], HEX); 

	} else {
		Serial.print(F("Daten in Sektor "));
		Serial.print(sector);
		Serial.print(F(" geschrieben. String: "));
		Serial.println(MY_DATA);
	}

	delay(5000);
}



-------------------------------------------------------------------------------
## License

This library is released under the terms of the **GNU Lesser General Public
License v2.1 (LGPL-2.1)**.
-------------------------------------------------------------------------------
END README
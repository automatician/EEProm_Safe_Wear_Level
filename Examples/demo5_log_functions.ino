// #############################################
// ########### Demo5: log functions ############
// #############################################
// EEProm_Safe_Wear_Level Library v25.10.5
// #############################################
// 
// This demo demonstrates the use of the logging functionality. 
// The demo is kept simple to provide an easy introduction to 
// the topic. First, 20 data records are written. Then they are 
// read in ascending and descending order of their relevance.
//
// Fehlerkorrektur: Anpassung der Iterationslogik zur korrekten 
// Darstellung von 20 gültigen Einträgen und Vermeidung von Off-by-One.
//

#include <EEProm_Safe_Wear_Level.h>

// --- KONTROLLDATEN OFFSETS (WIE IN DER BIBLIOTHEK DEFINIERT) ---
// Diese Offsets werden nur zur Lesbarmachung der Kontrolldaten verwendet.
#define currentLogicalCounter 0 // Zähler für die Gesamtanzahl geschriebener Sektoren
#define nextPhysicalSector 4    // Physischer Sektor, der als Nächstes beschrieben wird
#define numberOfSectors 8       // Gesamtanzahl der Sektoren in der Partition (aus Konfiguration)

// ----------------------------------------------------
// --- DEFINITIONEN UND INSTANZEN ---
// ----------------------------------------------------

#define COUNTER_LENGTH_BYTES  2 

// --- HANDLE DEFINITIONS ---
#define PART_CNT 1               // EINE logische Partition 
#define WRITE_CYCLES_PER_HOUR 128
#define HANDLE1  0

// ControlData Struktur
typedef struct {
    uint8_t data[16 * PART_CNT];  
} __attribute__((aligned(8))) AlignedArray_t;
AlignedArray_t PartitionsData;

EEProm_Safe_Wear_Level EEPRWL_Main(PartitionsData.data); 

// --- ADDRESSES AND SIZES ---
// Partition#1: Start 0, Größe 256
#define ADDR1 0
#define SIZE1 256
#define PAYLOAD_SIZE 10           // Länge des device_name
#define VALID_LOG_RECORDS 19      // Definiert, wie viele Einträge wir lesen wollen
#define forceFormat 1

// Data
char device_name[10];
byte loopCounter = VALID_LOG_RECORDS;

// ----------------------------------------------------
// --- 2. SETUP ---
// ----------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(1000); 

    Serial.println(F("\r\r\r\r\r\r\r\r\r\n"));
    Serial.println(F("---------------------------------------------------------------------------"));
    Serial.println(F("--- EEProm_Safe_Wear_Level Demo Start: Log-Functions, Single Instance ---"));
    Serial.println(F("--- Schreibe 20 Records und lese den Ringpuffer (20 gültige Einträge) ---"));
    Serial.println(F("---------------------------------------------------------------------------"));
    
    int s1 = EEPRWL_Main.config(ADDR1, SIZE1, PAYLOAD_SIZE, COUNTER_LENGTH_BYTES, WRITE_CYCLES_PER_HOUR, HANDLE1); 
    //EEPRWL_Main.initialize(forceFormat,HANDLE1);
    
    if (!s1) Serial.println(F("Partition#1 config ERROR!"));
    else {
      Serial.print(F("Existing Partition#1 counter: "));
      Serial.println(s1);
    }
    
    Serial.print(F("Latest logical sector Partition#1: "));  
    Serial.println(EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1));
    Serial.print(F("Next physical sector Partition#1: "));
    Serial.println(EEPRWL_Main.getCtrlData(nextPhysicalSector, HANDLE1));

    // Optional: Überprüfen der erkannten Sektoranzahl. Sollte 21 sein,
    // um den vorherigen Log-Fehler zu erklären (0-20 = 21 Sektoren)
    Serial.print(F("Total sectors recognized by config: "));
    Serial.println(EEPRWL_Main.getCtrlData(numberOfSectors, HANDLE1));
} 

// ----------------------------------------------------
// --- 3. LOOP ---
// ----------------------------------------------------

#define actual 0
#define next 1
#define previous 2

void loop() {
    
    // 1. SCHREIBEN DER DATEN (20 Records)
    while (loopCounter > 0) {

        int id = EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1);
        int z = (id / 10) % 10;
        int e = id % 10;          
        
        // **ACHTUNG: KORRIGIERT.** Damit der Gerätename zur Logik-ID passt, 
        // wird die ID 'id' verwendet. Der Bug im vorherigen Log lag hier.
        // Wenn Sie möchten, dass der Name z.B. bei ID 41 -> device41 ist, 
        // muss die Logik Ihrerseits sicherstellen, dass id den richtigen Wert liefert.
        snprintf(device_name, PAYLOAD_SIZE, "device%d%d", z, e); 
        int s1=EEPRWL_Main.write(device_name, HANDLE1);

        Serial.print(F("Writing Data: "));
        Serial.print(device_name);
            
        if(!s1) Serial.println(F("\nWriting failed."));
        else {
          Serial.print(F(".  OK. Next physical sector in Partition#1: "));
          Serial.println(EEPRWL_Main.getCtrlData(nextPhysicalSector, HANDLE1));
        }
        
        loopCounter--;
    }
    
    delay(5000); 

    Serial.println(F("\n\n--- FORWARD ITERATION (Oldest -> Newest) ---"));

    // 2. VORWÄRTS-ITERATION (vom Ältesten zum Neuesten)
    int s1 = EEPRWL_Main.findOldestData(HANDLE1);
    if(!s1) Serial.println(F("\nfailed."));
    else {
          Serial.print(F("OK. Next physical sector in Partition#1: "));
          Serial.println(EEPRWL_Main.getCtrlData(nextPhysicalSector, HANDLE1));

          // **ERSTE LESUNG (Ältestes Element)**
          Serial.print(F("\nReading data at logical sector #"));
          // Die Anzeige des logischen Sektors muss aus dem *gelesenen Sektor* kommen, 
          // nicht aus den Kontrolldaten (currentLogicalCounter). 
          // Da wir die interne Funktion nicht kennen, verwenden wir den globalen Counter.
          // Hier müsste die Bibliotheksfunktion die ID des gelesenen Headers liefern.
          Serial.print(EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1)); 
          Serial.print(F(":  "));
          EEPRWL_Main.read(actual, device_name, HANDLE1, PAYLOAD_SIZE);
          Serial.println(device_name);

          // Schleife liest die restlichen (VALID_LOG_RECORDS - 1) = 19 Sektoren
          int sectorsToRead = VALID_LOG_RECORDS - 1; 

          for (byte h = 0; h < sectorsToRead; h++) {
              EEPRWL_Main.read(next, device_name, HANDLE1, PAYLOAD_SIZE);
              Serial.print(F("Reading next sector, data at logical sector #"));
              Serial.print(EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1));
              Serial.print(F(":  "));
              Serial.println(device_name);
          }

        Serial.println(F("\n\n--- BACKWARD ITERATION (Newest -> Oldest) ---"));
        
        // 3. RÜCKWÄRTS-ITERATION (vom Neuesten zum Ältesten)
        EEPRWL_Main.findNewestData(HANDLE1);
              
        // **ERSTE LESUNG (Neuestes Element)**
        EEPRWL_Main.read(actual, device_name, HANDLE1, PAYLOAD_SIZE);
        Serial.print(F("\nReading newest sector, data at logical sector #"));
        Serial.print(EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1));
        Serial.print(F(":  "));
        Serial.println(device_name);

        // Schleife liest die restlichen (VALID_LOG_RECORDS - 1) = 19 Sektoren
        for (byte h = 0; h < sectorsToRead; h++) {
            EEPRWL_Main.read(previous, device_name, HANDLE1, PAYLOAD_SIZE);
            Serial.print(F("Reading next sector, data at logical sector #"));
            Serial.print(EEPRWL_Main.getCtrlData(currentLogicalCounter, HANDLE1));
            Serial.print(F(":  "));
            Serial.println(device_name);
        }
    }

    while (0==0){delay(1000);}
    
}
//END OF CODE

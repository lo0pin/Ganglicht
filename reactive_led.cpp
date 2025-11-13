#include <Arduino.h>
#include "reactive_led.h"

/*
  ===========================================
  PROJEKT: Reaktive LED-Beleuchtungsleiste
  ===========================================
  ... (Kommentarblock unverändert)
*/

/*
  ---------------------------------------------------------------------------
  ÜBERSICHT
  ---------------------------------------------------------------------------
  - Zwei PIR-Bewegungsmelder (links/rechts) starten – abhängig vom ersten Signal –
    eine sequentielle Einschalt-Animation der LED-Leiste von links bzw. rechts.
  - Nach dem vollständigen Einschalten bleibt die Leiste an (Zustand 3) und zeigt
    ein „Kerzenflackern“ mittels PWM. Jeder LED-Kanal hat einen eigenen Flicker-Index,
    damit die Kanäle nicht synchron flackern.
  - Solange ein Sensor erneut auslöst, wird die „An“-Dauer verlängert (Timer-Reset).
  - Wenn über eine definierte Zeit (onTimeLight) kein Sensor mehr triggert, dimmt
    die Leiste langsam herunter (Zustand 4) und geht anschließend aus (Zustand 0).
  - Ein LDR an A0 verhindert das Einschalten bei (zu) hoher Umgebungshelligkeit
    anhand des Schwellwerts `lumidity` (Vergleich erfolgt in Zustand 0).
*/

/* ----------------------------- Hardware-Pins ----------------------------- */
int LedPins[] = { 3, 5, 6, 9, 10, 11 };          // PWM-fähige Pins, an denen die LEDs hängen
#define ledPinLength sizeof(LedPins) / sizeof(LedPins[0]) // Anzahl der LED-Kanäle
#define SensorPinLeft  2                           // PIR links (digital)
#define SensorPinRight 4                           // PIR rechts (digital)

/* ------------------------------ Parameter -------------------------------- */
#define delaylight   500   // (aktuell ungenutzt) Delay zwischen zwei LEDs beim Einschalten
#define onTimeLight  10000 // Zeit (ms), die die LEDs an bleiben sollen, ohne neue Bewegung
#define dimSpeedHi   3     // Schritt-Delay beim Hochdimmen (klein = schneller)
#define dimSpeedLo   5     // Schritt-Delay beim Runterdimmen (klein = schneller)
#define lumidity     200   // LDR-Schwellwert: erst ab diesem A0-Wert wird eingeschaltet

/* ------------------------- Kerzenflackern-PWM ---------------------------- */
/*
  Die Liste enthält Helligkeitswerte (0..255), die ein lebendiges „Kerzenflackern“
  erzeugen. In Zustand 3 (LEDs an) wird für jeden LED-Kanal zyklisch ein eigener
  Index durch diese Liste geschoben, damit die Kanäle phasenverschoben flackern.
*/
const uint8_t candleVals[] = {
  236, 229, 241, 252, 235, 224, 212, 228,
  246, 240, 245, 238, 230, 214, 195, 222,
  239, 253, 244, 236, 228, 216, 224, 237,
  246, 249, 238, 229, 220, 208, 190, 210,
  225, 239, 251, 243, 234, 222, 231, 247,
  255, 244, 235, 226, 214, 205, 196, 208,
  219, 233, 246, 239, 230, 219, 212, 200,
  210, 224, 236, 248, 241, 232, 223, 212
};
const uint16_t FLICKER_DELAY_MS = 35;   // Update-Intervall des Flackerns
unsigned long lastFlicker = 0;          // Zeitstempel des letzten Flacker-Updates
size_t flickerIndexPerLed[ledPinLength]; // Pro LED eigener Index in candleVals

/* -------------------------- Zustandssteuerung ---------------------------- */
/*
  led_state:
    0 = aus
    1 = von links starten (Fade-In links -> rechts)
    2 = von rechts starten (Fade-In rechts -> links)
    3 = ein (Kerzenflackern aktiv, Timer-Verlängerung bei Bewegung)
    4 = abschalten (langsames Fade-Out aller Kanäle)
*/
int led_state = 0;
unsigned long timer = 0; // Merkt sich den Zeitpunkt, ab dem die „An“-Zeit läuft/verlängert wird

/* ---------------------- (Unveränderte Rest-Variablen) -------------------- */
/*  Diese Variablen stammen aus der ursprünglichen Fassung und bleiben erhalten,
    auch wenn sie in dieser Version nicht überall benötigt werden. */
int led_lumidity = 0;
int actual_led_index = 0;
volatile int sensorbool = 0x0;

int lightValue = 0;
unsigned long lastUpdate = 0;
const int fadeInterval = dimSpeedHi;
int off_lumidity = 255;
int off_led_index = 0;
unsigned long lastOffUpdate = 0;
const int fadeOffInterval = dimSpeedLo;
int dim_direction = 0;

/* ---------------------------- Funktionsköpfe ----------------------------- */
void setleft();
void setright();
void shutoff();
void keepon();
int  convert_led(int led, int sensorbool);

void turnoff();
void turnonleft();
void turnonright();

/* ------------------------------------------------------------------------- */
/*                Öffentliche Wrapper-Funktionen für main.ino                */
/* ------------------------------------------------------------------------- */

void reactiveLedSetup() {
  // LED-Pins als Ausgang konfigurieren
  for (int i = 0; i < ledPinLength; i++) {
    pinMode(LedPins[i], OUTPUT);
  }

  // PIR-Sensor-Pins als Eingang
  pinMode(SensorPinLeft, INPUT);
  pinMode(SensorPinRight, INPUT);

  // LDR (analoger Eingang)
  pinMode(A0, INPUT);

  // Initiale Flicker-Phasen je LED verschieden setzen (deterministisch, nicht „echt“ random)
  const size_t N = sizeof(candleVals) / sizeof(candleVals[0]);
  for (int i = 0; i < ledPinLength; i++) {
    flickerIndexPerLed[i] = (i * 7) % N; // Offset pro LED -> unsynchrones Flackern
  }
}

void reactiveLedLoop() {
  /*
    Zustandslegende:
      0 = aus
      1 = von links starten
      2 = von rechts starten
      3 = ein
      4 = abschalten
  */

  // Aktuelle Helligkeit vom LDR einlesen
  lightValue = analogRead(A0);

  // ------------------------------ Zustand 0 ------------------------------ //
  // Aus: auf Bewegungen warten, LDR-Schwelle muss erreicht sein
  if (led_state == 0) {
    if (digitalRead(SensorPinLeft) == 1 && lightValue >= lumidity) {
      led_state = 1;               // Start von links
    } else if (digitalRead(SensorPinRight) == 1 && lightValue >= lumidity) {
      led_state = 2;               // Start von rechts
    } else {
      shutoff();                   // Sicherheits-halber alle Kanäle aus
    }
  }

  // ------------------------------ Zustand 1 ------------------------------ //
  // Einschalt-Animation von links nach rechts (blockierend, sequentiell)
  else if (led_state == 1) {
    turnonleft();                  // Setzt am Ende led_state = 3 und timer = millis()
  }

  // ------------------------------ Zustand 2 ------------------------------ //
  // Einschalt-Animation von rechts nach links (blockierend, sequentiell)
  else if (led_state == 2) {
    turnonright();                 // Setzt am Ende led_state = 3 und timer = millis()
  }

  // ------------------------------ Zustand 3 ------------------------------ //
  // Ein: Kerzenflackern aktiv, Ein-Zeit wird bei jeder Bewegung verlängert
  else if (led_state == 3) {

    // Kerzenflackern in regelmäßigen Intervallen updaten (nicht-blockierend)
    if (millis() - lastFlicker >= FLICKER_DELAY_MS) {
      lastFlicker = millis();
      const size_t N = sizeof(candleVals) / sizeof(candleVals[0]);

      // Jede LED erhält ihren eigenen Helligkeitswert (unsynchrones Flackern)
      for (int i = 0; i < ledPinLength; i++) {
        uint8_t b = candleVals[flickerIndexPerLed[i]];
        analogWrite(LedPins[i], b);

        // Index für die nächste Runde vorziehen (mit Wrap-around)
        flickerIndexPerLed[i]++;
        if (flickerIndexPerLed[i] >= N) flickerIndexPerLed[i] = 0;
      }
    }

    // Ein-Dauer verlängern, sobald irgendein PIR erneut triggert
    if (digitalRead(SensorPinLeft) == 1 || digitalRead(SensorPinRight) == 1) {
      timer = millis();            // Reset der „An“-Zeit
    }
    // Wenn seit „timer“ länger als onTimeLight vergangen ist -> runterdimmen
    else if (millis() - timer >= onTimeLight) {
      led_state = 4;
    }
  }

  // ------------------------------ Zustand 4 ------------------------------ //
  // Abschalten: langsames, gemeinsames Herunterdimmen aller Kanäle (blockierend)
  else if (led_state == 4) {
    turnoff();                     // Setzt am Ende led_state = 0
  }
}

/* --------------------------- Hilfsfunktionen ----------------------------- */

/*
  Setzt alle LED-Kanäle hart auf LOW.
  Wird in Zustand 0 aufgerufen, wenn keine Startbedingung erfüllt ist.
*/
void shutoff() {
  for (int i = 0; i < ledPinLength; i++) {
    digitalWrite(LedPins[i], LOW);
  }
  led_state = 0;
}

/*
  Sequentielles Hochdimmen von links nach rechts.
  Jeder Kanal wird von 0..255 hochgefahren (blockierend via delay).
  Am Ende: Zustand 3 (ein) und Timer-Start für die „An“-Zeit.
*/
void turnonleft() {
  for (int i = 0; i < ledPinLength; i++) {
    for (int j = 0; j <= 255; j++) {
      analogWrite(LedPins[i], j);
      delay(dimSpeedHi);
    }
  }
  led_state = 3;
  timer = millis();
}

/*
  Sequentielles Hochdimmen von rechts nach links.
  Jeder Kanal wird von 0..255 hochgefahren (blockierend via delay).
  Am Ende: Zustand 3 (ein) und Timer-Start für die „An“-Zeit.
*/
void turnonright() {
  for (int i = 0; i < ledPinLength; i++) {
    for (int j = 0; j <= 255; j++) {
      analogWrite(LedPins[ledPinLength - i - 1], j);
      delay(dimSpeedHi);
    }
  }
  led_state = 3;
  timer = millis();
}

/*
  (In dieser Version nicht genutzt.)
  Würde alle Kanäle hart einschalten.
*/
void keepon() {
  for (int i = 0; i < ledPinLength; i++) {
    digitalWrite(LedPins[i], HIGH);
  }
}

/*
  Langsames, gemeinsames Herunterdimmen aller Kanäle von 255..0.
  Blockierende Schleife (delay nach jedem Schritt).
  Am Ende: Zustand 0 (aus).
*/
void turnoff() {
  for (int j = 255; j >= 0; --j) {
    for (int i = 0; i < ledPinLength; i++) {
      analogWrite(LedPins[i], j);
    }
    delay(dimSpeedLo);
  }
  led_state = 0;
}

/* -------------------- Platzhalter/Prototypen (ungenutzt) ----------------- */

void setleft() {
  // Platzhalter – aktuell nicht verwendet
}

void setright() {
  // Platzhalter – aktuell nicht verwendet
}

int convert_led(int led, int sensorbool) {
  // Platzhalter – aktuell nicht verwendet
  return led;
}

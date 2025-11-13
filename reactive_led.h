#ifndef REACTIVE_LED_H
#define REACTIVE_LED_H

#include <Arduino.h>

// Initialisierung der LED-Leiste, Sensoren, Flicker-Indizes etc.
void reactiveLedSetup();

// Zustandsmaschine und Effekte (muss in loop() zyklisch aufgerufen werden)
void reactiveLedLoop();

#endif // REACTIVE_LED_H

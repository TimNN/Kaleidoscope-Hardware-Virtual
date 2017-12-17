#include "Arduino.h"
#include "virtual_io.h"

// Estimate that a scan cycle takes this many ms on real hardware
#define MILLIS_PER_CYCLE 5

// This time emulation has the effect that millis() only updates once per scan cycle
//   which should be sufficient for most users (?)
unsigned long millis(void) {
  return currentCycle() * MILLIS_PER_CYCLE;
}
unsigned long micros(void) {
  return millis()*1000;
}


// for virtual hardware, there's not much point in respecting delay()
void delay(unsigned long ms) {
  /* An implementation that doesn't work with the current millis()
  unsigned long end = millis() + ms;
  while(millis() < end);
  */
  return;
}

void delayMicroseconds(unsigned int us) {
  /* An implementation that doesn't work with the current micros()
  unsigned long end = micros() + us;
  while(micros() < end);
  */
  return;
}

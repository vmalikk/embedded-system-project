

#include "msp430.h"
#include "macros.h"
#include "led.h"
#include "ports.h"

void Init_LEDs(void){     // Turn on both LEDs

  P1OUT &= ~RED_LED;
  P6OUT &= ~GRN_LED;

}



//------------------------------------------------------------------------------

// Header Files
#include  "msp430.h"
#include  "functions.h"
#include  "LCD.h"
#include  "ports.h"
#include  "macros.h"
#include  "timersB0.h"
#include  "ADC.h"
#include  "buttons.h"
#include  "switch.h"



// External Globals
extern volatile unsigned char update_display;

volatile unsigned int Time_Sequence;
extern unsigned int count_debounce_SW1;
extern unsigned int count_debounce_SW2;

// Timer display globals (Project 7)
// ADC results used by main/state machine
extern volatile unsigned int ADCLeft;
extern volatile unsigned int ADCRight;
extern volatile unsigned int ADCThumb;

static volatile unsigned int ADCChannel = 0;


// Timers
#pragma vector = TIMER0_B0_VECTOR
__interrupt void Timer0_B0_ISR(void){
    //    ADC_Update = TRUE;
    //-----------------------------------------------------------------------------
    // TimerB0 0 Interrupt handler
    //---------------------------------------------------------------------------
    update_display = TRUE;
    Time_Sequence++;
    TB0CCR0 += TB0CCR0_INTERVAL;
}

#pragma vector=TIMER0_B1_VECTOR
__interrupt void TIMER0_B1_ISR(void){
    //---------------------------------------------------------------------------
    // TimerB0 1-2, Overflow Interrupt Vector (TBIV) handler
    //---------------------------------------------------------------------------
    switch(__even_in_range(TB0IV,14)){
    case  0: break;                    // No interrupt
    case  2:                           // CCR1 Used for SW1 Debounce
        count_debounce_SW1++;
        if (count_debounce_SW1 >= DEBOUNCE_TIME){
            count_debounce_SW1 = 0;

            TB0CCTL1 &= ~CCIE;
            P4IFG &= ~SW1;
            P4IE  |= SW1;
            buttons_post_event(BUTTON_EVENT_SW1_PRESS);
        }

        TB0CCR1 += TB0CCR1_INTERVAL;


        break;

    case  4:                           // CCR2 Used for SW2 Debounce
        count_debounce_SW2++;
        if (count_debounce_SW2 >= DEBOUNCE_TIME){
            count_debounce_SW2 = 0;

            TB0CCTL2 &= ~CCIE;
            P2IFG &= ~SW2;
            P2IE  |=  SW2;
            buttons_post_event(BUTTON_EVENT_SW2_PRESS);
        }


        TB0CCR2 += TB0CCR2_INTERVAL;

        break;

    case 14:                           // overflow available for greater than 1 second timer
        break;
    default: break;
    }
}











// #pragma vector = TIMER0_B3_VECTOR
// __interrupt void Timer0_B3_ISR(void){
//     //-----------------------------------------------------------------------------
//     // TimerB3 0 Interrupt handler
//     //---------------------------------------------------------------------------
// //    update_display = TRUE;
//     //    State_Sequence = Time_Sequence;
//     //    P6OUT ^= LCD_BACKLITE;
//     TB3CCR0 += TB3CCR0_INTERVAL;
// }


    // Switches
    //-----------------------------------------------------------------------------
    // Port 4 interrupt for switch 1, it is disabled for the duration
    // of the debounce time. Debounce time is set for 1 second
#pragma vector=PORT4_VECTOR
    __interrupt void switch1_interrupt(void) {
        // Switch 1
        if (P4IFG & SW1) {

            P4IE &= ~SW1;
            P4IFG &= ~SW1;

            TB0CCTL1 &= ~CCIFG;             // Clear SW1 debounce interrupt flag
            TB0CCR1 = TB0R + TB0CCR1_INTERVAL; // Schedule debounce tick
            TB0CCTL1 |= CCIE;               // CCR1 enable interrupt

        }
        //-----------------------------------------------------------------------------
    }
    //-----------------------------------------------------------------------------
    // Port 2 interrupt for switch 2, it is disabled for the duration
    // of the debounce time. Debounce time is set for 1 second
#pragma vector=PORT2_VECTOR
    __interrupt void switch2_interrupt(void) {
        // Switch 2
        if (P2IFG & SW2) {

            P2IE &= ~SW2;
            P2IFG &= ~SW2;

            TB0CCTL2 &= ~CCIFG;             // Clear SW2 debounce interrupt flag
            TB0CCR2 = TB0R + TB0CCR2_INTERVAL; // Schedule debounce tick
            TB0CCTL2 |= CCIE;               // CCR2 enable interrupt

        }
        //-----------------------------------------------------------------------------
    }









    // ADC Interrupt
#pragma vector=ADC_VECTOR
    __interrupt void ADC_ISR(void){
        //    backlight_status = ON;
    switch(__even_in_range(ADCIV,ADCIV_ADCIFG)){
        case ADCIV_NONE:
            break;
        case ADCIV_ADCOVIFG:   // When a conversion result is written to the ADCMEM0
            // before its previous conversion result was read.
            break;
        case ADCIV_ADCTOVIFG:   // ADC conversion-time overflow
            break;
        case ADCIV_ADCHIIFG:    // Window comparator interrupt flags
            break;
        case ADCIV_ADCLOIFG:    // Window comparator interrupt flag
            break;
        case ADCIV_ADCINIFG:    // Window comparator interrupt flag
            break;
        case ADCIV_ADCIFG:      // ADCMEM0 memory register with the conversion result
            ADCCTL0 &= ~ADCENC;                          // Disable ENC bit.
            switch (ADCChannel++){
            case 0x00:                                   // Channel A2 (Left) Interrupt
                ADCMCTL0 &= ~ADCINCH_2;                  // Disable Last channel A2
                ADCMCTL0 |=  ADCINCH_3;                  // Enable Next channel A3

                ADCLeft = ADCMEM0 >> 2;                  // Scale result
                break;

            case 0x01:                                   // Channel A3 (Right) Interrupt
                ADCMCTL0 &= ~ADCINCH_3;                  // Disable Last channel A2
                ADCMCTL0 |=  ADCINCH_5;                  // Enable Next channel A1

                ADCRight = ADCMEM0 >> 2;                 // Scale result
                break;

            case 0x02:                                   // Channel A5 (Thumb) Interrupt
                ADCMCTL0 &= ~ADCINCH_5;                  // Disable Last channel A5
                ADCMCTL0 |= ADCINCH_2;                   // Enable Next [First] channel 2

                ADCThumb = ADCMEM0 >> 2;                 // Scale result
                ADCChannel = 0;
                break;

            default:
                ADCChannel = 0;
                break;
            }
            ADCCTL0 |= ADCENC;                          // Enable Conversions
            ADCCTL0 |= ADCSC;
            break;
            default:
                break;
        }
}




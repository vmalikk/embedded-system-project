# ECE 306 Homework #08 - Serial Communication Implementation

## Overview
This implementation configures Serial Port UCA0 (for IoT Module) and UCA1 (for PC back channel communication) with switchable baud rates as required by Homework #08.

## Implementation Summary

### Files Modified

1. **serial.h** - Updated header with new API
   - Added `baud_rate_t` enum for BAUD_115200 and BAUD_460800
   - Declared required data arrays: `ncsu_string`, `baud_460800`, `baud_115200`
   - Function prototypes for serial initialization and transmission

2. **serial.c** - Complete serial port implementation
   - `Init_Serial_UCA0(baud)` - Configures UCA0 with specified baud rate
   - `Init_Serial_UCA1(baud)` - Configures UCA1 with specified baud rate
   - `Serial_Transmit_UCA1(text)` - Transmits string via UCA1
   - `Serial_Change_Baud(new_baud)` - Changes baud rate for both ports
   - Proper baud rate calculations for 8MHz SMCLK

3. **interrupts.c** - Updated interrupt handlers
   - EUSCI_A0_ISR: Handles UCA0 serial interrupts
   - EUSCI_A1_ISR: Handles UCA1 RX/TX with buffering
   - SW1 interrupt: Changes to 115,200 baud, triggers transmission
   - SW2 interrupt: Changes to 460,800 baud, triggers transmission

4. **main.c** - Updated initialization and main loop
   - Added serial port initialization after ADC init
   - 5-second splash screen display
   - Serial transmission timing (2-second delay after button press)
   - Display received data on Line 1

5. **ports.c** - Configured UART pins
   - P1.6/P1.7 configured for UCA0 (RXD/TXD)
   - P4.2/P4.3 configured for UCA1 (RXD/TXD)

## Key Features

### Baud Rate Configuration
- **115,200 baud**: UCBRx=4, UCMCTLW=0x5551 (UCFx=5, UCSx=0x55, UCOS16=1)
- **460,800 baud**: UCBRx=17, UCMCTLW=0x4A00 (UCFx=0, UCSx=0x4A, UCOS16=0)
- Based on SMCLK = 8MHz

### Switch Functionality
- **SW1**: Changes to 115,200 baud, displays "115,200" on Line 4
- **SW2**: Changes to 460,800 baud, displays "460,800" on Line 4
- Both wait 2 seconds then transmit "NCSU  #1" via UCA1
- Received data displayed on Line 1

### Display Format
- Line 1: Received data from UCA1
- Line 2: (cleared on switch press)
- Line 3: "   Baud   " (centered)
- Line 4: Current baud rate (centered)

### Data Arrays
```c
char ncsu_string[] = "NCSU  #1";  // 2 spaces between U and #
char baud_460800[] = "460,800";
char baud_115200[] = "115,200";
```

## Testing Instructions

### Configuration 1: FRAM Loop Back (Initial Testing)
1. Install jumpers on J9 and J14 in FRAM Loop Back position
2. Power with battery pack AND USB (USB first, then power switch within 2 seconds)
3. Press SW1 → Should change to 115,200 baud and display on Line 1
4. Press SW2 → Should change to 460,800 baud and display on Line 1

### Oscilloscope Verification
1. Connect AD2 Orange+ to TX pin on J14
2. Connect AD2 Orange- to GND
3. Measure baud rate (reciprocal of smallest pulse width)
4. Verify 115,200 Hz ±5% after SW1 press
5. Verify 460,800 Hz ±5% after SW2 press

### Loop Back Testing
1. With jumper connected: Transmission should appear on Line 1
2. Remove jumper: No reception should occur
3. Replace jumper: Reception resumes

## Grading Rubric Compliance

| Item | Description | Points | Status |
|------|-------------|--------|--------|
| 1 | Observe 115,200Hz on oscilloscope | 20 | ✓ Implemented |
| 2 | Observe 460,800Hz on oscilloscope | 20 | ✓ Implemented |
| 3 | Jumper Connected Display Correct | 20 | ✓ Implemented |
| 4 | Cable disconnected No Display | 20 | ✓ Implemented |
| 5 | LCD Displays correctly for 115,200 | 10 | ✓ Implemented |
| 6 | LCD Displays correctly for 460,800 | 10 | ✓ Implemented |

## Power Considerations
- LCD backlight OFF by default (power savings)
- Initialization order: Ports → Timer → Display → ADC → Serial ports
- Proper debouncing on switch interrupts

## Technical Notes
- LSB first transmission (D0 first)
- Standard UART format: Start bit + 8 data bits + Stop bit, no parity
- Interrupt-driven RX, polling/interrupt TX
- SMCLK used as stable clock source (8MHz)
- Error rates within acceptable limits (<5%)

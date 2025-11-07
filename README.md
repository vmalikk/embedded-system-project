Project 8 – Serial Port Communication
====================================

This firmware extends the Homework 8 code base to deliver the full ECE306 Project 8 serial demo on the MSP430FR2355 car platform. The application exposes two UARTs (UCA0 ↔ FRAM/J9, UCA1 ↔ PC/AD2 via J14) with baud-rate agility, LCD status prompts, and button-driven echo behaviour.

Build & Flash
-------------
- Open the project in Code Composer Studio 10+ (workspace already contains the CCS metadata) and build the `Debug` configuration, *or* run `make -f Debug/makefile` from the project root using TI’s CGT toolchain.
- Connect the LaunchPad via USB and use CCS (Project → Debug) or the `Debug/EmbeddedSystemCar.out` image with `dslite flash` to program the MSP430FR2355.
- Ensure power cycling after flashing so the splash screen and splash timers reset cleanly.

Jumper & Harness Notes
----------------------
- **J9 (IOT/FRAM header)**: populate the loopback jumpers per the lab handout to short UCA0 TXD↔RXD when validating on-board loopback. Remove the shunt to demonstrate “no receive” behaviour.
- **J14 2×2 / 2×5 headers**: connect UCA1 TXD to the AD2/PC RX input and UCA1 RXD to the AD2/PC TX output (crossed). Tie grounds between the car and AD2/PC interface.
- Maintain the HW8 back-channel jumpers so the bridge between UCA0 and UCA1 remains available when the loopback shunt is installed.

Runtime Controls & UI
---------------------
- **Display default**: after the splash (5 s) the LCD shows “Waiting” on line 1 and the active baud centred on line 3.
- **SW1 (BTN1)**: echoes the most recent 10-character command back to UCA1, updates line 1 to “Transmit” and moves the command to line 2 while transmitting.
- **SW2 (BTN2)**: toggles the baud between 115,200 and 460,800. On each change the firmware clears status lines, updates the LCD baud label, waits ≈2 s, then transmits `"NCSU  #1"` as the path test string.
- **Receive path**: when any 10-character command is received on UCA1 the LCD shows “Received” (line 1) and places the command on line 4. The command remains cached for BTN1 echo.
- **Bridge behaviour**: bytes arriving on UCA0 are forwarded to UCA1 and vice versa whenever the bridge jumpers are installed, enabling FRAM loopback tests without additional firmware changes.

Command Set
-----------
Six fixed 10-character commands are recognised and echoed:
`SPDUP_0001`, `SPDDN_0002`, `TURNL_0003`, `TURNR_0004`, `BRAKE_0005`, `IDLE__0006`.

Demo Checklist (Quick Test Plan)
--------------------------------
1. **Initial splash** – Power cycle, verify 5 s splash followed by “Waiting”/baud display at 460,800.
2. **Loopback absent** – Remove J9 shunt, transmit a command from AD2, confirm LCD stays on “Waiting” (no receive).
3. **Loopback present** – Re-install shunt, resend command and observe “Received” + command on LCD line 4.
4. **Echo path** – Press SW1; confirm LCD switches to “Transmit”, command moves to line 2, and AD2/PC receives the echoed 10-byte payload plus CR/LF.
5. **Baud switch** – Press SW2; ensure baud label toggles, 2 s pause occurs, and `NCSU  #1` arrives on AD2. Repeat to return to 460,800.
6. **Command coverage** – Alternate between 115,200 and 460,800, exercising all six commands (≥3 per baud) while confirming correct LCD state transitions and echoed responses.
7. **Bridge forwarding** – With both jumpers installed, send data on UCA0 (FRAM/IOT side) and confirm it forwards to UCA1/PC path.

Notebook-Ready Bullets
----------------------
- **State flow**: `WAITING → RECEIVED → TRANSMIT` (BTN1) with `BAUD_CHANGED` returning to `WAITING` after the 2 s guard delay.
- **UART configuration**: `uart_calculate_dividers()` sets `BRW=4, MCTLW=0x5551` for 115.2 kbps (`UCOS16` enabled) and `BRW=17, MCTLW=0x4A00` for 460.8 kbps, guaranteeing <3% accumulated error; ISRs push RX bytes into 128-byte rings and optionally mirror traffic to the peer UART when bridge mode is enabled.
- **HW8 delta**: replaced polled serial helpers with ring-buffered `uart.c`, added `buttons.c` for debounced edge detection, migrated LCD status formatting to `lcd.c`, and introduced `app.c` state machine driving the Project 8 workflow.

Known Limitations
-----------------
- Only the two specified baud rates are exposed via the UI; expanding the table requires extending `messages.c` and the baud enum.
- The splash screen timing relies on the 200 ms TimerB0 tick; significant clock changes would require adjusting the tick constants.
- AD2 testing assumes CR/LF framing; other terminators are ignored until 10 command characters arrive.

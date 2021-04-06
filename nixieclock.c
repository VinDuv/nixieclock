// 150189-71 Nixie Clock alternative firmware
// Copyright (C) Vincent Duvert
// Distributed under the terms of the MIT license.

// Set the DEBUG macro in the project settings to enable debug and disable
// the watchdog.

#include "configbits.h"

#include <xc.h>

// I/O register allocation:
// A<0..3>  O   Minutes (ones digit)
// A4       O   Unused
// A5       O   Status LED
// A<6..7>  I   Used by the external oscillator (OSC = HS)
//
// B<0..3>  O   Hours (ones digit)
// B4       O   Hours (tens digit) = 1
// B5       O   Hours (tens digit) = 2
// B6       O   Hours (tens digit) = 0
// B7       O   Used by the programmer (PGD pin)
//
// C<0..2>  O   Minutes (tens digit, 0 to 7)
// C3       I   Jumper (currently unused)
// C4       I   Switch (currently unused)
// C5       O   Separator between hours and minutes
// C6       O   RS232 TX to GPS module
// C7       I   RS232 RX from GPS module
//
// D<0..3>  O   Seconds (ones digit)
// D<4..6>  O   Seconds (tens digit, 0 to 7)
// D7       O   Separator between minutes and seconds

#define STATUS_LED LATAbits.LA5
#define SWITCH PORTCbits.RC3

// GPS sync status (indicated on the status LED)
static enum {
    STATUS_OK = 0,          // GPS is synchronized
    STATUS_NO_DATA = 1,     // No data received from GPS in the last 30 seconds
    STATUS_BAD_DATA = 2,    // CRC error in data received from GPS
    STATUS_NO_SIGNAL = 3,   // No GPS signal
} gps_status = STATUS_NO_SIGNAL;

static void setup(void);

void main(void)
{
    setup();

    while (1);
}


// High priority interrupt handler
void __interrupt(high_priority) handle_int(void)
{
    static uint8_t blink_count = 0;

    if (INTCONbits.T0IF) {
        // Timer0 interrupt

        // Handle the LED blinking
        STATUS_LED = (((blink_count & 0b11) == 0) &&
                (blink_count >> 2) < gps_status);

        if ((++blink_count) == 20) {
            blink_count = 0;
        }

        // Acknowledge the interrupt
        INTCONbits.T0IF = 0;
    }
}

// Setup function
static void setup(void)
{
    // Disable interrupts during setup
    INTCON = 0b00000000;

    // Configure the use of external oscillator
    OSCCON = 0b00000000;

    // Analog/digital pin configuration: all digital
    ADCON0 = 0b00000000;
    ADCON1 = 0b00001111;

    // Pin configuration (input/output)
    TRISA = 0b11000000;
    TRISB = 0b00000000;
    TRISC = 0b10011000;
    TRISD = 0b00000000;

    // Timer and interrupt configuration
    T0CON = 0b10000010; // Timer0 enabled, 1:8 pre-scaler used

    INTCONbits.T0IE = 1;    // Interrupt on Timer0 overflow
    INTCON2bits.TMR0IP = 1; // The Timer0 overflow interrupt is high priority
    INTCONbits.T0IF = 0;    // Acknowledge any existing Timer0 interrupt

    INTCONbits.GIEH = 1;    // Enable general interrupts
}

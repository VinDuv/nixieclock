// 150189-71 Nixie Clock alternative firmware
// Copyright (C) Vincent Duvert
// Distributed under the terms of the MIT license.

// Set the DEBUG macro in the project settings to enable debug and disable
// the watchdog.

#include <stdbool.h>
#include <stdint.h>
#include <xc.h>

#include "configbits.h"
#include "datetime.h"
#include "gps.h"
#include "settings.h"


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

// Ticks counter
#define TICKS_PER_DAY 911336
static uint16_t cur_days = 0;
static uint32_t cur_ticks = 0;
static bool tick_happened = 0;

// Displayed value (after update)
#define BLANK ((uint8_t)0xff)
static struct {
    bool left_sep : 1; // Left separator
    bool right_sep : 1; // Right separator
    uint8_t digit0 : 2; // Hours tens, 0-2, 3 = blank
    uint8_t digit1 : 4; // Hours ones, 0-9, 15 = blank
    uint8_t digit2 : 4; // Minutes tens, 0-7, never blank
    uint8_t digit3 : 4; // Minutes ones, 0-9, 15 = blank
    uint8_t digit4 : 4; // Seconds tens, 0-7, never blank
    uint8_t digit5 : 4; // Seconds ones, 0-9, 15 = blank
} disp_value;

// Track GPS processing
static bool gps_proc_required = false;

static void setup(void);
static void gps_setup(void);
static bool check_tick(void);
static void delay(uint8_t ticks);
static void update_display(void);
static void disp_cur_time(void);


void main(void)
{
    setup();

    disp_value.left_sep = 0;
    disp_value.right_sep = 0;
    disp_value.digit0 = 0;
    disp_value.digit1 = 1;
    disp_value.digit2 = 2;
    disp_value.digit3 = 3;
    disp_value.digit4 = 4;
    disp_value.digit5 = 5;

    update_display();

    gps_setup();

    for (uint8_t i = 0 ; i < 10 ; i += 1) {
        disp_value.left_sep = i & 1;
        disp_value.right_sep = i & 1;
        disp_value.digit0 = i;
        disp_value.digit1 = i;
        disp_value.digit2 = i;
        disp_value.digit3 = i;
        disp_value.digit4 = i;
        disp_value.digit5 = i;

        update_display();
        delay(10);
    }

    for (;;) {
        if (check_tick()) {
            disp_cur_time();
        }
    }
}


// High priority interrupt handler
void __interrupt(high_priority) handle_int(void)
{
    if (INTCONbits.T0IF) {
        // Timer0 interrupt

        // Blink the status LED to indicate the GPS status
        uint8_t blink_count = cur_ticks & 0x1f;
        STATUS_LED = (((blink_count & 0b11) == 0) &&
                (blink_count >> 2) < gps_status);

        // Increment tick counter
        cur_ticks += 1;
        if (cur_ticks == TICKS_PER_DAY) {
            cur_days += 1;
            cur_ticks = 0;
        }
        tick_happened = true;

        // Acknowledge the interrupt
        INTCONbits.T0IF = 0;
    }

    if (PIR1bits.RCIF) {
        // Receive interrupt
        gps_proc_required = gps_handle_serial_rx();
    }
}


// Check if a tick interrupt happened. Also updates the local time and
// perform GPS message processing if needed.
static bool check_tick(void)
{
    bool process_gps;

    // Disable interrupts in the critical section
    INTCONbits.GIEH = 0; // FIXME: Only disable Timer0 interrupt?

    process_gps = gps_proc_required;
    gps_proc_required = false;
    if (!tick_happened) {
        INTCONbits.GIEH = 1;

        if (process_gps) {
            gps_process_received();
        }

        return false;
    }

    if (process_gps) {
        gps_process_received();
    }

    // Convert ticks to days and seconds
    uint16_t local_days = cur_days;
    uint32_t local_secs = (uint32_t)((uint64_t)cur_ticks * 86400
        / TICKS_PER_DAY);

    tick_happened = false;

    INTCONbits.GIEH = 1;

    recalc_local_time(local_days, local_secs);

    return true;
}


// Low-level setup function
static void setup(void)
{
    // Low-level setup

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
    TRISC = 0b11011000; // Serial TX and RX need to be set to 1
    TRISD = 0b00000000;

    // Serial port and interrupt configuration
    RCSTA = 0b10000000; // Serial port enabled, 8-bit, RX disabled (for now)
    TXSTA = 0b00100000; // 8-bit, TX enabled, async, low speed
    BAUDCON = 0b00000000; // Default baud rate control
    SPBRGH = 0;
    SPBRG = 71; // Base frequency / (64 * (71 + 1)) = 4800 baud

    IPR1 = 0b00100000; // Serial RX is high priority
    PIE1 = 0b00100000; // Serial RX interrupt enabled

    // Timer and interrupt configuration
    T0CON = 0b10000010; // Timer0 enabled, 1:8 pre-scaler used

    INTCONbits.T0IE = 1;    // Interrupt on Timer0 overflow
    INTCON2bits.TMR0IP = 1; // The Timer0 overflow interrupt is high priority
    INTCONbits.T0IF = 0;    // Acknowledge any existing Timer0 interrupt

    INTCONbits.PEIE = 1;    // Enable peripheral interrupts
    INTCONbits.GIEH = 1;    // Enable general interrupts

    // Date/time setup
    utc_offset_secs = UTC_OFFSET_SECS;

    dst_start.month = DST_START_MONTH;
    dst_start.week = DST_START_WEEK;
    dst_start.day = DST_START_DAY;
    dst_start.hour = DST_START_HOUR;

    dst_end.month = DST_END_MONTH;
    dst_end.week = DST_END_WEEK;
    dst_end.day = DST_END_DAY;
    dst_end.hour = DST_END_HOUR;
}


// GPS setup process (interrupts should be enabled))
static void gps_setup(void)
{
    // Reset the GPS to a known state
    gps_init_reset1();

    // Wait between sent messages
    delay(2);

    gps_init_reset2();

    // Wait for received messages to stop
    delay(2);

    // Enable serial reception
    RCSTAbits.CREN = 1;

    gps_init_setup();
}


// Wait a certain number of ticks, while updating the local time.
static void delay(uint8_t ticks)
{
    while (ticks > 0) {
        if (check_tick()) {
            ticks -= 1;
        }
    }
}


// Update the display depending on the disp_value variable
static void update_display(void)
{
    // Bits to enable on port B to get the correct hour tens
    static const uint8_t hour_tens_match[4] = {
        0b01000000, // 0 displayed
        0b00010000, // 1 displayed
        0b00100000, // 2 displayed
        0b00000000, // Blank
    };

    // Port A: ----XXXX XXXX = minutes ones
    LATA = (LATA & 0b11110000) | (disp_value.digit3 & 0b00001111);

    // Port B: -021XXXX 0/1/2 hour tens (0 or 1 of them), XXXX = hours ones
    LATB = (LATB & 0b10000000) |
            (hour_tens_match[disp_value.digit0]) |
            (disp_value.digit1 & 0b00001111);

    // Port C: --S--XXX S = left separator, XXX = minutes tens
    LATC = (LATC & 0b11011000) | (disp_value.left_sep ? 0b00100000 : 0) |
            (disp_value.digit2 & 0b00000111);

    // Port D: SXXXYYYY S = right separator, XXX = sec. tens, YYYY = sec. ones
    LATD = (disp_value.right_sep ? 0b10000000 : 0) |
            ((disp_value.digit4 << 4U) & 0b01110000) |
            (disp_value.digit5 & 0b00001111);
}


// Display the current time.
static void disp_cur_time(void)
{
    disp_value.left_sep = disp_value.right_sep = local_time.second & 1;

    disp_value.digit0 = local_time.hour / 10;
    disp_value.digit1 = local_time.hour % 10;
    disp_value.digit2 = local_time.minute / 10;
    disp_value.digit3 = local_time.minute % 10;
    disp_value.digit4 = local_time.second / 10;
    disp_value.digit5 = local_time.second % 10;

    update_display();
}

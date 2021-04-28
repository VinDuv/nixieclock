// 150189-71 Nixie Clock alternative firmware
// Copyright (C) Vincent Duvert
// Distributed under the terms of the MIT license.

#include "gps.h"

#include <stdint.h>
#include <stdbool.h>
#include <xc.h>

// Uncomment to help debugging GPS errors
//#define GPS_HALT_ON_ERRORS

// Definition of extern variables
enum gps_status_val gps_status;
uint64_t gps_deciseconds;

// Message payload buffer
static char payload_buf[150];
static uint8_t payload_length;

// Message timeout detection
static uint8_t idle_ticks;

// Receive progress
static uint8_t recv_pos;
static uint16_t calc_csum;
volatile enum {
    RECEIVED_NOTHING,
    RECEIVING_START,
    RECEIVING_LENGTH1,
    RECEIVING_LENGTH2,
    RECEIVING_PAYLOAD,
    RECEIVING_CSUM1,
    RECEIVING_CSUM2,
    RECEIVING_END1,
    RECEIVING_END2,
    RECEIVE_DONE,
} recv_state;

// GPS control messages:

// Switch to binary mode
static const char* gps_init_seq_data = (
    // Initial wait
    "\xfe\xfe\xfe\xfe\xfe\xfe"
    // Switch to binary mode and wait
    "$PSRF100,0,4800,8,1,0*0F\r\n\xfe"
    // Disable all messages and wait
    "\xa0\xa2\x00\x08\xa6\x02\x00\x00\x00\x00\x00\x00\x00\xa8\xb0\xb3\xfe"
    // Enable the clock message (message 7) every 10 seconds
    "\xa0\xa2\x00\x08\xa6\x00\x07\x0a\x00\x00\x00\x00\x00\xb7\xb0\xb3\xfe\xfe"
    // Send the clock message (message 7) immediately and finish
    "\xa0\xa2\x00\x02\x90\x00\x00\x90\xb0\xb3\xff"
);


static void gps_send_init_seq(void);
static inline uint8_t gps_wait_byte(void);
static uint8_t gps_wait_msg(void);

#ifdef GPS_HALT_ON_ERRORS
#define GPS_SET_ERR(error) do { gps_status = error; for (;;) {} } while(0)
#else
#define GPS_SET_ERR(error) do { gps_status = error; } while(0)
#endif


void gps_init(void)
{
    gps_status = STATUS_UNSYNC;
    recv_state = RECEIVED_NOTHING;
    gps_deciseconds = 0;

    // Send the initialization sequence
    gps_send_init_seq();

    // Wait for message 7
    RCSTAbits.CREN = 1;
    while (gps_wait_msg() != 7);

    idle_ticks = 0;

    // Enable serial interrupt
    PIE1bits.RCIE = 1;
}


// Send the initialization sequence to the GPS
static void gps_send_init_seq(void)
{
    const char* seq = gps_init_seq_data;
    uint8_t val;

    for (val = *seq ; val != '\xff' ; val = *(++seq)) {
        while (TXSTAbits.TRMT == 0); // Wait for previous character to be sent
        if (val == '\xfe') {
            uint32_t wait = 0x1ffff;
            while (--wait);
        } else {
            TXREG = val;
        }
    }
}


// Wait for a byte to be received on the serial, and return it.
static inline uint8_t gps_wait_byte(void)
{
    while (!PIR1bits.RCIF);// Wait for a byte to be received

    // Read and return the received byte
    return RCREG;
}


// Wait for a binary message to be received on the serial. Return the message
// type. Does not check message CRC or length.
static uint8_t gps_wait_msg(void)
{
    uint8_t msg_type;

    for (;;) {
        if (gps_wait_byte() != '\xA0')
            continue;

        if (gps_wait_byte() != '\xA2')
            continue;

        // Receive the length
        gps_wait_byte();
        gps_wait_byte();

        // Message type
        msg_type = gps_wait_byte();

        while (gps_wait_byte() != '\xB0') {
            // Wait for message end
        }

        if (gps_wait_byte() != '\xB3')
            continue;

        return msg_type;
    }
}


bool gps_handle_serial_rx(void)
{
    char recv_byte = RCREG; // Receive the data and acknowledge the interrupt

    idle_ticks = 0;

    if (gps_status >= STATUS_ERR_SERIAL) {
        // Do nothing if a error occurred, so it is visible to the user
        return false;
    }

    if (RCSTAbits.FERR || RCSTAbits.OERR) {
        GPS_SET_ERR(STATUS_ERR_SERIAL);
        return false;
    }

    switch (recv_state) {
        case RECEIVED_NOTHING:
            if (recv_byte == '\xA0') { // First start byte
                recv_state = RECEIVING_START;
            } else {
                GPS_SET_ERR(STATUS_ERR_INVAL_MSG_SEQ);
            }
        break;
        case RECEIVING_START:
            if (recv_byte == '\xA2') { // Second start byte
                recv_state = RECEIVING_LENGTH1;
            } else {
                GPS_SET_ERR(STATUS_ERR_INVAL_MSG_SEQ);
            }
        break;
        case RECEIVING_LENGTH1:
            if (recv_byte == 0) { // Length should be < 256 so high byte = 0
                recv_state = RECEIVING_LENGTH2;
            } else {
                GPS_SET_ERR(STATUS_ERR_INVAL_MSG_TYPE);
            }
        break;
        case RECEIVING_LENGTH2:
            if (recv_byte <= sizeof(payload_buf)) {
                payload_length = recv_byte;
                recv_pos = 0;
                calc_csum = 0;
                recv_state = RECEIVING_PAYLOAD;
            } else {
                GPS_SET_ERR(STATUS_ERR_INVAL_MSG_TYPE);
            }
        break;
        case RECEIVING_PAYLOAD:
            payload_buf[recv_pos] = recv_byte;
            calc_csum += recv_byte;
            recv_pos += 1;
            if (recv_pos == payload_length) {
                recv_state = RECEIVING_CSUM1;
                recv_pos = 0;
            }
        break;
        case RECEIVING_CSUM1:
            if (recv_byte == ((calc_csum >> 8) & 0x7F)) {
                recv_state = RECEIVING_CSUM2;
            } else {
                GPS_SET_ERR(STATUS_ERR_INVAL_MSG_CSUM);
            }
        break;
        case RECEIVING_CSUM2:
            if (recv_byte == (calc_csum & 0xFF)) {
                recv_state = RECEIVING_END1;
            } else {
                GPS_SET_ERR(STATUS_ERR_INVAL_MSG_CSUM);
            }
        break;
        case RECEIVING_END1:
            if (recv_byte == '\xb0') { // First end byte
                recv_state = RECEIVING_END2;
            } else {
                GPS_SET_ERR(STATUS_ERR_INVAL_MSG_SEQ);
            }
        break;
        case RECEIVING_END2:
            if (recv_byte == '\xb3') { // Second end byte
                recv_state = RECEIVE_DONE;
                return true;
            } else {
                GPS_SET_ERR(STATUS_ERR_INVAL_MSG_SEQ);
            }
        break;
        case RECEIVE_DONE:
            GPS_SET_ERR(STATUS_ERR_OVERFLOW);
        break;
    }

    return false;
}


void gps_handle_tick(void)
{
    if (idle_ticks < 255) {
        idle_ticks += 1;
    } else if (gps_status < STATUS_ERR_NO_DATA) {
        GPS_SET_ERR(STATUS_ERR_NO_DATA);
    }
}


void gps_process_received(void)
{
    // If a message was actually received, the receive state and the other
    // variables will be stable.
    if (recv_state != RECEIVE_DONE)
        return;

    if ((payload_buf[0] == 11) && (payload_length == 3)) {
        // Message 11: acknowledgment of command -- ignored
        recv_state = RECEIVED_NOTHING;
        return;
    }

    if ((payload_buf[0] == 225) && (payload_length == 39)) {
        // Message 225: statistics channel -- ignored
        // FIXME find how to disable this message (is debug correctly disabled?)
        recv_state = RECEIVED_NOTHING;
        return;
    }

    if (payload_buf[0] == 93) {
        // Message 93: ??? (the payload length seems to vary; seen 17 and 150)
        // FIXME find how to disable this message (is debug correctly disabled?)
        recv_state = RECEIVED_NOTHING;
        return;
    }

    if ((payload_buf[0] != 7) | (payload_length != 20)) {
        // Unexpected message

        GPS_SET_ERR(STATUS_ERR_INVAL_MSG_TYPE);
        return;
    }

    uint16_t gps_week;
    uint32_t gps_time_of_week;

    gps_week = ((uint16_t)payload_buf[1] << 8) | (uint16_t)payload_buf[2];
    gps_time_of_week = ((uint32_t)payload_buf[3] << 24) |
            ((uint32_t)payload_buf[4] << 16) |
            ((uint32_t)payload_buf[5] << 8) | (uint32_t)payload_buf[6];

    if (gps_week <= 1711) {
        // GPS time of 2012; seems to be returned before the GPS is synchronized
        // FIXME: Find a better way to detect desynchronized GPS? The "SVs"
        // info (byte 7 of the message payload) might indicate the number
        // of satellites, but it seems to always be 0.
        gps_status = STATUS_UNSYNC;
        recv_state = RECEIVED_NOTHING;
        return;
    }

    // There are 315964800 seconds between the UTC epoch 1/1/1970 and the
    // GPS epoch 6/1/1980. However, there have been 18 leap seconds (so far)
    // since the GPS epoch; UTC stands still during the leap second, but the
    // GPS time does not, so we need to subtract the amount of leap seconds from
    // the initial delta.
    gps_deciseconds = 31596480000 - 1800 + ((uint64_t)gps_week * 60480000) +
        (uint64_t)gps_time_of_week;

    gps_status = STATUS_OK;

    recv_state = RECEIVED_NOTHING;
}

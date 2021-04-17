// 150189-71 Nixie Clock alternative firmware
// Copyright (C) Vincent Duvert
// Distributed under the terms of the MIT license.

#include "gps.h"

#include <stdint.h>
#include <stdbool.h>
#include <xc.h>

// Definition of extern variables
enum gps_status_val gps_status;

// Message payload buffer
static char payload_buf[20];
static uint8_t payload_length;

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

// GPS initialization reset string
static const char* gps_reset_seq = (
    // Switch to binary protocol
    "$PSRF100,0,4800,8,1,0*0F\r\n"
    // Disable all messages
    "\xa0\xa2\x00\x08\xa6\x02\x00\x00\x00\x00\x00\x00\x00\xa8\xb0\xb3"
    // End marker
    "\xff"
);

// GPS initialization setup string
static const char* gps_setup_seq = (
    // Enable message 7 every 10 seconds
    "\xa0\xa2\x00\x08\xa6\x00\x07\x0a\x00\x00\x00\x00\x00\xb7\xb0\xb3"
    // End marker
    "\xff"
);

static void gps_send_seq(const char* seq);


void gps_init_reset(void)
{
    gps_status = STATUS_UNSYNC;
    recv_state = RECEIVED_NOTHING;

    gps_send_seq(gps_reset_seq);
}


void gps_init_setup(void)
{
    gps_send_seq(gps_setup_seq);
}


static void gps_send_seq(const char* seq)
{
    for (; *seq != '\xff' ; seq += 1) {
        while (TXSTAbits.TRMT == 0); // Wait for previous character to be sent
        TXREG = *seq;
    }
}


bool gps_handle_serial_rx(void)
{
    char recv_byte = RCREG; // Receive the data and acknowledge the interrupt

    if (gps_status >= STATUS_ERR_SERIAL) {
        // Do nothing if a error occurred, so it is visible to the user
        return false;
    }

    if (RCSTAbits.FERR || RCSTAbits.OERR) {
        gps_status = STATUS_ERR_SERIAL;
        return false;
    }

    switch (recv_state) {
        case RECEIVED_NOTHING:
            if (recv_byte == '\xA0') { // First start byte
                recv_state = RECEIVING_START;
            } else {
                gps_status = STATUS_ERR_INVALID_MSG;
            }
        break;
        case RECEIVING_START:
            if (recv_byte == '\xA2') { // Second start byte
                recv_state = RECEIVING_LENGTH1;
            } else {
                gps_status = STATUS_ERR_INVALID_MSG;
            }
        break;
        case RECEIVING_LENGTH1:
            if (recv_byte == 0) { // Length should be < 256 so high byte = 0
                recv_state = RECEIVING_LENGTH2;
            } else {
                gps_status = STATUS_ERR_INVALID_MSG;
            }
        break;
        case RECEIVING_LENGTH2:
            if (recv_byte <= sizeof(payload_buf)) {
                payload_length = recv_byte;
                recv_pos = 0;
                calc_csum = 0;
                recv_state = RECEIVING_PAYLOAD;
            } else {
                gps_status = STATUS_ERR_INVALID_MSG;
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
                gps_status = STATUS_ERR_CORRUPT_MSG;
            }
        break;
        case RECEIVING_CSUM2:
            if (recv_byte == (calc_csum & 0xFF)) {
                recv_state = RECEIVING_END1;
            } else {
                gps_status = STATUS_ERR_CORRUPT_MSG;
            }
        break;
        case RECEIVING_END1:
            if (recv_byte == '\xb0') { // First end byte
                recv_state = RECEIVING_END2;
            } else {
                gps_status = STATUS_ERR_INVALID_MSG;
            }
        break;
        case RECEIVING_END2:
            if (recv_byte == '\xb3') { // Second end byte
                recv_state = RECEIVE_DONE;
                return true;
            } else {
                gps_status = STATUS_ERR_INVALID_MSG;
            }
        break;
        case RECEIVE_DONE:
            gps_status = STATUS_ERR_OVERFLOW;
        break;
    }

    return false;
}


void gps_process_received(void)
{
    // If a message was actually received, the receive state and the other
    // variables will be stable.
    if (recv_state != RECEIVE_DONE)
        return;

    if (payload_buf[0] == 11) {
        // Message 11: acknowledgment of command -- ignored
        recv_state = RECEIVED_NOTHING;
        return;
    }

    if (payload_buf[0] != 7) {
        // Unexpected message

        gps_status = STATUS_ERR_INVALID_MSG;
        return;
    }

    recv_state = RECEIVED_NOTHING;
}

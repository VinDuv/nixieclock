// 150189-71 Nixie Clock alternative firmware
// Copyright (C) Vincent Duvert
// Distributed under the terms of the MIT license.

#include "gps.h"

#include <stdint.h>
#include <stdbool.h>
#include <xc.h>

// Definition of extern variables
enum gps_status_val gps_status;

static char recv_buf[128];
static uint8_t recv_count;

// GPS initialization string
static const char* gps_init_seq = (
    // Switch to binary protocol
    "$PSRF100,0,4800,8,1,0*0F\r\n"
    // Disable all messages
    "\xa0\xa2\x00\x08\xa6\x02\x00\x00\x00\x00\x00\x00\x00\xa8\xb0\xb3"
    // Enable message 7 every 10 seconds
    "\xa0\xa2\x00\x08\xa6\x00\x07\x0a\x00\x00\x00\x00\x00\xb7\xb0\xb3"
    // End marker
    "\xff"
);


void gps_init(void)
{
    recv_count = 0;

    const char* ptr;
    for (ptr = gps_init_seq ; *ptr != '\xff' ; ptr += 1) {
        while (TXSTAbits.TRMT == 0); // Wait for previous character to be sent
        TXREG = *ptr;
    }
}


void gps_handle_serial_rx(void)
{
    char recv_byte = RCREG; // Receive the data and acknowledge the interrupt

    if (gps_status >= STATUS_ERR_SERIAL) {
        // Do nothing if a error occurred, so it is visible to the user
        return;
    }

    if (RCSTAbits.FERR || RCSTAbits.OERR) {
        gps_status = STATUS_ERR_SERIAL;
        return;
    }

    if (recv_count == sizeof(recv_buf)) {
        gps_status = STATUS_ERR_OVERFLOW;
        return;
    }

    recv_buf[recv_count] = recv_byte;
    recv_count += 1;
}

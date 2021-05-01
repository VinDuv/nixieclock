// 150189-71 Nixie Clock alternative firmware
// Distributed under the terms of the MIT license.

#ifndef GPS_H
#define GPS_H

#include <stdbool.h>
#include <stdint.h>

// GPS error and sync status
enum gps_status_val {
    STATUS_OK = 0,                  // GPS communication OK
    STATUS_ERR_NO_DATA = 1,         // GPS module not responding
    STATUS_ERR_SERIAL = 2,          // Serial I/O communication error
    STATUS_ERR_OVERFLOW = 3,        // Input message buffer overflow
    STATUS_ERR_INVAL_MSG_SEQ = 4,   // Invalid message received from GPS (seq)
    STATUS_ERR_INVAL_MSG_CSUM = 5,  // Invalid message received from GPS (csum)
    STATUS_ERR_INVAL_MSG_TYPE = 6,  // Invalid message received from GPS (type)
};
extern enum gps_status_val gps_status;
extern bool gps_is_sync;

// Time received from GPS, in seconds * 100 from 1/1/1970 00:00:00 UTC
// Updated when processing messages
extern uint64_t gps_deciseconds;

// Initialize the GPS receiver. The serial receive status and interrupt should
// be disabled; they will be automatically enabled when this function returns.
void gps_init(void);

// Handle serial reception interrupt. Return true if a message is received.
bool gps_handle_serial_rx(void);

// Handle a tick interrupt (used for timeout detection)
void gps_handle_tick(void);

// Process the received message.
void gps_process_received(void);
#endif

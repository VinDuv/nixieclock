// 150189-71 Nixie Clock alternative firmware
// Distributed under the terms of the MIT license.

#ifndef GPS_H
#define GPS_H

#include <stdbool.h>

// GPS sync status
enum gps_status_val {
    STATUS_OK = 0,                  // GPS is synchronized
    STATUS_UNSYNC = 1,              // GPS not synchronized
    STATUS_ERR_NO_DATA = 2,         // GPS module not responding
    STATUS_ERR_SERIAL = 3,          // Serial I/O communication error
    STATUS_ERR_OVERFLOW = 4,        // Input message buffer overflow
    STATUS_ERR_INVAL_MSG_SEQ = 5,   // Invalid message received from GPS (seq)
    STATUS_ERR_INVAL_MSG_CSUM = 6,  // Invalid message received from GPS (csum)
    STATUS_ERR_INVAL_MSG_TYPE = 7,  // Invalid message received from GPS (type)
};
extern enum gps_status_val gps_status;

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

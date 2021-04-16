// 150189-71 Nixie Clock alternative firmware
// Distributed under the terms of the MIT license.

#ifndef GPS_H
#define GPS_H

#include <stdbool.h>

// GPS sync status
enum gps_status_val {
    STATUS_OK = 0,              // GPS is synchronized
    STATUS_UNSYNC = 1,          // GPS not synchronized
    STATUS_ERR_NO_DATA = 2,     // GPS module not responding
    STATUS_ERR_SERIAL = 3,      // Serial I/O communication error
    STATUS_ERR_OVERFLOW = 4,    // Message too long received from GPS
    STATUS_ERR_INVALID_MSG = 5, // Invalid message received from GPS
    STATUS_ERR_CORRUPT_MSG = 6, // Corrupt message received from GPS (checksum)
};
extern enum gps_status_val gps_status;

// Initialize the GPS module by setting the message rates
void gps_init(void);

// Handle serial reception interrupt. Return true if a message is received.
bool gps_handle_serial_rx(void);

// Process the received message.
void gps_process_received(void);
#endif

#ifndef FLAGS_H
#define FLAGS_H

#include <stdint.h>

extern const uint8_t CLIENT_HANDLE_FLAG;
extern const uint8_t HANDLE_GOOD_FLAG;
extern const uint8_t HANDLE_BAD_FLAG;
extern const uint8_t BROADCAST_FLAG;
extern const uint8_t UNICAST_FLAG;
extern const uint8_t MULTICAST_FLAG;
extern const uint8_t CAST_ERROR_FLAG;
extern const uint8_t GET_HANDLES_FLAG;
extern const uint8_t TOTAL_HANDLES_FLAG;
extern const uint8_t SENT_HANDLE_FLAG; 
extern const uint8_t DONE_SENDING_HANDLES_FLAG;

uint8_t getFlag(uint8_t *flagStart);

#endif
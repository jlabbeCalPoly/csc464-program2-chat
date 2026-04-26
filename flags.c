#include <stdint.h>
#include <string.h>

const uint8_t CLIENT_HANDLE_FLAG = 1;
const uint8_t HANDLE_GOOD_FLAG = 2;
const uint8_t HANDLE_BAD_FLAG = 3;
const uint8_t BROADCAST_FLAG = 4;
const uint8_t UNICAST_FLAG = 5;
const uint8_t MULTICAST_FLAG = 6;
const uint8_t CAST_ERROR_FLAG = 7; // Sent to from the server to the sending client in the event the handle couldn't be resolved. Sends one for each send failure
const uint8_t GET_HANDLES_FLAG = 10;
const uint8_t TOTAL_HANDLES_FLAG = 11; // Simply sends back how many handles are in the handle table.
const uint8_t SENT_HANDLE_FLAG = 12; // Contains information for one of the handles in the handle table
const uint8_t DONE_SENDING_HANDLES_FLAG = 13; // Sent right after the last packet containing 12 for the handle

/**
 * Retrieves the value of the flag from the payload, returns it
 * 
 * @param flagStart Pointer to where the flag begins in the payload
 */ 
uint8_t getFlag(uint8_t *flagStart) {
    uint8_t flag;
    memcpy(&flag, flagStart, 1);

    return flag;
}

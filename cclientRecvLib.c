// Library for handling receiving server PDUs on the client
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/**
 * Terminates the program with an error message in the event the server deems the handle invalid (called when receiving a flag = 2 from the client)
 */
void onRecvBadHandle() {
    printf("Error: Invalid handle, please try again\n");
    exit(1);
}

void onRecvMessage(uint8_t *dataBuffer, int lengthOfData) {
    // First byte is the sending client's handle length
    uint8_t handleLength = 0;
    memcpy(&handleLength, dataBuffer, 1);

    // Following "handleLength" bytes are the handle itself, also accounting for the null so it can be printed easily
    uint8_t handleBuffer[handleLength + 1];
    memcpy(handleBuffer, dataBuffer + 1, handleLength);
    handleBuffer[handleLength] = '\0';

    uint8_t messageLength = lengthOfData - handleLength - 1;
    // Space for the message, also accounting for the null so it can be printed easily
    uint8_t messageBuffer[messageLength + 1];
    memcpy(messageBuffer, dataBuffer + handleLength + 1, messageLength);
    messageBuffer[messageLength] = '\0';

    // Print out the message:
    printf("%s: %s\n", handleBuffer, messageBuffer);
}

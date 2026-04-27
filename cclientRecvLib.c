// Library for handling receiving server PDUs on the client
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

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

    // debug
    // printf("Handle length: %d  Message length: %d\n", handleLength, messageLength);

    // Space for the message (message already has a null byte, so no need to add it)
    uint8_t messageBuffer[messageLength];
    memcpy(messageBuffer, dataBuffer + handleLength + 1, messageLength);

    // Print out the message
    printf("%s: %s\n", handleBuffer, messageBuffer);
}

void onRecvCastError(uint8_t *dataBuffer) {
    // First byte is the destination client's handle length
    uint8_t handleLength = 0;
    memcpy(&handleLength, dataBuffer, 1);

     // Following "handleLength" bytes are the handle itself, also accounting for the null so it can be printed easily
    uint8_t handleBuffer[handleLength + 1];
    memcpy(handleBuffer, dataBuffer + 1, handleLength);
    handleBuffer[handleLength] = '\0';

    // Print out the message
    printf("Client with the following handle does not exist: %s\n", handleBuffer);
}

void onRecvTotalHandles(uint8_t *dataBuffer) {
    // First four bytes represent the number of handles (IN NETWORK ORDER)
    uint32_t handlesNet;
    memcpy(&handlesNet, dataBuffer, 4);
    uint32_t handlesHost = ntohl(handlesNet);

    // Print out the number of handles
    printf("Number of clients: %d\n", handlesHost);
}

void onRecvSentHandle(uint8_t *dataBuffer, int lengthOfData) {
     // First byte is the sending client's handle length
    uint8_t handleLength = 0;
    memcpy(&handleLength, dataBuffer, 1);

    // Following "handleLength" bytes are the handle itself, also accounting for the null so it can be printed easily
    uint8_t handleBuffer[handleLength + 1];
    memcpy(handleBuffer, dataBuffer + 1, handleLength);
    handleBuffer[handleLength] = '\0';

    // Print out the message
    printf("\t%s\n", handleBuffer);
}
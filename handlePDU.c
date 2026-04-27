#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>

#include "safeUtil.h"

/**
 * Simple copies the provided flag into the first byte of the dataBuffer, returing the additional payload length added
 * 
 * @param dataBuffer Pointer to the start of the data buffer
 * @param flag The flag to add
 */
int addFlagToBuffer(uint8_t *dataBuffer, uint8_t flag) {
    memcpy(dataBuffer, &flag, 1);
    return 1;
}

/**
 Create the PDU and send the PDU. Return value is data bytes sent
 
 @param clientSocket The socket number to send to
 @param dataBuffer Pointer to the start of the payload
 @param lengthOfData The payload size (in bytes)
*/
int sendPDU(int clientSocket, uint8_t *dataBuffer, int lengthOfData) {
    uint16_t pduBufferLength = lengthOfData + 2;
    uint8_t pduBuffer[pduBufferLength];

    uint16_t pduBufferLengthNetwork = htons(pduBufferLength);
    // Copy the length of the PDU (header + payload) into the first two bytes
    memcpy(pduBuffer, &pduBufferLengthNetwork, 2);

    // Copy the contents of the data buffer into the remaining space
    memcpy(pduBuffer + 2, dataBuffer, lengthOfData);

    int totalSendBytes = safeSend(clientSocket, pduBuffer, pduBufferLength, MSG_WAITALL);

    // calculate the number of DATA bytes sent
    int dataSendBytes = totalSendBytes > 2 ? totalSendBytes - 2 : 0;
    return dataSendBytes;
}

int recvPDU(int clientSocket, uint8_t * dataBuffer, int bufferSize) {
    // First recv(), get the length of the application PDU (header + payload)
    uint8_t lengthBuffer[2];
    int recvHeaderBytes = 0;
    if ((recvHeaderBytes = safeRecv(clientSocket, lengthBuffer, 2, MSG_WAITALL)) <= 0) {
        return recvHeaderBytes;
    };
    uint16_t lengthNetwork;
    memcpy(&lengthNetwork, lengthBuffer, 2);
    uint16_t lengthHost = ntohs(lengthNetwork);

    // debug
    // printf("received header length is: %d\n", lengthHost);
     
    // Make sure the data length fits (not inluding the additional bytes for the pdu length field) within the dataBuffer size
    int dataLength = lengthHost - 2;
    if (bufferSize <= dataLength) {
        printf("dataBuffer of size %d is too small for data length %d\n", bufferSize, dataLength);
        // Only use the amount of data that can be fit in the buffer
        return bufferSize;
    }

    // Second recv(), get the payload (and print the payload contents, if any)
    int recvPayloadBytes = safeRecv(clientSocket, dataBuffer, dataLength, MSG_WAITALL);
    return recvPayloadBytes;
}
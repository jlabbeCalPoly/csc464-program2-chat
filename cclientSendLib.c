// Library for sending PDUs from the client to the server
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "flags.h"
#include "handlePDU.h"

// Basic function for sending packets to the server
void sendToServer(int socketNum, uint8_t *dataBuffer, int lengthOfData) {	
	int sent = sendPDU(socketNum, dataBuffer, lengthOfData);
	if (sent < 0) {
		perror("Server has terminated");
		exit(-1);
	}

    // debug
	// printf("Socket %d: Sent, Length: %d msg: %s\n", socketNum, sent, dataBuffer);
}

/**
 * Make sure the provide client handle meets basic requirments, then send to the server for validation
 * 
 * @param socketNum The socket number for the client socket
 * @param handle The provided handle length
 * @param MAXBUF Constant for the max size of the data buffer
 */
void validateAndSendClientHandle(int socketNum, char *handle, int MAXBUF) {
	uint8_t dataBuffer[MAXBUF]; //data buffer
	int lengthOfData = 0;
	lengthOfData += addFlagToBuffer(dataBuffer, CLIENT_HANDLE_FLAG);

	uint8_t handleLength = strlen(handle);
    if (handleLength > 100) {
        printf("Error: handle is too long, can only be up to 100 characters\n");
        exit(1);
    }

	// Copy the length of the handle into the byte right next to the flag
	memcpy(dataBuffer + 1, &handleLength, 1);
	lengthOfData += 1;

	// Copy the handle into the remaining space
	memcpy(dataBuffer + 2, handle, handleLength);
	lengthOfData += handleLength;

	// Send data to the server
	sendToServer(socketNum, dataBuffer, lengthOfData);
}

/**
 * parse and validate the input from stdin, then send to the server if the input meets requirements
 * 
 * @param socketNum The socket number for the client socket
 * @param dataBuffer The buffer containing the input
 * @param lengthOfData The length of the entire input
 */
void parseAndSendStdin(int socketNum, uint8_t *dataBuffer, int lengthOfData) {
    sendToServer(socketNum, dataBuffer, lengthOfData);
}
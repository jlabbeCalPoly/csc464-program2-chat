// Library for handling client PDUs and sending PDUs from the server back to the client
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "flags.h"
#include "pollLib.h"
#include "handleTable.h"
#include "handlePDU.h"

/*
    Called when data needs to be sent back to the client after verifying/rejecting their handle
*/
void validateAndAddClientHandleOnError(int clientSocket, uint8_t *dataBuffer, int lengthOfData) {
    lengthOfData += addFlagToBuffer(dataBuffer, HANDLE_BAD_FLAG);
	sendPDU(clientSocket, dataBuffer, lengthOfData);
}

/** 
 * Attempts to add the provided client to the handle table (called when receiving a flag = 1 from the client)
 *  
 * @param clientSocket The socket that the client is currently connected to on the server
 * @param payload Pointer to the memory location of the message payload
 * @param payloadLength The length of the payload
 */ 
void validateAndAddClientHandle(int clientSocket, uint8_t *payload, int payloadLength) {
	uint8_t dataBuffer[1];
	int lengthOfData = 0;

	// Get the handle length
	uint8_t handleLength;
	memcpy(&handleLength, payload, 1);

	if (handleLength > MAX_HANDLE_LEN) {
		validateAndAddClientHandleOnError(clientSocket, dataBuffer, lengthOfData);
		return;
	}

	// Format the handle for cross-referancing with entries in the handle table
	uint8_t handle[100] = {0};
	formatHandle(handle, payload + 1, handleLength);

	int index = checkAddToHandleTable(handle);
	if (index == -1) {
		validateAndAddClientHandleOnError(clientSocket, dataBuffer, lengthOfData);
		return;
	}

	addToHandleTable(handle, clientSocket, index);
	lengthOfData += addFlagToBuffer(dataBuffer, HANDLE_GOOD_FLAG);
	sendPDU(clientSocket, dataBuffer, lengthOfData);
}

/**
 * send a unicast to the provided client, if any
 * 
 * @param clientSocket The socket that the client is currently connected to on the server
 * @param payload Pointer to the memory location of the message payload
 * @param payloadLength The length of the payload
 */
void sendUnicast(int clientSocket, uint8_t *dataBuffer, int lengthOfData) {
    
}
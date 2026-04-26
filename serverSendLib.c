// Library for handling client PDUs and sending PDUs from the server back to the client
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
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

	addToHandleTable(handle, handleLength, clientSocket, index);
	lengthOfData += addFlagToBuffer(dataBuffer, HANDLE_GOOD_FLAG);
	sendPDU(clientSocket, dataBuffer, lengthOfData);
}

void sendUnicastOnError(int clientSocket, uint8_t *handleBuffer, uint8_t handleLength) {
	// 1 byte for the flag, 1 byte for the handle length, and space for the handle itself
	uint8_t lengthOfData = 2 + handleLength;
	uint8_t dataBuffer[lengthOfData];

	// Copy data into the buffer
	memcpy(dataBuffer, &CAST_ERROR_FLAG, 1);
	memcpy(dataBuffer + 1, &handleLength, 1);
	memcpy(dataBuffer + 2, handleBuffer, handleLength);

	sendPDU(clientSocket, dataBuffer, lengthOfData);
}

/**
 * Send a unicast to the provided client, if valid. Returns an error message to the client in the event the 
 * 
 * @param clientSocket The socket that the client is currently connected to on the server
 * @param payload Pointer to the memory location of the message payload
 * @param payloadLength The length of the payload
 */
void sendUnicast(int clientSocket, uint8_t *payload, int payloadLength) {
	// Get the handle length for the client being SENT TO
	uint8_t handleLength;
	memcpy(&handleLength, payload, 1);

	uint8_t handleBuffer[handleLength];
	memcpy(handleBuffer, payload + 1, handleLength);

	// Immediately check if the handleLength is too large
	if (handleLength > MAX_HANDLE_LEN) {
		// debug
		printf("Client handle is too large, failed to send\n");

		sendUnicastOnError(clientSocket, handleBuffer, handleLength);
		return;
	}

	// format the handle, get the socket it's attached to (if any)
	uint8_t handle[100] = {0};
	formatHandle(handle, payload + 1, handleLength);

	int socket = getSocketFromHandle(handle);
	if (socket == -1) {
		// debug
		printf("Failed to get the client socket from the handle, failed to send\n");

		sendUnicastOnError(clientSocket, handleBuffer, handleLength);
		return;
	}

	// Valid client handle and active socket, so send the message to them
	// First, determine how long the message is, then copy its contents to a buffer
	uint8_t headerLength = handleLength + 1;
	uint8_t messageLength = payloadLength - headerLength;
	uint8_t messageBuffer[messageLength];
	memcpy(messageBuffer, payload + headerLength, messageLength);

	uint8_t fromClientHandleBuffer[100];
	int fromClientHandleLength = getHandleFromSocket(clientSocket, fromClientHandleBuffer);
	if (fromClientHandleLength == -1) {
		// debug
		printf("Failed to get the from client handle from the socket, failed to send\n");

		sendUnicastOnError(clientSocket, handleBuffer, handleLength);
		return;
	}

	// Finally, build the PDU to send to the client
	// 1 byte for the flag, 1 byte for the fromClient handle length, then space for the handle and message
	uint8_t lengthOfData = 2 + fromClientHandleLength + messageLength;
	uint8_t dataBuffer[lengthOfData];

	memcpy(dataBuffer, &UNICAST_FLAG, 1);
	memcpy(dataBuffer + 1, &fromClientHandleLength, 1);
	memcpy(dataBuffer + 2, fromClientHandleBuffer, fromClientHandleLength);
	memcpy(dataBuffer + 2 + fromClientHandleLength, messageBuffer, messageLength);

	sendPDU(socket, dataBuffer, lengthOfData);

	// debug
	// printf("Successfully send PDU to the recipient\n");
}

// Helper function for sendHandles, sends the packet containing the TOTAL_HANDLES_FLAG
void sendTotalHandles(int clientSocket) {
	uint32_t handlesHost = getActiveHandles();
	uint32_t handlesNet = htonl(handlesHost);

	// 1 byte for the flag, 4 bytes for the total active handles 
	u_int8_t lengthOfData = 5;
	uint8_t dataBuffer[lengthOfData];
	memcpy(dataBuffer, &TOTAL_HANDLES_FLAG, 1);
	memcpy(dataBuffer + 1, &handlesNet, 4);

	sendPDU(clientSocket, dataBuffer, lengthOfData);
}

// Helper function for sendHandles, sends the packets (containing the SENT_HANDLE_FLAG) for each individual active handle 
void sendIndividualHandles(int clientSocket) {
	int handleLength = 0;
	int index = 0;
	uint8_t handleBuffer[100];
	// handleLength will be -1 when there are no more entries to check for
	while ((handleLength = getHandleIfActive(index, handleBuffer)) != -1) {
		// If the returned handleLength isn't 0, means that the entry at this index in handleTable is active
		if (handleLength != 0) {
			// 1 byte for the flag, 1 byte for the handle length, then space for the handle itself
			u_int8_t lengthOfData = 2 + handleLength;
			uint8_t dataBuffer[lengthOfData];
			memcpy(dataBuffer, &SENT_HANDLE_FLAG, 1);
			memcpy(dataBuffer + 1, &handleLength, 1);
			memcpy(dataBuffer + 2, handleBuffer, handleLength);

			sendPDU(clientSocket, dataBuffer, lengthOfData);
		}
		index += 1;
	}
}

// Helper function for sendHandles, sends the packet containing the DONE_SENDING_HANDLES_FLAG
void sendHandlesEnd(int clientSocket) {
	uint8_t lengthOfData = 1;
	uint8_t dataBuffer[lengthOfData];

	memcpy(dataBuffer, &DONE_SENDING_HANDLES_FLAG, 1);

	sendPDU(clientSocket, dataBuffer, lengthOfData);
}

/**
 * Sends 3 types of packets (Total handles, individual handles, done sending handles) to the client
 * 
 * @param clientSocket The socket to send the handle information to
 */
void sendHandles(int clientSocket) {
	sendTotalHandles(clientSocket);
	sendIndividualHandles(clientSocket);
	sendHandlesEnd(clientSocket);
}
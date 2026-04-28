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

void sendCastOnError(int clientSocket, uint8_t *handleBuffer, uint8_t handleLength) {
	// 1 byte for the flag, 1 byte for the handle length, and space for the handle itself
	uint8_t lengthOfData = 2 + handleLength;
	uint8_t dataBuffer[lengthOfData];

	// Copy data into the buffer
	memcpy(dataBuffer, &CAST_ERROR_FLAG, 1);
	memcpy(dataBuffer + 1, &handleLength, 1);
	memcpy(dataBuffer + 2, handleBuffer, handleLength);

	sendPDU(clientSocket, dataBuffer, lengthOfData);
}

/*
 * Recursive function to send the provided text message to each of the recipient clients, returns the length of text message
 *
 * @param clientSocket The socket that the SENDING client is connected to
 * @param payloadStart Pointer to where in the payload the information for the current destination client is
 * @param payloadLengthFromStart The remaining amount of space in the buffer relative to the payloadStart position
 * @param dataBuffer The buffer to send to the destination client
 * @param lengthOfData The amount of data inside the dataBuffer
 * @param handlesRemaining The remaining amount of destination handles that are still in the payload after this one
 */
int sendCastToHandle(
	int clientSocket, 
	uint8_t *payloadStart, 
	int payloadLengthFromStart, 
	uint8_t *dataBuffer,
	int lengthOfData,
	int handlesRemaining
) {
	// Get the handle length for the client being SENT TO
	uint8_t handleLength;
	memcpy(&handleLength, payloadStart, 1);

	uint8_t handleBuffer[handleLength];
	memcpy(handleBuffer, payloadStart + 1, handleLength);

	int lengthOfText;
	int lengthOfHandleInformation = handleLength + 1;
	// Base case: If the number of handles remaining is 0 (this is the last one), then the text is found right after this handle
	if (handlesRemaining == 0) {
		lengthOfText = payloadLengthFromStart - lengthOfHandleInformation;
		memcpy(dataBuffer + lengthOfData, payloadStart + lengthOfHandleInformation, lengthOfText);
	// Otherwise, recurse to continue searching further into the pdu for the text message
	} else {
		lengthOfText = sendCastToHandle(
			clientSocket,
			payloadStart + lengthOfHandleInformation,
			payloadLengthFromStart - lengthOfHandleInformation,
			dataBuffer,
			lengthOfData,
			handlesRemaining - 1
		);
	}

	// Check if the handleLength is too large to send to
	if (handleLength > MAX_HANDLE_LEN) {
		// debug
		// printf("Client handle is too large, failed to send\n");

		sendCastOnError(clientSocket, handleBuffer, handleLength);
		return lengthOfText;
	}

	// format the handle, get the socket it's attached to (if any)
	uint8_t handle[100] = {0};
	formatHandle(handle, payloadStart + 1, handleLength);

	int socket = getSocketFromHandle(handle);
	if (socket == -1) {
		// debug
		// printf("Failed to get the client socket from the handle, failed to send\n");

		sendCastOnError(clientSocket, handleBuffer, handleLength);
		return lengthOfText;
	}

	// Finally, send the PDU to the destination client
	sendPDU(socket, dataBuffer, lengthOfData + lengthOfText);
	
	return lengthOfText;
}

/**
 * Extracts the sending client's handle, handlelength and number of handles to send to from the payload. Then, sends the message to valid handles
 * 
 * @param clientSocket The socket that the client is currently connected to on the server
 * @param payload Pointer to the memory location of the message payload
 * @param payloadLength The length of the payload
 * @param flag The cast type (Unicast or multicast)
 */
void sendCasts(int clientSocket, uint8_t *payload, int payloadLength, int flag) {
	uint8_t dataBuffer[1400];
	int lengthOfData = 0;

	// Get the handle length for the sending client
	uint8_t handleLength;
	memcpy(&handleLength, payload, 1);

	// retrieve the amount of handles that are included in this cast (found after the sending handle)
	uint8_t handleCount;
	memcpy(&handleCount, payload + 1 + handleLength, 1);

	// debug
	// printf("Handle length: %d  Handle count: %d\n", handleLength, handleCount);

	// Copy the flag, sending handle length and handle into the dataBuffer, adjust lengthOfData accordingly
	memcpy(dataBuffer, &flag, 1);
	memcpy(dataBuffer + 1, &handleLength, 1);
	memcpy(dataBuffer + 2, payload + 1, handleLength);

	// Send the message to the provided destination clients
	lengthOfData = 2 + handleLength; // 1 byte for the flag, 1 byte for handle length, then space for the handle itself
	sendCastToHandle(
		clientSocket,
		payload + lengthOfData,
		payloadLength - lengthOfData,
		dataBuffer,
		lengthOfData,
		handleCount - 1
	);
}

/**
 * Sends the message in the payload to all active clients (excluding the sending client)
 * 
 * @param payload Pointer to the memory location of the message payload
 * @param payloadLength The length of the payload
 */
void sendBroadcast(uint8_t *payload, int payloadLength) {
	// Get the handle length for the sending client
	uint8_t handleLength;
	memcpy(&handleLength, payload, 1);

	// Format the handle for cross-referancing with entries in the handle table
	uint8_t handle[100] = {0};
	formatHandle(handle, payload + 1, handleLength);

	// Create the broadcast pdu to send to each valid client (will contain all the information found in the payload + 1 byte for the flag)
	uint8_t lengthOfData = payloadLength + 1;
	uint8_t dataBuffer[lengthOfData];
	memcpy(dataBuffer, &BROADCAST_FLAG, 1); // 1 byte for the flag
	memcpy(dataBuffer + 1, payload, payloadLength); // Copy the contents of the payload over
	
	int socket = 0;
	int index = 0;
	// handleLength will be -1 when there are no more entries to check for
	while ((socket = getSocketIfActiveAndUnique(index, handle)) != -1) {
		// If the returned handleLength isn't 0, means that the entry at this index in handleTable is active
		// Also need to check and make sure that 
		if (socket != 0) {
			sendPDU(socket, dataBuffer, lengthOfData);
		}
		index += 1;
	}
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
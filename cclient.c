/******************************************************************************
* myClient.c
*
* Writen by Prof. Smith, updated Jan 2023
* Use at your own risk.  
*
*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdint.h>

#include "networks.h"
#include "safeUtil.h"
#include "handlePDU.h"
#include "cclientSendLib.h"
#include "cclientRecvLib.h"
#include "pollLib.h"
#include "flags.h"

#define MAXBUF 1400
#define MAXTEXT 199
#define DEBUG_FLAG 1

// Hold onto the client handle
char *clientHandle;
// Hold onto the client handle length
uint8_t clientHandleLength = 0;

// Basic function to put the prompt back on the screen, that way I don't have to copy it each time
void flushPrompt() {
	// Put the prompt “$: “ back out
	printf("$: ");
	fflush(stdout); // Need this since $: won't print on its own when I want due to output buffering
}

void processMsgFromServer(int socketNum) {
	uint8_t dataBuffer[MAXBUF];
	int messageLen = 0;
	
	//now get the data from the client_socket (if any)
	if ((messageLen = recvPDU(socketNum, dataBuffer, MAXBUF)) > 0) {
		// debug
		// printf("Socket %d: Message received, length: %d Data: %s\n", socketNum, messageLen, dataBuffer);

		uint8_t flag = getFlag(dataBuffer);
		// debug
		// printf("Flag: %d\n", flag);
		
		if (flag == HANDLE_GOOD_FLAG) {
			flushPrompt();
		} else if (flag == HANDLE_BAD_FLAG) {
			onRecvBadHandle(clientHandle);
			flushPrompt();
		} else if (flag == UNICAST_FLAG) {
			printf("\n");
			onRecvMessage(dataBuffer + 1, messageLen - 1);
			flushPrompt();
		} else if (flag == MULTICAST_FLAG) {
			printf("\n");
			onRecvMessage(dataBuffer + 1, messageLen - 1);
			flushPrompt();
		} else if (flag == BROADCAST_FLAG) {
			printf("\n");
			onRecvMessage(dataBuffer + 1, messageLen - 1);
			flushPrompt();
		} else if (flag == CAST_ERROR_FLAG) {
			printf("\n");
			onRecvCastError(dataBuffer + 1);
			flushPrompt();
		} else if (flag == TOTAL_HANDLES_FLAG) {
			printf("\n");
			onRecvTotalHandles(dataBuffer + 1);
		} else if (flag == SENT_HANDLE_FLAG) {
			onRecvSentHandle(dataBuffer + 1, messageLen - 1);
		} else if (flag == DONE_SENDING_HANDLES_FLAG) {
			flushPrompt();
		} else {
			// debug
			printf("\nUnknown flag\n");
			flushPrompt();
		}
	} else {
		// Server terminated, so exit the program
		printf("Server Terminated");
		exit(0);
	}
}

// In the event an error occurs when parsing stdin, clear the rest of it before the next input
void clearStdin(uint8_t endOnNewline) {
	// Avoid blocking in the event the entire input has already been consumed
	if (endOnNewline == 1) {
		return;
	}

	int aChar = 0;
	while (aChar != '\n' && aChar != EOF) {
		aChar = getchar();
	}
}

// Reads the text part of the client message (only ends on newline)
int readFromStdinText(uint8_t *buffer, uint8_t *endOnNewline) {
	// Avoid blocking in the event the entire input has already been consumed
	if (*endOnNewline == 1) {
		return 0;
	}

	int aChar = 0;
	int inputLen = 0;        

	while (aChar != '\n') {
		// Return an error if there's not enough space in the buffer
		if (inputLen == MAXBUF) {
			printf("Not enough buffer space when reading from stdin\n");
			return -1;
		}

		aChar = getchar();
		if (aChar == '\n') {
			*endOnNewline = 1;
		} else {
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}
	return inputLen;
}

// Read into the provided buffer for the given amount of values in stdin, then return the amount of bytes read or -1 if an error occured (already reached the end of stdin)
int readFromStdinSplit(uint8_t *buffer, uint8_t *endOnNewline) {
	// Avoid blocking in the event the entire input has already been consumed
	if (*endOnNewline == 1) {
		return 0;
	}

	int aChar = 0;
	int inputLen = 0;        

	while (aChar != '\n' && aChar != ' ') {
		// Return an error if there's not enough space in the buffer
		if (inputLen == MAXBUF) {
			printf("Not enough buffer space when reading from stdin\n");
			return -1;
		}

		aChar = getchar();
		if (aChar == '\n') {
			*endOnNewline = 1;
		} else if (aChar != ' ') {
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}
	return inputLen;
}

// Combine the contents of the header and text buffers into one packet to send to the server
void concatenateAndSendData(int socketNum, uint8_t *headerBuffer, int headerLength, uint8_t *textBuffer, int textLength) {
	// printf("Text length: %d\n", textLength);

	int prevTextTaken = 0;
	
	do {
		uint8_t sendBuffer[MAXBUF];
		// Determine how much text to include in the following send to the server (200 max, but accounting for null -> 199)
		int totalAfterTake = prevTextTaken + 199;
		int textSendLength = totalAfterTake <= textLength ? 199 : textLength - prevTextTaken;

		// Copy the header into the sendBuffer
		memcpy(sendBuffer, headerBuffer, headerLength);

		// Copy the text into the sendBuffer
		memcpy(sendBuffer + headerLength, textBuffer + prevTextTaken, textSendLength);
		
		// Terminate the text string with a null
		sendBuffer[headerLength + textSendLength] = 0;

		// Send to the server, + 1 byte for the lengthOfData parameter since a null byte was added on
		// printf("Sending header bytes: %d  Sending text bytes (With null): %d\n", headerLength, textSendLength + 1);
		sendToServer(socketNum, sendBuffer, headerLength + textSendLength + 1);

		// Update the values of prevTextTaken for the next iteration
		prevTextTaken += textSendLength;
	} while (prevTextTaken < textLength);
}

// Specific function for parsing a handle from stdin (involves check)
int parseHandleFromStdin(uint8_t *buffer, uint8_t *endOnNewline, int bufferOffset) {
	uint8_t handleBuffer[MAXBUF];
	int handleLength = 0;
	if ((handleLength = readFromStdinSplit(handleBuffer, endOnNewline)) == -1) {
		// Error message is already handled by readFromStdinSplit method
		return -1;
	}

	// A valid handle must be at least 1 character and at most 100 characters
	if (handleLength < 1 || handleLength > 100) {
		printf("Invalid command format, all handles must be between 1 and 100 characters\n");
		return -1;
	}

	// Account for the length field in the handle buffer
	int handleBufferLength = handleLength + 1;
	if (handleBufferLength > MAXBUF - bufferOffset) {
		printf("Input too large, max of 1400 input characters\n");
		return -1;
	}
	
	// Copy contents to the buffer, then return the handleLength + 1 (accounting for the length field)
	memcpy(buffer + bufferOffset, &handleLength, 1);
	memcpy(buffer + bufferOffset + 1, handleBuffer, handleLength);

	return handleBufferLength + bufferOffset;
}

// Helper function for adding client's handle and handle length data to the buffer (required for unicast, multicast and broadcast packets), returning the space they take in the buffer
int addHandleToHeader(uint8_t *buffer) {
	memcpy(buffer, &clientHandleLength, 1);
	memcpy(buffer + 1, clientHandle, clientHandleLength);

	return clientHandleLength + 1;
}

int parseStdinHeaderUnicast(uint8_t *buffer, uint8_t *endOnNewline) {
	int bufferOffset = addHandleToHeader(buffer);

	// Make the number of client's being sent to as 1
	buffer[bufferOffset] = 1;

	// Add the destination handle's information to the buffer, accounting for the other information that's already in the buffer
	return parseHandleFromStdin(buffer, endOnNewline, bufferOffset + 1);
}

int parseStdinHeaderMulticast(uint8_t *buffer, uint8_t *endOnNewline) {
	int bufferOffset = addHandleToHeader(buffer);

	// Retrieve the number of handles being sent to, then verify if it's a valid number (must be between 2 and 9)
	uint8_t handleBuffer[MAXBUF];
	int handleCountLength = 0;
	if ((handleCountLength = readFromStdinSplit(handleBuffer, endOnNewline)) == -1 || handleCountLength != 1) {
		return -1;
	}

	// Since the handle count will contain the character representation, need to offset it by the ascii value of '0' to get its numeric value
	uint8_t handleCount;
	memcpy(&handleCount, handleBuffer, 1);
	handleCount -= '0';

	// debug
	// printf("Handle count: %d\n", handleCount);

	if (handleCount < 2 || handleCount > 9) {
		printf("Number of handles for multicast must be between 2 and 9\n");
		return -1;
	}

	buffer[bufferOffset] = handleCount;
	bufferOffset += 1;

	// Loop through and add the handle data to the buffer, terminating early with -1 if there aren't enough handles or space in the buffer
	while (handleCount > 0) {
		if ((bufferOffset = parseHandleFromStdin(buffer, endOnNewline, bufferOffset)) == -1) {
			return -1;
		} 
		handleCount -= 1;
	}
	return bufferOffset;
}

// Broadcast header will only contain the flag (set in parseStdinHeader) and the handle length + name details
int parseStdinHeaderBroadcast(uint8_t *buffer, uint8_t *endOnNewline) {
	return addHandleToHeader(buffer);
}

// Returns the length of the header or -1 if an error is encountered
int parseStdinHeader(uint8_t *headerBuffer, uint8_t *endOnNewline) {
	uint8_t commandBuffer[MAXBUF];
	int commandBufferLength = 0;
	// expecting the command length to be exactly two. Immediate errors if it's not
	if ((commandBufferLength = readFromStdinSplit(commandBuffer, endOnNewline)) != 2) {
		printf("Invalid command format\n");
		return -1;
	}

	uint8_t flag = 0;
	uint8_t handleBuffer[MAXBUF];
	int handleBufferLength = 0;
	// Unicast
	if (memcmp(commandBuffer, "%M", 2) == 0 || memcmp(commandBuffer, "%m", 2) == 0) {
		flag = UNICAST_FLAG;
		handleBufferLength = parseStdinHeaderUnicast(handleBuffer, endOnNewline);
	// Multicast
	} else if (memcmp(commandBuffer, "%C", 2) == 0 || memcmp(commandBuffer, "%c", 2) == 0) {
		flag = MULTICAST_FLAG;
		handleBufferLength = parseStdinHeaderMulticast(handleBuffer, endOnNewline);
	// Broadcast
	} else if (memcmp(commandBuffer, "%B", 2) == 0 || memcmp(commandBuffer, "%b", 2) == 0) {
		flag = BROADCAST_FLAG;
		handleBufferLength = parseStdinHeaderBroadcast(handleBuffer, endOnNewline);
	// Get active handles
	} else if (memcmp(commandBuffer, "%L", 2) == 0 || memcmp(commandBuffer, "%l", 2) == 0) {
		flag = GET_HANDLES_FLAG;
		// No handles are expected on the %L command, so set its length to 0
		handleBufferLength = 0;
	} else {
		// Invalid command
		printf("Invalid command\n");
		return -1;
	}

	int headerBufferLength = 1 + handleBufferLength;
	// Determine if anhy errors occured while parsing the header
	if (handleBufferLength == -1) {
		return -1;
	}
	// Determine if the contents won't exceed the size of the header buffer
	if (headerBufferLength > MAXBUF) {
		printf("Input too large, max of 1400 input characters\n");
		return -1;
	}

	// Copy the flag and handleBuffer data into the headerBuffer
	memcpy(headerBuffer, &flag, 1);
	memcpy(headerBuffer + 1, handleBuffer, handleBufferLength);

	return headerBufferLength;
}

int parseStdinText(uint8_t *textBuffer, uint8_t *endOnNewline) {
	int textBufferLength = 0;
	if ((textBufferLength = readFromStdinText(textBuffer, endOnNewline)) == -1) {
		return -1;
	}

	return textBufferLength;
}

// Process the input and send data to the server in the event parsing completes successfully
void processStdin(int socketNum) {
	uint8_t endOnNewline = 0;
	uint8_t headerBuffer[MAXBUF];
	int headerLength;
	if ((headerLength = parseStdinHeader(headerBuffer, &endOnNewline)) == -1) {
		clearStdin(endOnNewline);

		// debug
		// printf("Error: Failed to parse the stdin header\n");

		// Put the prompt “$: “ back out
		flushPrompt();
		return;
	}

	uint8_t textBuffer[MAXBUF];
	int textLength;
	if ((textLength = parseStdinText(textBuffer, &endOnNewline)) == -1) {
		clearStdin(endOnNewline);

		// debug
		// printf("Error: Failed to parse the stdin text\n");

		// Put the prompt “$: “ back out
		flushPrompt();
		return;
	}

	// If the length of the header and text exceeds 1400 bytes, print an error message and continue
	if (headerLength + textLength > 1400) {
		clearStdin(endOnNewline);

		printf("Input too large, max of 1400 input characters\n");

		// Put the prompt “$: “ back out
		flushPrompt();
		return;
	}

	concatenateAndSendData(socketNum, headerBuffer, headerLength, textBuffer, textLength);

	// Put the prompt “$: “ back out
	flushPrompt();
}

void clientControl(int socketNum, char *handle) {
	//setup and add to poll set
	setupPollSet();
	addToPollSet(STDIN_FILENO);
	addToPollSet(socketNum);

	// First, send the initial packet to the server with the requested handle
	validateAndSendClientHandle(socketNum, handle, MAXBUF);

	while (1) {
		int socket = pollCall(-1);
		if (socket != -1) {
			if (socket == STDIN_FILENO) {
				processStdin(socketNum);
			} else {
				processMsgFromServer(socketNum);
			}
		}
	}
}

void checkArgs(int argc, char * argv[]) {
	/* check command line arguments  */
	if (argc != 4)
	{
		printf("usage: %s handle host-name port-number \n", argv[0]);
		exit(1);
	}
}

int main(int argc, char * argv[]) {
	checkArgs(argc, argv);

	// Save the client handle and handle length
	clientHandle = argv[1];
	clientHandleLength = strlen(argv[1]);
	// Immediate check on the length of the handle
	if (clientHandleLength > 100) {
		printf("Invalid handle, handle longer than 100 characters: %s\n", clientHandle);
		exit(1);
	}
	
	/* set up the TCP Client socket  */
	int socketNum = 0; //socket descriptor
	socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);

	clientControl(socketNum, argv[1]);

	close(STDIN_FILENO);
	close(socketNum);
	
	return 0;
}
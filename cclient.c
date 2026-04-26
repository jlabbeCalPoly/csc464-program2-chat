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
#define DEBUG_FLAG 1

void processMsgFromServer(int socketNum) {
	uint8_t dataBuffer[MAXBUF];
	int messageLen = 0;
	
	//now get the data from the client_socket (if any)
	if ((messageLen = recvPDU(socketNum, dataBuffer, MAXBUF)) > 0) {
		// debug
		// printf("Socket %d: Message received, length: %d Data: %s\n", socketNum, messageLen, dataBuffer);

		uint8_t flag = getFlag(dataBuffer);
		// debug
		printf("Flag: %d\n", flag);
		
		if (flag == HANDLE_GOOD_FLAG) {
			/*
				Nothing actually happens on the client (other than program continuing) when the handle is validated by the server, 
				I just have this here for proof I've considered it
			*/
		} else if (flag == HANDLE_BAD_FLAG) {
			printf("\n");
			onRecvBadHandle();
		} else {
			printf("\n");
			// debug
			printf("Unknown flag");
		}
		// Put the prompt “$: “ back out
		printf("$: ");
		fflush(stdout); // Need this since $: won't print on its own when I want due to output buffering
	} else {
		// Server terminated, so exit the program
		printf("Server terminated");
		exit(0);
	}
}

int readFromStdin(uint8_t * buffer) {
	char aChar = 0;
	int inputLen = 0;        
	
	// Important you don't input more characters than you have space 
	buffer[0] = '\0';
	while (inputLen < (MAXBUF - 1) && aChar != '\n')
	{
		aChar = getchar();
		if (aChar != '\n')
		{
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}
	
	// Null terminate the string
	buffer[inputLen] = '\0';
	inputLen++;
	
	return inputLen;
}

void processStdin(int socketNum) {
	uint8_t dataBuffer[MAXBUF];   //data buffer
	int lengthOfData = 0;         //amount of data to send
	
	lengthOfData = readFromStdin(dataBuffer);
	// debug
	// printf("read: %s string len: %d (including null)\n", dataBuffer, lengthOfData);

	parseAndSendStdin(socketNum, dataBuffer, lengthOfData);
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
	
	/* set up the TCP Client socket  */
	int socketNum = 0; //socket descriptor
	socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);

	clientControl(socketNum, argv[1]);

	close(STDIN_FILENO);
	close(socketNum);
	
	return 0;
}

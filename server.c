/******************************************************************************
* myServer.c
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
#include "pollLib.h"
#include "handlePDU.h"
#include "handleTable.h"
#include "flags.h"
#include "serverSendLib.h"

#define MAXBUF 1400
#define DEBUG_FLAG 1

void recvFromClient(int clientSocket);
int checkArgs(int argc, char *argv[]);
void addNewSocket(int mainServerSocket);

void serverControl(int portNumber) {
	int mainServerSocket = 0;   //socket descriptor for the server socket

	// Setup the handleTable
	handleTableSetup();
	
	// Create the server socket, add it to the poll set
	mainServerSocket = tcpServerSetup(portNumber);
	setupPollSet();
	addToPollSet(mainServerSocket);

	while (1) {
		int socket = pollCall(-1);
		if (socket != -1) {
			if (socket == mainServerSocket) {
				addNewSocket(mainServerSocket);
			} else {
				recvFromClient(socket);
			}
		}
	}
}

// Determine which function to use for processing the message from the client
void processMsgFromClient(int clientSocket, uint8_t *dataBuffer, int messageLen) {
	uint8_t flag = getFlag(dataBuffer);
	printf("Flag: %d\n", flag);
	if (flag == CLIENT_HANDLE_FLAG) {
		validateAndAddClientHandle(clientSocket, dataBuffer + 1, messageLen - 1);
	} else if (flag == UNICAST_FLAG) {
		sendCasts(clientSocket, dataBuffer + 1, messageLen - 1, UNICAST_FLAG);
	} else if (flag == MULTICAST_FLAG) {
		sendCasts(clientSocket, dataBuffer + 1, messageLen - 1, MULTICAST_FLAG);
	} else if (flag == GET_HANDLES_FLAG) {
		sendHandles(clientSocket);
	}
}

void recvFromClient(int clientSocket)
{
	uint8_t dataBuffer[MAXBUF];
	int messageLen = 0;
	
	//now get the data from the client_socket
	if ((messageLen = recvPDU(clientSocket, dataBuffer, MAXBUF)) < 0)
	{
		perror("recv call");
		exit(-1);
	}

	if (messageLen > 0)
	{
		printf("Socket %d: Message received, length: %d Data: %s\n", clientSocket, messageLen, dataBuffer);
		processMsgFromClient(clientSocket, dataBuffer, messageLen);
	}
	else
	{
		printf("Socket %d: Connection closed by other side\n", clientSocket);

		// remove from the handle table (if it was added) and poll set, then closes the socket
		removeFromHandleTable(clientSocket);
		removeFromPollSet(clientSocket);
		close(clientSocket);
	}
}

void addNewSocket(int mainServerSocket) {
	int clientSocket = tcpAccept(mainServerSocket, DEBUG_FLAG);
	addToPollSet(clientSocket);
}

int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	if (argc == 2)
	{
		portNumber = atoi(argv[1]);
	}
	
	return portNumber;
}

int main(int argc, char *argv[])
{
	int portNumber = checkArgs(argc, argv);
	serverControl(portNumber);

	return 0;
}
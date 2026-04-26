#ifndef HANDLE_PDU_H
#define HANDLE_PDU_H

int addFlagToBuffer(uint8_t *dataBuffer, uint8_t flag);
int sendPDU(int clientSocket, uint8_t *dataBuffer, int lengthOfData);
int recvPDU(int clientSocket, uint8_t * dataBuffer, int bufferSize);

#endif

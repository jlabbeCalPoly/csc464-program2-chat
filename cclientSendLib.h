#ifndef CCLIENT_SEND_LIB_H
#define CCLIENT_SEND_LIB_H

#include <stdint.h>

void sendToServer(int socketNum, uint8_t *dataBuffer, int lengthOfData);
void validateAndSendClientHandle(int socketNum, char *handle, int MAXBUF);
void parseAndSendStdin(int socketNum, uint8_t *dataBuffer, int lengthOfData);

#endif

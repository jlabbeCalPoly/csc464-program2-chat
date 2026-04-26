#ifndef SERVER_SEND_LIB_H
#define SERVER_SEND_LIB_H

#include <stdint.h>

void validateAndAddClientHandle(int clientSocket, uint8_t *payload, int payloadLength);

#endif
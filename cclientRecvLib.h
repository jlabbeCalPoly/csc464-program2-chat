#ifndef CCLIENT_RECV_LIB_H
#define CCLIENT_RECV_LIB_H

#include <stdint.h>

void onRecvBadHandle();
void onRecvMessage(uint8_t *dataBuffer, int lengthOfData);
void onRecvCastError(uint8_t *dataBuffer);
void onRecvTotalHandles(uint8_t *dataBuffer);
void onRecvSentHandle(uint8_t *dataBuffer, int lengthOfData);

#endif

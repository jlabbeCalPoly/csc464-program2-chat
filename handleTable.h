#ifndef HANDLE_TABLE_H
#define HANDLE_TABLE_H

#include <stdint.h>

extern const int MAX_HANDLE_LEN;

uint8_t *formatHandle(uint8_t *formatted, uint8_t *handle, int handleLength);
void handleTableSetup();
int checkAddToHandleTable(uint8_t handle[]);
int addToHandleTable(uint8_t handle[], int socket, int index);
void removeFromHandleTable(int socket);

#endif
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

const int MAX_HANDLE_LEN = 100;
const int INACTIVE_HANDLE = 0;
const int ACTIVE_HANDLE = 1;

/*
    Contains the handle name, the socket it's connected to, and if it's currently active or not (client connected/disconnected)
*/
struct handleEntry {
    uint8_t handle[100]; // Max length for the handle name
    int socket;
    int active; // 0 representing inactive, 1 representing active
};

int addedToHandleTable = 0; // The amount of elements that HAVE BEEN ADDED to the handle table
int maxUntilHandleTableResize = 1; // The maximum amount of handles the current handle table can hold
struct handleEntry *handleTable;

/**
 * Format the handle for comparison
 * 
 * @param formatted The buffer to store the formatted handle in
 * @param handle Pointer to the start of the handle
 * @param handleLength how long the handle is
 */
void formatHandle(uint8_t *formatted, uint8_t *handle, int handleLength) {
    memcpy(formatted, handle, handleLength);
}

void handleTableSetup() {
    handleTable = malloc(sizeof(struct handleEntry));
}

/**
 * Check if the provided handle can be added to the handle table. If the handle already exists and is active, returns -1 (failure). Otherwise, returns the index on success
 * 
 * @param handle The handle that 
 */
int checkAddToHandleTable(uint8_t handle[]) {
    int index = 0;
    // Continue looping until there are no more entries in the handle table to check or a match is found
    while (index < addedToHandleTable) {
        struct handleEntry entry = handleTable[index];
        if (memcmp(entry.handle, handle, MAX_HANDLE_LEN) == 0) {
            if(entry.active == ACTIVE_HANDLE) {
                // debug
                printf("There is already an active client with handle: %s\n", handle);

                return -1;
            } else {
                // debug
                printf("Reactivating an old entry with handle: %s\n", handle);

                return index;
            }
        }
        index += 1;
    }
    // debug
    printf("Adding a new entry with handle: %s\n", handle);

    return index;
}

/*
    Grow the handle table (to twice the current size)
    TODO: may want to add error checking in the future in the event realloc fails
*/
void growHandleTable(){
    maxUntilHandleTableResize *= 2;
    handleTable = realloc(handleTable, maxUntilHandleTableResize * sizeof(struct handleEntry));

    // debug
    printf("Growing handle table to size: %d\n", maxUntilHandleTableResize);
}

/**
 * add the given handle/socket to the handle table, growing it if needed
 * 
 */
void addToHandleTable(uint8_t handle[], int socket, int index) {
    // Check if the handle table needs to grow
    if (index >= maxUntilHandleTableResize) {
        growHandleTable();
    }

    // Create the new handleTable entry, update the entry location in the handleTable
    struct handleEntry *entry = malloc(sizeof(struct handleEntry));
    memcpy(entry->handle, handle, MAX_HANDLE_LEN);
    entry->socket = socket;
    entry->active = ACTIVE_HANDLE;

    addedToHandleTable += 1;
    handleTable[index] = *entry;

    // debug
    printf("Handle: %s\n", entry->handle);
    printf("Socket: %d\n", entry->socket);
    printf("Active: %d\n", entry->active);
}

/**
 * "Removes" the provided entry from the handle table on match (ie. sets its "Active" field to the value of INACTIVE_HANDLE)
 * 
 * @param socket The socket the disconnecting client is connected to that should be set as inactive
 */
void removeFromHandleTable(int socket) {
    int index = 0;

    // Continue looping until there are no more entries in the handle table to check or a match is found
    while (index < addedToHandleTable) {
        struct handleEntry entry = handleTable[index];
        printf("Entry socket: %d\n", entry.socket);
        if (entry.socket == socket) {
            // Need to update the table directly
            handleTable[index].active = INACTIVE_HANDLE;

            // debug
            printf("Successfully deactivated handle: %s\n", entry.handle);
            return;
        }
        index += 1;
    }
}
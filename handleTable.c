#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

const int MAX_HANDLE_LEN = 100;
const int INACTIVE_HANDLE = 0;
const int ACTIVE_HANDLE = 1;

/*
    Contains the handle name, the length of the handle name, the socket it's connected to, and if it's currently active or not (client connected/disconnected)
*/
struct handleEntry {
    uint8_t handle[100]; // Max length for the handle name
    uint8_t handleLength;
    int socket;
    int active; // 0 representing inactive, 1 representing active
};

int addedToHandleTable = 0; // The amount of elements that HAVE BEEN ADDED to the handle table
int maxUntilHandleTableResize = 1; // The maximum amount of handles the current handle table can hold
int activeHandles = 0; // The amount of active handles in the table
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
                // printf("There is already an active client with handle: %s\n", handle);

                return -1;
            } else {
                // debug
                // printf("Reactivating an old entry with handle: %s\n", handle);

                return index;
            }
        }
        index += 1;
    }
    // debug
    // printf("Adding a new entry with handle: %s\n", handle);

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
    // printf("Growing handle table to size: %d\n", maxUntilHandleTableResize);
}

/**
 * add the given handle/socket to the handle table, growing it if needed
 * 
 */
void addToHandleTable(uint8_t handle[], uint8_t handleLength, int socket, int index) {
    // Check if the handle table needs to grow
    if (index >= maxUntilHandleTableResize) {
        growHandleTable();
    }

    // Create the new handleTable entry, update the entry location in the handleTable
    struct handleEntry *entry = malloc(sizeof(struct handleEntry));
    memcpy(entry->handle, handle, MAX_HANDLE_LEN);
    entry->handleLength = handleLength;
    entry->socket = socket;
    entry->active = ACTIVE_HANDLE;

    addedToHandleTable += 1;
    activeHandles += 1;
    handleTable[index] = *entry;

    // debug
    // printf("Handle: %s\n", entry->handle);
    // printf("HandleLength: %d\n", entry->handleLength);
    // printf("Socket: %d\n", entry->socket);
    // printf("Active: %d\n", entry->active);
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
        if (entry.socket == socket && entry.active == ACTIVE_HANDLE) {
            // Need to update the table directly
            handleTable[index].active = INACTIVE_HANDLE;
            activeHandles -= 1;

            // debug
            // printf("Successfully deactivated handle: %s\n", entry.handle);
            return;
        }
        index += 1;
    }
}

/**
 * Returns the socket the provided handle is connected to, or -1 in the event an active client with the handle isn't found
 * 
 */
int getSocketFromHandle(uint8_t handle[]) {
    int index = 0;

    // Continue looping until there are no more entries in the handle table to check or a match is found
    while (index < addedToHandleTable) {
        struct handleEntry entry = handleTable[index];

        if (memcmp(entry.handle, handle, MAX_HANDLE_LEN) == 0) {
            if (entry.active == ACTIVE_HANDLE) {
                return entry.socket;
            } else {
                return -1;
            }
        }
        index += 1;
    }

    return -1;
}

/**
 * Retrieves the handle at the given INDEX in handleTable (if exists). 
 * Returns the length of the handle and the handle name in the handleBuffer, or 0 if inactive
 * Returns -1 if the index would be out of bounds
 * 
 * @param index The index in the handleTable to search for
 * @param handleBuffer Where the handle name will be stored on success
 */
int getHandleIfActive(int index, uint8_t handleBuffer[]) {
    // Check if the index is out of bounds
    if (index >= addedToHandleTable) {
        return -1;
    }

    struct handleEntry entry = handleTable[index];
    if (entry.active == ACTIVE_HANDLE) {
        memcpy(handleBuffer, entry.handle, MAX_HANDLE_LEN);
        return entry.handleLength;
    } else {
        return 0;
    }
}

/**
 * Retrieves the handle at the given INDEX in handleTable (if exists). 
 * Returns the socket number or 0 if the entry is inactive and/or the handle names match (used for broadcase)
 * Returns -1 if the index would be out of bounds
 * 
 * @param index The index in the handleTable to search for
 * @param handleBuffer Where sending handle name to compare the entry handle with
 */
int getSocketIfActiveAndUnique(int index, uint8_t handleBuffer[]) {
    // Check if the index is out of bounds
    if (index >= addedToHandleTable) {
        return -1;
    }

    struct handleEntry entry = handleTable[index];
    if (entry.active == ACTIVE_HANDLE && memcmp(entry.handle, handleBuffer, MAX_HANDLE_LEN) != 0) {
        return entry.socket;
    } else {
        return 0;
    }
}

/**
 * Simply returns the number of active handles in the table, useful for putting together packets that use flag = TOTAL_HANDLES_FLAG
 */
int getActiveHandles() {
    return activeHandles;
}
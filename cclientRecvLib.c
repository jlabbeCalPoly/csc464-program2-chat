// Library for handling receiving server PDUs on the client
#include <stdio.h>
#include <stdlib.h>

/**
 * Terminates the program with an error message in the event the server deems the handle invalid (called when receiving a flag = 2 from the client)
 */
void onRecvBadHandle() {
    printf("Error: Invalid handle, please try again\n");
    exit(1);
}

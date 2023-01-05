#include "wire_protocol.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CODE_PUB_REGISTRATION    1
#define CODE_SUB_REGISTRATION    2 
#define CODE_INBOX_CREATE_REQ    3 
#define CODE_INBOX_CREATE_RESP   4  
#define CODE_INBOX_REMOVE_REQ    5 
#define CODE_INBOX_REMOVE_RESP   6  
#define CODE_INBOX_LIST_REQ      7
#define CODE_INBOX_LIST_RESP     8
#define CODE_PUB_SEND_MESSAGE    9 
#define CODE_SUB_RECEIVE_MESSAGE 10

/*
 * Create an encoded string used for requesting the registration of a publisher.
 * 
 * Input:
 *   - pipe_path: the name of the pipe
 *   - box_name: the name of the inbox
 *   - encoded: a pointer to the string where the encoding will be made
 *   - encoded_len: the size of the previous string
 * 
 * Possible errors:
 *   - The size of the string used to encode is smaller than required: returns -1.
 */
int prot_encode_pub_registration(char pipe_path[256], char box_name[32], char* encoded, int encoded_len) {
    if(encoded_len < 291)
        return -1;

    sprintf(encoded, "%d", CODE_PUB_REGISTRATION);
    encoded[1] = '|';
    memcpy(encoded + 2, pipe_path, 256);
    encoded[258] = '|';
    memcpy(encoded + 259, box_name, 32);

    return 0;
}
/*
int main() {
    char pipe_path[256];
    memset(pipe_path, 0, 256);
    strcpy(pipe_path, "123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345");

    char box_name[32];
    memset(box_name, 0, 32);
    strcpy(box_name, "boxes");

    char aaa[256+32+3];
    memset(aaa, 0, 256+32+3);
    prot_encode_pub_registration(pipe_path, box_name, aaa, sizeof(aaa));

    if (aaa[0] == 1) {
        box_name[0] = '1';
    }
    return 0;
}
*/
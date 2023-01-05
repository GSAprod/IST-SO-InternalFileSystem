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

int prot_aux_encode_registrations(int code, char pipe_path[256], char box_name[32], 
        char* encoded, int encoded_len) 
{
    if(encoded_len < 291)
        return -1;
    memset(encoded, 0, encoded_len);

    sprintf(encoded, "%d", code);
    encoded[1] = '|';
    memcpy(encoded + 2, pipe_path, 256);
    encoded[258] = '|';
    memcpy(encoded + 259, box_name, 32);

    return 0;
}

int prot_aux_encode_inbox_response(int code, __int32_t return_code, char error_message[1024], 
        char* encoded, int encoded_len) 
{
    // Calculate the number of digits used to store the return code number.
    int digits = sizeof(return_code);
    do {
        digits++;
        return_code /= 10;
    } while (return_code != 0);

    if(encoded < 1028+digits);
        return -1;
    memset(encoded, 0, encoded_len);

    sprintf(encoded, "%d", code);
    encoded[1] = '|';    

    memcpy(encoded + 2, return_code, digits);
    encoded[3+digits] = "|";

    memcpy(encoded + 4 + digits, error_message, 1024);

    return 0;
}

/*
 * Create an encoded string used for requesting the registration of a publisher.
 * The encoded string should have this format:
 * [ code = 1 (uint8_t) ] | [ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]
 * 
 * Input:
 *   - pipe_path: the name of the pipe
 *   - box_name: the name of the inbox
 *   - encoded: a pointer to the string where the encoding will be made
 *   - encoded_len: the size of the previous string
 * 
 */
int prot_encode_pub_registration(char pipe_path[256], char box_name[32], char* encoded, int encoded_len) {
    return prot_aux_encode_registrations(CODE_PUB_REGISTRATION, pipe_path, box_name, encoded, encoded_len);
}

/*
 * Create an encoded string used for requesting the registration of a subscriber.
 * The encoded string should have this format:
 * [ code = 2 (uint8_t) ] | [ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]
 * 
 * Input:
 *   - pipe_path: the name of the pipe
 *   - box_name: the name of the inbox
 *   - encoded: a pointer to the string where the encoding will be made
 *   - encoded_len: the size of the previous string
 */
int prot_encode_sub_registration(char pipe_path[256], char box_name[32], char* encoded, int encoded_len) {
    return prot_aux_encode_registrations(CODE_SUB_REGISTRATION, pipe_path, box_name, encoded, encoded_len);
}

/*
 * Create an encoded string used for requesting the creation of an inbox.
 * The encoded string should have this format:
 * [ code = 3 (uint8_t) ] | [ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]
 * 
 * Input:
 *   - pipe_path: the name of the pipe
 *   - box_name: the name of the inbox
 *   - encoded: a pointer to the string where the encoding will be made
 *   - encoded_len: the size of the previous string
 */
int prot_encode_inbox_creation_req(char pipe_path[256], char box_name[32], char* encoded, int encoded_len) {
    return prot_aux_encode_registrations(CODE_INBOX_CREATE_REQ, pipe_path, box_name, encoded, encoded_len);
}

/*
 * Create an encoded string used for the response of the creation of an inbox.
 * The encoded string should have this format:
 * [ code = 4 (uint8_t) ] | [ return_code (int32_t) ] | [ error_message (char[1024]) ]
 * 
 * Input:
 *   - return_code: the response code
 *   - error_message: the string with an error message
 *   - encoded: a pointer to the string where the encoding will be made
 *   - encoded_len: the size of the previous string
 */
int prot_encode_inbox_creation_resp(__int32_t return_code, char error_message[1024], char* encoded, int encoded_len) {
    return prot_aux_encode_inbox_response(CODE_INBOX_CREATE_RESP, return_code, error_message, encoded, encoded_len);
}

/*
 * Create an encoded string used for requesting the removal of an inbox.
 * The encoded string should have this format:
 * [ code = 5 (uint8_t) ] | [ client_named_pipe_path (char[256]) ] | [ box_name (char[32]) ]
 * 
 * Input:
 *   - pipe_path: the name of the pipe
 *   - box_name: the name of the inbox
 *   - encoded: a pointer to the string where the encoding will be made
 *   - encoded_len: the size of the previous string
 */
int prot_encode_inbox_removal_req(char pipe_path[256], char box_name[32], char* encoded, int encoded_len) {
    return prot_aux_encode_registrations(CODE_INBOX_REMOVE_REQ, pipe_path, box_name, encoded, encoded_len);
}

/*
 * Create an encoded string used for the response of the removal of an inbox.
 * The encoded string should have this format:
 * [ code = 6 (uint8_t) ] | [ return_code (int32_t) ] | [ error_message (char[1024]) ]
 * 
 * Input:
 *   - return_code: the response code
 *   - error_message: the string with an error message
 *   - encoded: a pointer to the string where the encoding will be made
 *   - encoded_len: the size of the previous string
 */
int prot_encode_inbox_removal_resp(__int32_t return_code, char error_message[1024], char* encoded, int encoded_len) {
    return prot_aux_encode_inbox_response(CODE_INBOX_REMOVE_RESP, return_code, error_message, encoded, encoded_len);
}

/*
 * Create an encoded string used for requesting the listing of all inboxes.
 * The encoded string should have this format:
 * [ code = 7 (uint8_t) ] | [ client_named_pipe_path (char[256]) ]
 * 
 * Input:
 *   - pipe_path: the pathname of the pipe to be used
 *   - encoded: a pointer to the string where the encoding will be made
 *   - encoded_len: the size of the previous string
 */
int prot_encode_inbox_listing_req(char pipe_path[256], char* encoded, int encoded_len) {
    if(encoded_len < 258)
        return -1;
    memset(encoded, 0, encoded_len);

    sprintf(encoded, "%d", CODE_INBOX_LIST_REQ);
    encoded[1] = '|';
    memcpy(encoded + 2, pipe_path, 256);

    return 0;
}

/*
 * Create an encoded string used for a line of the response of the listing of all inboxes.
 * The encoded string should have this format:
 * [ code = 8 (uint8_t) ] | [ last (uint8_t) ] | [ box_name (char[32]) ] | [ box_size (uint64_t) ] | [ n_publishers (uint64_t) ] | [ n_subscribers (uint64_t) ]
 * 
 * Input:
 *   - pipe_path: the pathname of the pipe to be used
 *   - encoded: a pointer to the string where the encoding will be made
 *   - encoded_len: the size of the previous string
 */
int prot_encode_inbox_listing_resp(__int8_t last, char box_name[32], __int64_t box_size, char* encoded, int encoded_len) {
    // TODO: Add missing arguments
    int digitsA = sizeof(last);
    int digitsB = sizeof(box_size);

    if(encoded_len < 38 + digitsA + digitsB)
        return -1;
    memset(encoded, 0, encoded_len);
    
    sprintf(encoded, "%d", encoded_len);
    encoded[1] = '|';
    memcpy(encoded + 2, last, digitsA);
    encoded[3 + digitsA] = '|';
    memcpy(encoded + 4 + digitsA, box_name, 32);
    encoded[37 + digitsA] = '|';
    memcpy(encoded + 38 + digitsA, box_size, digitsB);
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
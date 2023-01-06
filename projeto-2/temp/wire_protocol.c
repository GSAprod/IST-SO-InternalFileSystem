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

int prot_aux_encode_registrations(__int8_t code, char pipe_path[256], char box_name[32], 
        char* encoded, size_t encoded_len) 
{
    if(encoded_len < 291)
        return -1;
    memset(encoded, 0, encoded_len);

    memcpy(encoded, &code, sizeof(__int8_t));
    encoded[1] = '|';
    memcpy(encoded + 2, pipe_path, 256);
    encoded[258] = '|';
    memcpy(encoded + 259, box_name, 32);

    return 0;
}

int prot_aux_encode_inbox_response(__int8_t code, __int32_t return_code, char error_message[1024], 
        char* encoded, size_t encoded_len) 
{
    if(encoded_len < 1030)
        return -1;
    memset(encoded, 0, encoded_len);

    memcpy(encoded, &code, sizeof(__int8_t));
    encoded[1] = '|';    

    memcpy(encoded + 2, &return_code, sizeof(__int32_t));  // Maximum length of a 32-bit signed int is 11. 
    encoded[6] = '|';

    memcpy(encoded + 7, error_message, 1024);
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

    __int8_t code = (__int8_t) CODE_INBOX_LIST_REQ;
    memcpy(encoded, &code, sizeof(__int8_t));
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
int prot_encode_inbox_listing_resp(__int8_t last, char box_name[32], __int64_t box_size, 
        __int64_t n_publishers, __int64_t n_subscribers, char* encoded, int encoded_len) 
{
    // TODO: Add missing arguments
    int digitsA = sizeof(last);
    int digitsB = sizeof(box_size);

    if(encoded_len < 63)
        return -1;
    memset(encoded, 0, encoded_len);
    
    __int8_t code = (__int8_t) CODE_INBOX_LIST_RESP;
    memcpy(encoded, &code, sizeof(__int8_t));
    encoded[1] = '|';
    memcpy(encoded + 2, &last, sizeof(__int8_t));
    encoded[3] = '|';
    memcpy(encoded + 4, box_name, 32);
    encoded[36] = '|';
    memcpy(encoded + 37, box_size, sizeof(__int64_t));
    encoded[45] = '|';
    memcpy(encoded + 46, n_publishers, sizeof(__int64_t));
    encoded[54] = '|';
    memcpy(encoded + 55, n_subscribers, sizeof(__int64_t));

    return 0;
}

/*
 * Create an encoded string used for a request for sending a message.
 * The encoded string should have this format:
 * [ code = 9 (uint8_t) ] | [ message (char[1024]) ]
 * 
 * Input:
 *   - message: the message to send
 *   - encoded: a pointer to the string where the encoding will be made
 *   - encoded_len: the size of the previous string
 */
int prot_encode_pub_send_message(char message[1024], char* encoded, int encoded_len) {
    if (encoded_len < 1026)
        return -1;
    memset(encoded, 0, encoded_len);

    __int8_t code = CODE_PUB_SEND_MESSAGE;
    memcpy(encoded, &code, sizeof(__int8_t));
    encoded[1] = '|';
    memcpy(encoded + 2, message, 1024);

    return 0;
}

/*
 * Create an encoded string used for a request for sending a message.
 * The encoded string should have this format:
 * [ code = 10 (uint8_t) ] | [ message (char[1024]) ]
 * 
 * Input:
 *   - message: the message to send
 *   - encoded: a pointer to the string where the encoding will be made
 *   - encoded_len: the size of the previous string
 */
int prot_encode_sub_receive_message(char message[1024], char* encoded, int encoded_len) {
    if (encoded_len < 1026)
        return -1;
    memset(encoded, 0, encoded_len);

    __int8_t code = CODE_SUB_RECEIVE_MESSAGE;
    memcpy(encoded, &code, sizeof(__int8_t));
    encoded[1] = '|';
    memcpy(encoded + 2, message, 1024);

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
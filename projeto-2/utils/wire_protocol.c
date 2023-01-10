#include "wire_protocol.h"

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

void print_encoded(char* encoded, size_t len) {
    printf("encoded = \"");

    for(int i = 0; i < len; i++) {
        if (encoded[i] == '\0')
            fputs("\\0", stdout);
        else {
            char c = encoded[i];
            fputs(&c, stdout);
        }
    }

    printf("\"");
}

int prot_aux_encode_registrations(__int8_t code, char pipe_path[256], char box_name[32], char* encoded, size_t encoded_len) 
{
    if(encoded_len < 291)
        return -1;
    memset(encoded, 0, encoded_len);

    memcpy(encoded, &code, sizeof(__int8_t));
    encoded[1] = '|';
    memcpy(encoded + 2, pipe_path, 256*sizeof(char));
    encoded[258] = '|';
    memcpy(encoded + 259, box_name, 32*sizeof(char));

    return 0;
}



int prot_aux_encode_inbox_response(__int8_t code, __int32_t return_code, char error_message[1024], char* encoded, size_t encoded_len) 
{
    if(encoded_len < 1030)
        return -1;
    memset(encoded, 0, encoded_len);

    memcpy(encoded, &code, sizeof(__int8_t));
    encoded[1] = '|';    
    memcpy(encoded + 2, &return_code, sizeof(__int32_t));  // Maximum length of a 32-bit signed int is 11. 
    encoded[6] = '|';
    memcpy(encoded + 7, error_message, 1024*sizeof(char));
    return 0;
}

int prot_aux_encode_message(__int8_t code, char message[1024], char* encoded, size_t encoded_len) {
    if (encoded_len < 1026)
        return -1;
    memset(encoded, 0, encoded_len);

    memcpy(encoded, &code, sizeof(__int8_t));
    encoded[1] = '|';
    memcpy(encoded + 2, message, 1024*sizeof(char));

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
int prot_encode_pub_registration(char pipe_path[256], char box_name[32], char* encoded, size_t encoded_len) {
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
int prot_encode_sub_registration(char pipe_path[256], char box_name[32], char* encoded, size_t encoded_len) {
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
int prot_encode_inbox_creation_req(char pipe_path[256], char box_name[32], char* encoded, size_t encoded_len) {
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
int prot_encode_inbox_creation_resp(__int32_t return_code, char error_message[1024], char* encoded, size_t encoded_len) {
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
int prot_encode_inbox_removal_req(char pipe_path[256], char box_name[32], char* encoded, size_t encoded_len) {
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
int prot_encode_inbox_removal_resp(__int32_t return_code, char error_message[1024], char* encoded, size_t encoded_len) {
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
int prot_encode_inbox_listing_req(char pipe_path[256], char* encoded, size_t encoded_len) {
    if(encoded_len < 258)
        return -1;
    memset(encoded, 0, encoded_len);

    __int8_t code = (__int8_t) CODE_INBOX_LIST_REQ;
    memcpy(encoded, &code, sizeof(__int8_t));
    encoded[1] = '|';
    memcpy(encoded + 2, pipe_path, 256*sizeof(char));

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
int prot_encode_inbox_listing_resp(__int8_t last, char box_name[32], __int64_t box_size, __int64_t n_publishers, __int64_t n_subscribers, char* encoded, size_t encoded_len) 
{
    if(encoded_len < 63)
        return -1;
    memset(encoded, 0, encoded_len);
    
    __int8_t code = (__int8_t) CODE_INBOX_LIST_RESP;
    memcpy(encoded, &code, sizeof(__int8_t));
    encoded[1] = '|';
    memcpy(encoded + 2, &last, sizeof(__int8_t));
    encoded[3] = '|';
    memcpy(encoded + 4, box_name, 32*sizeof(char));
    encoded[36] = '|';
    memcpy(encoded + 37, &box_size, sizeof(__int64_t));
    encoded[45] = '|';
    memcpy(encoded + 46, &n_publishers, sizeof(__int64_t));
    encoded[54] = '|';
    memcpy(encoded + 55, &n_subscribers, sizeof(__int64_t));

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
int prot_encode_pub_send_message(char message[1024], char* encoded, size_t encoded_len) {
    return prot_aux_encode_message(CODE_PUB_SEND_MESSAGE, message, encoded, encoded_len);
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
int prot_encode_sub_receive_message(char message[1024], char* encoded, size_t encoded_len) {
    return prot_aux_encode_message(CODE_SUB_RECEIVE_MESSAGE, message, encoded, encoded_len);
}

/*
 * Returns the number of bytes to read after the request code:
 *
 * Input:
 *   - code_encoded: the byte related to the request code
 */
int protocol_decode_request_n_bytes(char code_encoded) {
    __int8_t code;
    memcpy(&code, &code_encoded, sizeof(__int8_t));

    switch (code) {
        case 1:
        case 2:
        case 3:
        case 5:
            return 290;
            break;
        case 4:
        case 6:
            return 1029;
            break;
        case 7:
            return 257;
            break;
        case 8:
            return 62;
            break;
        case 9:
        case 10:
            return 1025;
            break;
        default:
            return -1;
    }
}

/*
 * Decodes requests related to codes 1, 2, 3, 5 and 7.
 * 
 * Input:
 *   - pipe_path: the string where the name of the pipe will be written
 *   - box_name: the string where the name of the inbox will be written
 *   - encoded: a pointer to the string that contains the encoded message
 *   - encoded_len: the size of the encoded message
 */
int prot_decode_registrations(char pipe_path[256], char box_name[32], char* encoded, size_t encoded_len) {
    if(encoded_len < 290)
        return -1;

    memcpy(pipe_path, encoded + 2*sizeof(char), 256*sizeof(char));
    memcpy(box_name, encoded + 259*sizeof(char), 32*sizeof(char));
    return 0;
}

/*
 * Decodes messages related to codes 4 and 6.
 * 
 * Input:
 *   - return_code: a pointer to the integer where the return code will be written
 *   - error_message: the string where the error_message will be written
 *   - encoded: a pointer to the string that contains the encoded message
 *   - encoded_len: the size of the encoded message
 */
int prot_decode_inbox_response(__int32_t* return_code, char error_message[1024], char* encoded, size_t encoded_len) 
{
    if(encoded_len < 1029)
        return -1;

    memcpy(return_code, encoded + 2, sizeof(__int32_t));
    memcpy(error_message, encoded + 7, 1024*sizeof(char));
    return 0;
}


/*
 * Decodes messages related to code 7.
 * 
 * Input:
 *   - pipe_path: the string where the name of the pipe will be written
 *   - encoded: a pointer to the string that contains the encoded message
 *   - encoded_len: the size of the encoded message
 */
int prot_decode_inbox_listing_req(char pipe_path[256], char* encoded, size_t encoded_len) {
    if(encoded_len < 258)
        return -1;

    memcpy(pipe_path, encoded + 2, 256*sizeof(char));

    return 0;
}

/*
 * Decodes messages related to code 8.
 * 
 * Input:
 *   - last: points to the address to store the integer that decodes whether it's the last message or not
 *   - error_message: the string to where the inbox name will be written
 *   - box_size, n_publishers, n_subscribers: the addresses that will store some of the stats of a particular inbox
 *   - encoded: a pointer to the string that contains the encoded message
 *   - encoded_len: the size of the encoded message
 */
int prot_decode_inbox_listing_resp(__int8_t* last, char box_name[32], __int64_t* box_size, __int64_t* n_publishers, __int64_t* n_subscribers, char* encoded, size_t encoded_len) 
{
    if(encoded_len < 63)
        return -1;
    
    memcpy(last, encoded + 2, sizeof(__int8_t));
    memcpy(box_name, encoded + 4, 32*sizeof(char));
    memcpy(box_size, encoded + 37, sizeof(__int64_t));
    memcpy(n_publishers, encoded + 46, sizeof(__int64_t));
    memcpy(n_subscribers, encoded + 55, sizeof(__int64_t));

    return 0;
}

/*
 * Decodes messages related to codes 9 and 10.
 * 
 * Input:
 *   - message: the string where the inbox message will be written
 *   - encoded: a pointer to the string that contains the encoded message
 *   - encoded_len: the size of the encoded message
 */
int prot_decode_message(char message[1024], char* encoded, size_t encoded_len) {
    if (encoded_len < 1026)
        return -1;
    memset(encoded, 0, encoded_len);

    memcpy(message, encoded+1, 1024*sizeof(char));

    return 0;
}
/*
int main() {
    char pipe_path[1024];
    memset(pipe_path, 0, 1024);
    strcpy(pipe_path, "pipe_name");

    __int64_t integer = 345466846546153;
    __int8_t d = 4;

    char aaa[1031];
    memset(aaa, 0, 1030);
    prot_encode_inbox_listing_resp(d, pipe_path, integer, integer, integer, aaa, sizeof(aaa));
    print_encoded(aaa, sizeof(aaa));

    printf("bytes: %d\n", protocol_decode_request_n_bytes(aaa[0]));

    char new_box_name[1030];
    __int64_t newInt1;
    __int64_t newInt2;
    __int64_t newInt3;
    __int8_t nn;
    prot_decode_inbox_listing_resp(&nn, new_box_name, &newInt1, &newInt2, &newInt3, aaa + 1, sizeof(aaa));

    printf("l: %d, box: %s, a: %ld, b: %ld, c: %ld\n", nn, new_box_name, newInt1, newInt2, newInt3);


    if (aaa[0] == 1) {
        aaa[0] = '1';
    }
    return 0;
}
*/
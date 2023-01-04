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

char* prot_encode_pub_registration(char pipe_path[256], char box_name[32]) {
    char encoded[256+32+3];

    strcpy(encoded, CODE_PUB_REGISTRATION);

    return CODE_PUB_REGISTRATION + "|" + pipe_path + "|" + 
}
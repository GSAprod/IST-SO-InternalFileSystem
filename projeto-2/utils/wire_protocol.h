#ifndef __UTILS_PROTOCOL_H__
#define __UTILS_PROTOCOL_H__

#include "wire_protocol.c"

int prot_aux_encode_registrations(int code, char pipe_path[256], char box_name[32], 
        char* encoded, int encoded_len);
int prot_aux_encode_inbox_response(int code, __int32_t return_code, char error_message[1024], 
        char* encoded, int encoded_len);

int prot_encode_pub_registration(char pipe_path[256], char box_name[32], char* encoded, int encoded_len);
int prot_encode_sub_registration(char pipe_path[256], char box_name[32], char* encoded, int encoded_len);
int prot_encode_inbox_creation_req(char pipe_path[256], char box_name[32], char* encoded, int encoded_len);
int prot_encode_inbox_creation_resp(__int32_t return_code, char error_message[1024], char* encoded, int encoded_len);
int prot_encode_inbox_removal_req(char pipe_path[256], char box_name[32], char* encoded, int encoded_len);
int prot_encode_inbox_removal_resp(__int32_t return_code, char error_message[1024], char* encoded, int encoded_len);
int prot_encode_inbox_listing_req(char pipe_path[256], char* encoded, int encoded_len);
int prot_encode_inbox_listing_resp(__int8_t last, char box_name[32], __int64_t box_size, char* encoded, int encoded_len);
int prot_encode_pub_send_message(char message[1024], char* encoded, int encoded_len);
int prot_encode_sub_receive_message(char message[1024], char* encoded, int encoded_len);

#endif
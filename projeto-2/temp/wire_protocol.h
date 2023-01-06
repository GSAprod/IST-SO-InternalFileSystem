#ifndef __UTILS_PROTOCOL_H__
#define __UTILS_PROTOCOL_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int prot_aux_encode_registrations(__int8_t code, char pipe_path[256], char box_name[32], char* encoded, size_t encoded_len);
int prot_aux_encode_inbox_response(__int8_t code, __int32_t return_code, char error_message[1024], char* encoded, size_t encoded_len);

int prot_encode_pub_registration(char pipe_path[256], char box_name[32], char* encoded, size_t encoded_len);
int prot_encode_sub_registration(char pipe_path[256], char box_name[32], char* encoded, size_t encoded_len);
int prot_encode_inbox_creation_req(char pipe_path[256], char box_name[32], char* encoded, size_t encoded_len);
int prot_encode_inbox_creation_resp(__int32_t return_code, char error_message[1024], char* encoded, size_t encoded_len);
int prot_encode_inbox_removal_req(char pipe_path[256], char box_name[32], char* encoded, size_t encoded_len);
int prot_encode_inbox_removal_resp(__int32_t return_code, char error_message[1024], char* encoded, size_t encoded_len);
int prot_encode_inbox_listing_req(char pipe_path[256], char* encoded, size_t encoded_len);
int prot_encode_inbox_listing_resp(__int8_t last, char box_name[32], __int64_t box_size, __int64_t n_publishers, __int64_t n_subscribers, char* encoded, size_t encoded_len);
int prot_encode_pub_send_message(char message[1024], char* encoded, size_t encoded_len);
int prot_encode_sub_receive_message(char message[1024], char* encoded, size_t encoded_len);

int main();
#endif
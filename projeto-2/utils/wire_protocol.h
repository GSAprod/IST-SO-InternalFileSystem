#ifndef __UTILS_PROTOCOL_H__
#define __UTILS_PROTOCOL_H__

#include "wire_protocol.c"

int prot_encode_pub_registration(char pipe_path[256], char box_name[32], char* encoded, int encoded_len);

#endif
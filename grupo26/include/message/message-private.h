#ifndef _MESSAGE_PRIVATE_H
#define _MESSAGE_PRIVATE_H

#include "inet.h"
#include "sdmessage.pb-c.h"

struct message_t {
    MessageT *message;
    /*
    union content_u {
        struct data_t *value;
	    struct entry_t *entry;
        char *key;
        char **keys;
        int result;
    } content;   
    */ 
};


int write_all(int sock, void *buf, int len);

int read_all(int sock, void *buf, int len);

int send_all(int sockfd, MessageT *message);

MessageT *receive_all(int sockfd);

#endif
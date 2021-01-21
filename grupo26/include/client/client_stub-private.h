#ifndef _CLIENT_STUB_PRIVATE_H
#define _CLIENT_STUB_PRIVATE_H

#include "client_stub.h"

/* Remote tree. A definir pelo grupo em client_stub-private.h
 */
struct rtree_t {
    int sockfd;
    struct sockaddr_in *server;
    char* address;
};

#endif

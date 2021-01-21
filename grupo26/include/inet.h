#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <poll.h>
#include <pthread.h>
#include <netdb.h>

//tamanho máximo da mensagem enviada pelo cliente
#define MAX_MSG 2048

#define NFDESC 10
#define TIMEOUT 5000
#define MAX_IP 1024
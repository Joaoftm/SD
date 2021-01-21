
#include "message/message-private.h"

struct message_t;

int write_all(int sock, void *buf, int len) {
    int bufsize = len;
    while(len > 0) {
        int res = write(sock, buf, len);
        buf += res;
        len -= res;
    }
    return bufsize;
}

int read_all(int sock, void *buf, int len) {
    int bufsize = len;
    while(len > 0) {
        int res = read(sock, buf, len);
        buf += res;
        len -= res;
    }
    return bufsize;
}

void free_message(MessageT *msg) {
    message_t__free_unpacked(msg,NULL);
}

int send_all(int sockfd, MessageT *message){

    if (sockfd <= 0 || message == NULL)
        return -1;

    int message_size = message_t__get_packed_size(message);
    void *buf = malloc(message_size);
    int buf_size = message_t__pack(message,buf);

    if (write_all(sockfd,&buf_size,sizeof(int)) != sizeof(int)) {             
        perror("Message:\n\tError in message size.\n");
        close(sockfd);
        free(buf);
        return -1;
    }

    if (write_all(sockfd,buf,buf_size) != buf_size) {           
        perror("Message:\n\tError in sending data.\n");
        close(sockfd);
        free(buf);
        return -1;
    }

    free(buf);
    return buf_size;
}

MessageT *receive_all(int sockfd){

    if(sockfd < 0) return NULL;

    int buf_size = -1;
    
    if (read_all(sockfd,&buf_size,sizeof(int)) != sizeof(int) ||
        buf_size == -1) {
        printf("Message:\n\tError reading message.\n");
        return NULL;
    }

    void *buf = malloc(buf_size);

    if (read_all(sockfd,buf,buf_size) != buf_size) {
        perror("Message:\n\tError reading data.\n");
        free(buf);
        return NULL;
    }

    MessageT *message = message_t__unpack(NULL,buf_size,buf);
    free(buf);

    if (message == NULL) {
        printf("Message:\n\tError unpacking message.\n");
        return NULL;
    }

    return message;
}

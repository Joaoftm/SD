
#include "inet.h"
#include "client/client_stub-private.h"
#include "client/network_client.h"
#include "message/message-private.h"

int network_connect(struct rtree_t *rtree) {

     if (rtree == NULL) {
        printf("Network_Client:\n\tError in tree client.\n");
        return -1;
    }

    if ((connect(rtree->sockfd, (struct sockaddr*) rtree->server, sizeof(struct sockaddr_in)) < 0)) {
        perror("Network_Client:\n\tConnection error. ");
        printf("\tSocket: %d | Server address: %p | Size Server: %ld\n",rtree->sockfd,(struct sockaddr*) rtree->server,sizeof(struct sockaddr_in));
        close(rtree->sockfd);
        return -1;
    }
    
    return 0;
}


struct message_t *network_send_receive(struct rtree_t * rtree,
                                       struct message_t *msg) {
    if (rtree == NULL || rtree->sockfd < 0 ||
        msg == NULL || msg->message == NULL)
        return NULL;
    
    int sockfd = rtree->sockfd;
    printf("\n\n- Sending message to server.\n");

    if (send_all(sockfd, msg->message) == -1) {
        printf("Network_Client:\n\tError sending message.\n");
        return NULL;
    }
    printf("+ Message sent.\n\n");

    printf("- Receiving message in server.\n");

    if ((msg->message = receive_all(sockfd)) == NULL) {
        perror("Network_Client:\n\tError receiving message.\n");
        close(rtree->sockfd);
        return NULL;
    }
    printf("+ Message received.\n\n");
    
    return msg;
}

int network_close(struct rtree_t *rtree) {

    if (rtree == NULL)
        return -1;

    close(rtree->sockfd);
    return 0;
}

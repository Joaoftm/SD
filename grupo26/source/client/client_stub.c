

#include "message/message-private.h"
#include "client/client_stub-private.h"
#include "client/network_client.h"
#include "inet.h"

struct rtree_t;


void delete_message(struct message_t *msg) {
    free(msg);
}

struct rtree_t *rtree_connect(const char *address_port){
    if (address_port == NULL)
        return NULL;

    char *same_port = strdup(address_port);

    char delimitator[sizeof(":")] = ":";
    char *hostname = strdup(strtok(same_port,delimitator));
    int port = atoi(strtok(NULL,delimitator));
    
    free(same_port);

    if (hostname == NULL || port <= 0) {
        printf("Client_Stub:\n\tError in server address\n");
        free(hostname);
        return NULL;
    }
    
    printf("\t\nTree connection :: Hostname:\t%s | Port:\t%d\n ",hostname,port);
    
    struct rtree_t *tree = malloc(sizeof(struct rtree_t));
    tree->address = strdup(address_port);

    if (tree == NULL) {
        free(hostname);
        return NULL;
    }

    tree->server = malloc(sizeof(struct sockaddr_in));

    if (tree->server == NULL) {
        free(hostname);
        free(tree);
        return NULL;
    }
    if ((tree->sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("Client Stub:\n\tError creating TCP socket.\n");
        free(hostname);
        free(tree);
        return NULL;
    }

    tree->server->sin_family = AF_INET;
    tree->server->sin_port = htons(port);

    if (inet_pton(AF_INET, hostname, &tree->server->sin_addr) < 1) {
        printf("Client Stub:\n\tError converting IP.\n");
        rtree_disconnect(tree);
        free(hostname);
        return NULL;
    }

    if (network_connect(tree) == -1) {
        printf("Client Stub:\n\tError connecting to server.\n");
        rtree_disconnect(tree);
        free(hostname);
        return NULL;
    }

    free(hostname);
    return tree;
}

int rtree_disconnect(struct rtree_t *rtree) {

    if (rtree == NULL)
        return -1;
    else if (network_close(rtree) != 0) {
        free(rtree);
        return -1;
    }

    free(rtree);
    return 0;
}

int rtree_put(struct rtree_t *rtree, struct entry_t *entry) {

    if (rtree == NULL || rtree->server == NULL ||
        entry == NULL || entry->key == NULL || entry->value == NULL)
        return -1;

    struct message_t *msg = malloc(sizeof(struct message_t));
    if (msg == NULL)
        return -1;

    MessageT msgt; 
    message_t__init(&msgt);
    msg->message = &msgt;
    
    msg->message->opcode = MESSAGE_T__OPCODE__OP_PUT;
    msg->message->c_type = MESSAGE_T__C_TYPE__CT_ENTRY;

    int keysize = sizeof(char)*(strlen(entry->key)+1);
    int datasize = entry->value->datasize;

    if (keysize <= 0|| datasize <= 0)
        return -1;

    msg->message->data_size = datasize;
    msg->message->data = entry->value->data;
    msg->message->key = entry->key;
    

    struct message_t *r_msg = network_send_receive(rtree,msg);

    int error = r_msg->message->opcode == MESSAGE_T__OPCODE__OP_ERROR &&
                r_msg->message->c_type == MESSAGE_T__C_TYPE__CT_NONE ? -1 : 0;

    delete_message(msg);

    return error;
}

struct data_t *rtree_get(struct rtree_t *rtree, char *key) {

    if(rtree == NULL || rtree->server == NULL || key == NULL)
        return NULL;

    struct message_t *msg = malloc(sizeof(struct message_t));
    if (msg == NULL)
        return NULL;

    MessageT msgt; 
    message_t__init(&msgt);
    msg->message = &msgt;

    if (msg->message == NULL)
        return NULL;

    msg->message->opcode = MESSAGE_T__OPCODE__OP_GET;
    msg->message->c_type = MESSAGE_T__C_TYPE__CT_KEY;
    msg->message->data_size = 0;
    msg->message->data = key;

    struct message_t *r_msg = network_send_receive(rtree,msg);

    if(r_msg->message->opcode == MESSAGE_T__OPCODE__OP_ERROR &&
        r_msg->message->c_type == MESSAGE_T__C_TYPE__CT_NONE) {
        delete_message(msg);
        return NULL;
    }
    
    int data_size = r_msg->message->data_size;
    void *r_data = r_msg->message->data;
    
    struct data_t *data = data_create2(data_size, r_data);

    delete_message(msg);

    return data;
}

int rtree_del(struct rtree_t *rtree, char *key) {

    if (rtree == NULL || rtree->server == NULL ||
        key == NULL)
        return -1;

    struct message_t *msg = malloc(sizeof(struct message_t));
    if (msg == NULL)
        return -1;

    MessageT msgt; 
    message_t__init(&msgt);
    msg->message = &msgt;

    if (msg->message == NULL)
        return -1;

    msg->message->opcode = MESSAGE_T__OPCODE__OP_DEL;
    msg->message->c_type = MESSAGE_T__C_TYPE__CT_KEY;
    msg->message->data_size = 0;
    msg->message->data = key;

    struct message_t *r_msg = network_send_receive(rtree,msg);

    int error =
            r_msg->message->opcode == MESSAGE_T__OPCODE__OP_DEL &&
            r_msg->message->c_type == MESSAGE_T__C_TYPE__CT_NONE ? 1 : 
            (r_msg->message->opcode == MESSAGE_T__OPCODE__OP_DEL+1 &&
            r_msg->message->c_type == MESSAGE_T__C_TYPE__CT_NONE ? 0 : -1);

    delete_message(msg);

    return error;
}


int rtree_size(struct rtree_t *rtree) {

    if (rtree == NULL || rtree->server == NULL)
        return -1;

    struct message_t *msg = malloc(sizeof(struct message_t));
    if (msg == NULL)
        return -1;

    MessageT msgt; 
    message_t__init(&msgt);
    msg->message = &msgt;

    if(msg->message == NULL)
        return -1;

    msg->message->opcode = MESSAGE_T__OPCODE__OP_SIZE;
    msg->message->c_type = MESSAGE_T__C_TYPE__CT_NONE;
    msg->message->data_size = 0;
    
    struct message_t *r_msg  = network_send_receive(rtree,msg);

    int size = -1;
    if (r_msg == NULL || r_msg->message->data_size < 0) {
        msg->message->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->message->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return -1;
    }

    size = r_msg->message->data_size;
    delete_message(msg);

    return size;
}

int rtree_height(struct rtree_t *rtree) {

    if (rtree == NULL || rtree->server == NULL)
        return -1;

    struct message_t *msg = malloc(sizeof(struct message_t));
    if (msg == NULL)
        return -1;

    MessageT msgt; 
    message_t__init(&msgt);
    msg->message = &msgt;

    if(msg->message == NULL)
        return -1;

    msg->message->opcode = MESSAGE_T__OPCODE__OP_HEIGHT;
    msg->message->c_type = MESSAGE_T__C_TYPE__CT_NONE;
    msg->message->data_size = 0;

    struct message_t *r_msg  = network_send_receive(rtree,msg);

    int size = -1;
    if (r_msg == NULL || r_msg->message->data_size < 0) {
        msg->message->opcode = MESSAGE_T__OPCODE__OP_ERROR;
        msg->message->c_type = MESSAGE_T__C_TYPE__CT_NONE;
        return -1;
    }

    size = r_msg->message->data_size;
    delete_message(msg);

    return size;
}

char **rtree_get_keys(struct rtree_t *rtree) {

    if (rtree == NULL || rtree->server == NULL)
        return NULL;

    struct message_t *msg = malloc(sizeof(struct message_t));
    if (msg == NULL)
        return NULL;

    MessageT msgt; 
    message_t__init(&msgt);
    msg->message = &msgt;

    if (msg->message == NULL)
        return NULL;

    msg->message->opcode = MESSAGE_T__OPCODE__OP_GETKEYS;
    msg->message->c_type = MESSAGE_T__C_TYPE__CT_NONE;
    msg->message->data_size = 0;
    msg->message->keys_size = 0;

    struct message_t *r_msg = network_send_receive(rtree,msg);
    
    if (r_msg == NULL)
        return NULL;
    
    int tree_size = r_msg->message->n_arr_keys;
    
    if (tree_size <= 0)
        return NULL;
    
    char **keys = malloc(tree_size*sizeof(char*)+sizeof(NULL));
    for (int i = 0; i < tree_size; i++)
        keys[i] = strdup(r_msg->message->arr_keys[i]);

    keys[tree_size] = NULL;

    delete_message(msg);

    return keys;
}

void rtree_free_keys(char **keys) {

    if (keys == NULL)
        return;

    int i = 0;
    while (keys[i] != NULL)
        free(keys[i++]);

    free(keys);
}


int rtree_verify(struct rtree_t *rtree, int op_n) {
    if (rtree == NULL || rtree->server == NULL)
        return -1;

    struct message_t *msg = malloc(sizeof(struct message_t));
    if (msg == NULL)
        return -1;

    MessageT msgt; 
    message_t__init(&msgt);
    msg->message = &msgt;

    if (msg->message == NULL)
        return -1;

    msg->message->opcode = MESSAGE_T__OPCODE__OP_VERIFY;
    msg->message->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
    msg->message->op = op_n;
    msg->message->data_size = 0;
    msg->message->keys_size = 0;

    struct message_t *r_msg = network_send_receive(rtree,msg);

    int error =
            r_msg->message->opcode == MESSAGE_T__OPCODE__OP_VERIFY+1 &&
            r_msg->message->c_type == MESSAGE_T__C_TYPE__CT_RESULT ? 0 : 
            (r_msg->message->opcode == MESSAGE_T__OPCODE__OP_ERROR &&
            r_msg->message->c_type == MESSAGE_T__C_TYPE__CT_NONE ? 1 : -1);

    int op = r_msg->message->op;

    delete_message(msg);

    if (error != 0)
        return error;
    
    return op;
}

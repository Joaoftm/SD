
#include "inet.h"
#include "message/message-private.h"
#include "server/tree_skel.h"
#include "tree.h"
#include "tree-private.h"
#include "zookeeper/zookeeper.h"
#include "client/client_stub.h"

// // // // Zookeper // // // //
#define ZDATALEN 1024 * 1024
typedef struct String_vector zoo_string; 
//
static char *root_path = "/kvstore";
static char *primary_path = "/kvstore/primary";
static char *backup_path = "/kvstore/backup";
char* id;
static zhandle_t *zh;
static int is_connected;
static char *watcher_ctx = "ZooKeeper Data Watcher";
// CONNECTION TO NEXT SERVER
struct rtree_t* new_server;
//
void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context);
static void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx);
// // // // // // // // // // //

struct tree_t *tree = NULL;

int last_assigned = 0;
int op_count = 0;
int terminated = 0;

struct task_t *queue_head = NULL;
pthread_mutex_t queue_lock, tree_lock;
pthread_cond_t queue_not_empty;
pthread_t process_thread;

int tree_skel_init(const char *address, const char* zookeeper_addr, const char* port){
    // obter IP deste servidor
    char hostbuffer[256]; 
    char *IPbuffer; 
    struct hostent *host_entry; 
    int hostname; 

    hostname = gethostname(hostbuffer, sizeof(hostbuffer)); 
    if (hostname == -1) { 
        return -1; 
    } 

    host_entry = gethostbyname(hostbuffer); 
    if (host_entry == NULL) { 
        return -1;
    } 

    IPbuffer = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0])); 

    // concatenar ip com port do servidor
    char *ip_port = IPbuffer; 
    strcat(ip_port, ":");
    strcat(ip_port, port);
    printf("\n%s searching .....\n",ip_port);
    tree = tree_create();
    
    if(tree == NULL){
        return -1;
    }

    int error = 0;
    error += pthread_mutex_init(&queue_lock,NULL);
    error += pthread_mutex_init(&tree_lock,NULL);
    error += pthread_cond_init(&queue_not_empty,NULL);

    if (error != 0) {
        printf("Tree Skel:\n\tErro a inicializar locks.\n");
        return -1;
    }

    error = pthread_create(&process_thread,NULL,process_task,NULL);
    if (error != 0) {
        printf("tree Skel:\n\tErro a criar thread: Erro %d.\n",error);
        return -1;
    }

    // CONNECTING TO ZOOKEEPER
	zh = zookeeper_init(ip_port, connection_watcher, 2000, 0, NULL, 0);
    if (zh == NULL)	{
		fprintf(stderr, "Error connecting to ZooKeeper server!\n");
	    exit(EXIT_FAILURE);
	}
    printf("\nGoing to connection...\n");
    sleep(3); /* Sleep a little for connection to complete */

    if (is_connected) {

        printf("\nis Connected!\n");  

        // Se nao existir nó pai, cria
        if (ZNONODE == zoo_exists(zh, root_path, 0, NULL)) {
			fprintf(stderr, "%s doesn't exist! Creating ZNode.\n", root_path);
            
            // CRIA ZNODE NORMAL /kvstore
            if (ZOK != zoo_create(zh, root_path, NULL, -1, & ZOO_OPEN_ACL_UNSAFE, 0, NULL, 0)) {
				fprintf(stderr, "Error creating znode from path %s!\n", root_path);
			    	exit(EXIT_FAILURE);
			}
			
        }
        else{
            fprintf(stderr, "%s ZNode is online.\n", root_path);
        }
        

        // WATCH 	int zoo_data_len = ZDATALEN;
        zoo_string *children_list =	(zoo_string *) malloc(sizeof(zoo_string));

        if (ZOK != zoo_wget_children(zh, root_path, &child_watcher, watcher_ctx, children_list)) {
            fprintf(stderr, "Error setting watch at %s!\n", root_path);
            exit(EXIT_FAILURE);
		}
        else{
            for (int i = 0; i < children_list->count; i++)  {
				fprintf(stderr, "\n(%d): %s\n", i+1, children_list->data[i]);
			}
        }
        if (ZNONODE == zoo_exists(zh, primary_path, 0, NULL)){
            //todo bloquear comandos
        }

        //Criar server name
        char this_server[MAX_IP];
        strcpy(this_server,zookeeper_addr);
        strcat(this_server,":");
        strcat(this_server,address);

        printf("Connected to server : %s\n",this_server);
        
        int size = (strlen(this_server)+1) * sizeof(char);

        // CRIA ZNODE EFEMERO /primary    
        id = malloc(MAX_IP);                          //id zookeper
        // Se nao existir nó primary, cria
        if (ZOK != zoo_create(zh, primary_path, this_server, size, & ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, id, MAX_IP)) {
            fprintf(stderr, "\n%s ZNode is online.\n", primary_path);
        }
        else{
            fprintf(stderr, "\n%s doesn't exist! Creating ZNode\n", primary_path);
        }
        //apenas qd existe necessidade de um backup
        if(children_list->count == 1){
          
            //Cria nó Backup
            // Se nao existir nó backup, cria
            if (ZOK != zoo_create(zh, backup_path, this_server, size, & ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, id, MAX_IP)) {
                fprintf(stderr, "%s ZNode is online.\n", backup_path);
            }
            else{
                fprintf(stderr, "%s doesn't exist! Creating ZNode.\n", backup_path);

            }
        }
        if(children_list->count == 0){
            new_server = NULL;
            fprintf(stderr, "\n\t WARNING :: no backup created \t\n");
        }
    
    }
    return 0;
}

void tree_skel_destroy(){
    
    if (terminated == 0) {
        terminated = 1;
        pthread_cond_signal(&queue_not_empty);

        int error = 0;
        if ((error = pthread_join(process_thread,NULL)) != 0)
            printf("\tError joining Thread: Error %d.\n",error);
        else
            printf("\tThread joined.\n");
        
        tree_destroy(tree);
        rtree_disconnect(new_server);
        free(id); 
    }
    
}

int invoke(struct message_t *msg) {
  
    if(msg == NULL || msg->message == NULL || tree == NULL)
        return -1;

    char *key;
    struct data_t *data;

    MessageT *message = msg->message;
    struct task_t *task ;

    switch(message->opcode){
        case MESSAGE_T__OPCODE__OP_SIZE:
            if (message == NULL)
                return -1;

            message->opcode = MESSAGE_T__OPCODE__OP_SIZE + 1;
            message->c_type = MESSAGE_T__C_TYPE__CT_RESULT;
                        
            pthread_mutex_lock(&tree_lock);
            int treesize = tree_size(tree);
            pthread_mutex_unlock(&tree_lock);

            if (treesize < 0)
                return -1;

            message->data_size = treesize;
            return 0;

        case MESSAGE_T__OPCODE__OP_DEL:
            task =  task_create(last_assigned,
                                                        1,
                                                        
                                                        message->key,
                                                        message->data,
                                                        message->data_size);
            /*-------------------------------------------------------------------------------*/
            /*----------------------ver se nao foi possivel criar task---------------------*/
            if(task == NULL){
                message->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                message->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            
                return -1;
            }
            if(tree == NULL || msg == NULL || message->data == NULL) {
                    message->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                    message->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                    return -1;
            }

            key = message->data;
            if (tree_del(tree,key) == -1) {
                message->opcode = MESSAGE_T__OPCODE__OP_DEL;
                message->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }

            message->opcode = MESSAGE_T__OPCODE__OP_DEL+1;
            message->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return 0;        

        case MESSAGE_T__OPCODE__OP_GET:

            if (tree == NULL || msg == NULL || message->data == NULL) {
                    message->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                    message->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                    return -1;
            }

            key = message->data;

           data = tree_get(tree,key);

            if (data == NULL) {
                message->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                message->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                message->data_size = 0;
                message->data = 0;
                return 0;
            }

            message->data_size = data->datasize;
            message->data = data->data;
            
            message->opcode = MESSAGE_T__OPCODE__OP_GET+1;
            message->c_type = MESSAGE_T__C_TYPE__CT_VALUE;
            return 0;

        case MESSAGE_T__OPCODE__OP_PUT: ;

            last_assigned++; ;

            /*--------------------------------criar task-----------------------------------*/
            task =  task_create(last_assigned, 1, message->key, message->data, message->data_size);
            /*-------------------------------------------------------------------------------*/
            /*----------------------ver se nao foi possivel criar task---------------------*/
            if(task == NULL){
                message->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                message->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            
                return -1;
            }

            /*adicionar task ah lista e sinaliza ah variavel de condicao que foi adicionado algo ah lista*/
            queue_add_task(task);     

            if (tree == NULL || msg == NULL ||
                message->data_size <= 0 || message->data == NULL) {

                message->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                message->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }
            
            key = message->key;
            void *value = malloc(message->data_size);
            memcpy(value,message->data,message->data_size);
            data = data_create2(message->data_size,value);

            if(tree_put(tree,key,data) == 0){
                message->opcode = MESSAGE_T__OPCODE__OP_PUT+1;
                message->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                data_destroy(data);
                return 0;
            }

            data_destroy(data);
            message->opcode = MESSAGE_T__OPCODE__OP_ERROR;
            message->c_type = MESSAGE_T__C_TYPE__CT_NONE;
            return -1;

        case MESSAGE_T__OPCODE__OP_GETKEYS:
            if (tree == NULL || msg == NULL ||
                message->c_type == MESSAGE_T__C_TYPE__CT_BAD) {
                message->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                message->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }

            pthread_mutex_lock(&tree_lock);
            int key_size = tree_size(tree);
            pthread_mutex_unlock(&tree_lock);
            message->n_arr_keys = key_size;

            if (key_size > 0) {
                pthread_mutex_lock(&tree_lock);
                msg->message->arr_keys = tree_get_keys(tree);
                pthread_mutex_unlock(&tree_lock);
                
                printf("%p / ",msg->message->arr_keys);
            } else if (key_size < 0) {
                message->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                message->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }

            message->opcode = MESSAGE_T__OPCODE__OP_GETKEYS + 1;
            message->c_type = MESSAGE_T__C_TYPE__CT_KEYS;
            return 0;

        case MESSAGE_T__OPCODE__OP_HEIGHT:

            if (msg == NULL)
                return -1;

            message->opcode = MESSAGE_T__OPCODE__OP_HEIGHT + 1;
            message->c_type = MESSAGE_T__C_TYPE__CT_RESULT;

            pthread_mutex_lock(&tree_lock);
            int treeheight = tree_height(tree);
            pthread_mutex_unlock(&tree_lock);

            if (treeheight < 0)
                return -1;

            message->data_size = treeheight;
            return 0;
        case MESSAGE_T__OPCODE__OP_VERIFY:
                    
            if (tree == NULL || msg == NULL ||
                message->c_type == MESSAGE_T__C_TYPE__CT_BAD) {
                message->opcode = MESSAGE_T__OPCODE__OP_ERROR;
                message->c_type = MESSAGE_T__C_TYPE__CT_NONE;
                return -1;
            }
            
            message->opcode = MESSAGE_T__OPCODE__OP_VERIFY+1;
            message->c_type = MESSAGE_T__C_TYPE__CT_RESULT;

            message->op = verify(message->op);

        default:
            printf("tree Skel:\n\tError receiving message.\n");
            return -1;
    }

    return -1;

}

void *process_task(void *params) {

	printf("\t thread inicializing.\n");
    
	while (terminated == 0) {

		struct task_t *task = queue_get_task();

        if (task == NULL) {
            printf("\tThread exit.\n");
            pthread_exit(0);
        }

		printf("  Thread in queue .\n");

        int error = 0;

        //mutex lock
        pthread_mutex_lock(&tree_lock);

        if (task->op == 0) {                     // DELETE
            error = tree_del(tree,task->key);

            // Tells new server what to do
            rtree_del(new_server,task->key);
            


        } else if (task->op == 1) {              // PUT
            struct data_t *data = data_create2(task->data_size,task->data);
            error = tree_put(tree,task->key,data);
            
            
            // Tells new server what to do
            int error2 = rtree_put(new_server,entry_create(strdup(task->key),data));
        
            data_destroy(data);
        }

        //mutex unlock
        pthread_mutex_unlock(&tree_lock);

        if (error == 0){
            op_count++;   
        }

        task_destroy(task);
	}

    printf("\tThread closing ...\n");
    pthread_exit(0);
    
}
//////////////////////////////////////////////////////////////////////////////////
//QUEUE
/*PRIVATE*/
void queue_add_task(struct task_t *task) {

    pthread_mutex_lock(&queue_lock);
    task->queue_next = NULL;

    if(queue_head == NULL)
        queue_head = task; 
        
    else {
        struct task_t *task_i = queue_head;
        while(task_i->queue_next != NULL)
            task_i = task_i->queue_next;
        
        task_i->queue_next = task;
    }
    //printf("\n%dFODASSE PUTAS\n",tree_size(tree));
    pthread_cond_signal(&queue_not_empty); 
    pthread_mutex_unlock(&queue_lock);
}
/*PRIVATE*/
struct task_t *queue_get_task() {

    pthread_mutex_lock(&queue_lock);

    //wait for thread
    pthread_cond_wait(&queue_not_empty, &queue_lock);

    if (terminated)
        return NULL;

    struct task_t *task = queue_head; 
    queue_head = task->queue_next;

    pthread_mutex_unlock(&queue_lock);
    
    return task;
}

/*PRIVATE*/
/*Funcao para alocar memoria e criar uma task*/
struct task_t* task_create(int op_n, int op, char* key, char* data, int dataSize){
    if( op_n < 0 || op < 0 || key == NULL || dataSize < 0 ){ /*data pode ser == NULL, por isso nao verificar*/
        return NULL;
    }

    struct task_t *ptrTask = malloc(sizeof(struct task_t));
    if( ptrTask != NULL ){
        ptrTask->op_n = op_n;
        ptrTask->op = op;
        ptrTask->key = strdup(key);/*nem na operacao put nem na del data eh NULL*/
        if(data == NULL){/*na operacao del, data eh NULL*/
            ptrTask->data = data;
        }else{
            ptrTask->data = strdup(data);
        }
        ptrTask->data_size = dataSize;
    }

    return ptrTask;
}

/*PRIVATE*/
void task_destroy(struct task_t *task){
    free(task);
}

int verify(int op_n){
    /*se o numero da operacao for maior do que o numero
    de operacoes de escrita realizadas pelo servidor significa
    que a operacao ainda nao foi feita ou terminada*/
    return op_n <= op_count;
}
////////////////////////////////////////////////////////////////////////////////
//ZOOKEEPER 

/*PRIVATE*/
//funçao para verificar a conexao
void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context) {
	if (type == ZOO_SESSION_EVENT) {
		if (state == ZOO_CONNECTED_STATE) {
			is_connected = 1; 
		} else {
			is_connected = 0; 
		}
	}
}

/*PRIVATE*/
//funçao para verificar filhos nós
static void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx) {
	zoo_string* children_list =	(zoo_string *) malloc(sizeof(zoo_string));
	//int zoo_data_len = ZDATALEN;
	if (state == ZOO_CONNECTED_STATE)	 {
		if (type == ZOO_CHILD_EVENT) {
	 	   /* Get the updated children and reset the watch */ 
 			if (ZOK != zoo_wget_children(zh, root_path, child_watcher, watcher_ctx, children_list)) {
 				fprintf(stderr, "Error setting watch at %s!\n", root_path);
 			}
			fprintf(stderr, "\n=== znode listing === [ %s ]", root_path); 
			for (int i = 0; i < children_list->count; i++)  {
				fprintf(stderr, "\n(%d): %s", i+1, children_list->data[i]);
			}
            
			fprintf(stderr, "\n=== watching ===\n");
            if(children_list->count == 2){
                int size_backup = MAX_IP;
                char* address_backup = malloc(MAX_IP);
                memset(address_backup,0,MAX_IP);
                zoo_get(zh, backup_path, 0, address_backup, &size_backup, NULL);
                new_server = rtree_connect(address_backup);
                printf("\nTree backup connection %s created with zookeeper \n",address_backup);

            }
            else{
                new_server = NULL;
            }
		 } 
	 }

    int size = MAX_IP;
    char *address = malloc(size);
    memset(address,0,size);

    //if primary server dies
    if (ZOK != zoo_get(zh, primary_path, 0, address, &size, NULL)){
        fprintf(stderr, "\n\tPRIMARY SERVER OFFLINE, replacing backup for primary\n");
        zoo_delete(zh, backup_path, -1);
        if (ZOK != zoo_create(zh, primary_path, address, size, & ZOO_OPEN_ACL_UNSAFE, ZOO_EPHEMERAL, id, MAX_IP)) {
            fprintf(stderr, "\n%s ZNode is online.\n", primary_path);
        }
        new_server = NULL;
    }
	free(children_list);
}

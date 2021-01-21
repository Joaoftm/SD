
#include "inet.h"
#include "client/client_stub.h"
#include "client/client_stub-private.h"
#include "zookeeper/zookeeper.h"

int exists(char *fname, char *functions[], int n_functions);
int t_put(struct rtree_t *tree, char *key, char* data_, int strlen);
int t_get(struct rtree_t *tree, char *key);
int t_del(struct rtree_t *tree, char *key);
int t_size(struct rtree_t *tree);
int t_free_keys(char **keys);
int t_get_keys(struct rtree_t *tree);
int t_height(struct rtree_t *tree);
int t_verify(struct rtree_t *tree, char* op_n_str);
int start_stdin(struct rtree_t *tree_primary, struct rtree_t *tree_backup, int length);


////////////////////////////////////////////////////////////////////////
//Zookeeper

struct rtree_t *tree_primary;
struct rtree_t *tree_backup;
const char *primary_path = "/kvstore/primary";
const char *backup_path = "/kvstore/backup";
static zhandle_t *zh;
static char* root_path = "/kvstore";
static int is_connected;
static char *watcher_ctx = "ZooKeeper Data Watcher";

typedef struct String_vector zoo_string; 
zoo_string* children_list = NULL;

/**
* Data Watcher function for /MyData node
*/
static void child_watcher(zhandle_t *wzh, int type, int state, const char *zpath, void *watcher_ctx) {
	zoo_string* children_list =	(zoo_string *) malloc(sizeof(zoo_string));
	
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
			fprintf(stderr, "\n=== done ===\n");
		 } 
	 }
	 free(children_list);
}

void connection_watcher(zhandle_t *zzh, int type, int state, const char *path, void* context) {
	if (type == ZOO_SESSION_EVENT) {
		if (state == ZOO_CONNECTED_STATE) {
			is_connected = 1; 
		} else {
			is_connected = 0; 
		}
	} 
}

int main(int argc, char *argv[]) {

    printf("\n\n");
    if (argc != 2) {
        printf("Params are invalid.\nUse: ./binary/tree-client [host:port]\n");
        return -1;
    }
    
    char *zookeeper_addr = argv[1];
    
    printf("Connecting to Zookeeper address: %s\n",zookeeper_addr);

    // CONNECT TO ZOOKEEPER
    if (zookeeper_addr == NULL)
        return -1;

    // Connect to ZooKeeper server
    zh = zookeeper_init(zookeeper_addr, connection_watcher, 2000, 0, NULL, 0);

    if (zh == NULL) {
        fprintf(stderr, "Tree Client:\n\tError zookeeper_init().\n");
        exit(EXIT_FAILURE);
    }

    children_list = (zoo_string*) malloc(sizeof(zoo_string));

    // Get the list children synchronously
    if (ZOK != zoo_wget_children(zh, root_path, &child_watcher, watcher_ctx, children_list)) {
        fprintf(stderr, "Error setting watch at %s!\n", root_path);
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "\n=== znode connections === [ %s ]", root_path); 

    for (int i = 1; i < children_list->count; i++)  {
        fprintf(stderr, "\n(%d): %s\n", i, children_list->data[i]);
    }

    // server address
    int size_primary = MAX_IP;
    char* address_primary = malloc(MAX_IP);
    memset(address_primary,0,MAX_IP);
    zoo_get(zh, primary_path, 0, address_primary, &size_primary, NULL);

    int size_backup = MAX_IP;
    char* address_backup = malloc(MAX_IP);
    memset(address_backup,0,MAX_IP);
    zoo_get(zh, backup_path, 0, address_backup, &size_backup, NULL);

    printf("\n%s primary // %s backup \n",address_primary,address_backup);
    printf("There are %d servers ONLINE",children_list->count);
    // CONNECT
    if (tree_primary == NULL || strcmp(tree_primary->address,address_primary) != 0)
        tree_primary = rtree_connect(address_primary);
    if (tree_backup == NULL || strcmp(tree_backup->address,address_backup) != 0)
        tree_backup = rtree_connect(address_backup);
    free(address_primary);
    free(address_backup);

    fprintf(stderr, "\n=== done ===\n");

    // saved children in zoo_string

    // Cycle
    if (start_stdin(tree_primary,tree_backup,MAX_MSG) != 0) return -1;             // Define input max length

    // Close the ZooKeeper handle
    zookeeper_close(zh);

    // DISCONNECT
    int error = 0;
    error +=  rtree_disconnect(tree_primary);
    error += rtree_disconnect(tree_backup);
    return error;
}

int start_stdin(struct rtree_t *tree_primary, struct rtree_t *tree_backup, int max_length) {

    char input[max_length];
    input[max_length-1] = '\0';

    int error = 0;
    while(error == 0) {    

        printf("--> ");
        fgets(input,max_length-1,stdin);
        printf("Input:\t%s",input);

        if (input == NULL)
            continue;

        int n_functions = 8;
        char *functions[] = {"quit","size","getkeys","get","del","put","height","verify"};
        char delimitor[sizeof(" \n")] = " \n";

        char *fname = strtok(input,delimitor);

        if (!exists(fname,functions,n_functions)) {
            printf("\tFunction name inexistent.\n");
            printf("\tTry one of these:\n");
            printf("\t\tput [key] [data]\n\t\tdel [key]\n\t\tget [key]\n\t\tverify [op_n]\n\t\theight\n\t\tgetkeys\n\t\tsize\n\t\tquit\n");
            continue;
        }

        printf("\tFunction called:\t%s\n",fname);
        
        if (strcmp(input,functions[0]) == 0) {                     
        //               QUIT        //
            break;
        } else if (strcmp(fname,functions[1]) == 0)             
        //               SIZE                   //
            error += t_size(tree_backup);
        else if (strcmp(fname,functions[2]) == 0)
        //              GETKEYS             //                  
            error += t_get_keys(tree_backup);
        else if (strcmp(fname,functions[6]) == 0){                 
            //          HEIGHT          //
                error += t_height(tree_backup);
        }
        else {
            char *key = strtok(NULL,delimitor);
            printf("\tKey:\t%s\n",key);

            if (key == NULL)
                continue;
            else if (strcmp(fname,functions[3]) == 0)   
            //          GET         //                           
                error += t_get(tree_backup,key);
            else if (strcmp(fname,functions[4]) == 0)    

            //          DEL            //                           
                error += t_del(tree_primary,key);
            else if (strcmp(fname,functions[5]) == 0) {
                char *data = strtok(NULL,delimitor);
                
                if (data == NULL)
                    continue;
                
                printf("\tData:\t%s\n",data);

                //      PUT     //
                error += t_put(tree_primary,key,data,strlen(data));      
                //error += t_put(tree_backup,key,data,strlen(data));                       
            }
            else if (strcmp(fname,functions[7]) == 0)              // VERIFY
                error += t_verify(tree_backup,key);
            
        }
    }
    return error;
}

int exists(char *fname, char *functions[], int n_functions) {

    if (fname == NULL || functions == NULL || n_functions <= 0)
        return 0;

    for (int i = 0; i < n_functions; i++)
        if (strcmp(fname,functions[i]) == 0)
            return 1;
    
    return 0;
}

// Adapters
// PUT
int t_put(struct rtree_t *tree, char *key, char *data, int strlen) {
    if (tree == NULL || key == NULL || data == NULL)
        return -1;

    int data_size = (strlen+1)*sizeof(char);    
    void *data_to_send = malloc(data_size);
    memcpy(data_to_send,data,data_size);
    

    struct data_t *content = data_create2(data_size,data_to_send);
    struct entry_t *entry = entry_create(strdup(key),content);

    if (data == NULL)
        return -1;
    else if (entry == NULL) {
        data_destroy(content);
        return -1;
    } 

    if (rtree_put(tree,entry) == -1) {
        entry_destroy(entry);
        printf("tree_Client:\n\tError inserting Entry.\n");
        return -1;
    }
    
    entry_destroy(entry);
    printf("\t-> Entry created. Key %s | Data %s\n",key,data);
    return 0;
}
// GET
int t_get(struct rtree_t *tree, char *key) {
    if (tree == NULL || key == NULL)
        return -1;

    struct data_t *data = rtree_get(tree,key);

    if (data == NULL) {
        printf("\t-> Inexisting key.\n");
        return 0;
    }

    printf("\t-> Data of size: %d\n\t\tContent: %s\n",data->datasize,(char *) data->data);   // Watch out for cast
    data_destroy(data);
    return 0;
}
// DEL
int t_del(struct rtree_t *tree, char *key) {
    if (tree == NULL || key == NULL)
        return -1;

    int code = rtree_del(tree,key);

    if  (code == 1)
        printf("\t-> Non existing key.\n");
    else if (code == 0)
        printf("\t-> Entry deleted. Key %s\n",key);
    else
        return -1;
    
    return 0;
}
// SIZE
int t_size(struct rtree_t *tree) {
    
    if (tree == NULL)
        return -1;

    int size = rtree_size(tree);

    if (size < 0)
        return -1;

    printf("\t-> Size of tree: %d values.\n",size);
    return 0;
}
// FREE KEYS
int t_free_keys(char **keys) {
    if (keys == NULL)
        return -1;

    rtree_free_keys(keys);
    return 0;
}
// GET KEYS
int t_get_keys(struct rtree_t *tree) {
    if (tree == NULL) 
        return -1;

    char size = rtree_size(tree);
    char **keys = rtree_get_keys(tree);

    if (size == 0) {
        printf("\t-> No keys in the tree.\n");
        return 0;
    } else if (size < 0 || keys == NULL)
        return -1;

    printf("\t-> Keys in the tree:\n");
    int i = 0;
    for (i = 0; i < size; i++)
        printf("\t\t\t%s\n",keys[i]);

    return t_free_keys(keys);
}
// HEIGHT
int t_height(struct rtree_t *tree) {
    if (tree == NULL)
        return -1;

    int h = rtree_height(tree);

    if (h < 0)
        return -1;

    printf("\t-> Height of the tree: %d \n",h);
    return 0;
}
// VERIFY
int t_verify(struct rtree_t *tree, char* op_n_str) {

    int op_n = atoi(op_n_str);
    int op_network = rtree_verify(tree,op_n);

    printf("Op sent: %d | ",op_n);
    if (op_network == 1)
        printf("Op complete.\n");
    else 
        printf("Op failed.\n");
    
    return 0;
}

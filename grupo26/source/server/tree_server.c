/*
# Grupo 26
# Miguel Vasconcelos - 51302
# João Miranda - 47143
# Gonçalo Santos - 51962
*/

#include "inet.h"
#include "server/network_server.h"
#include "message/message-private.h"

void close_connection() {
    network_server_close();
    tree_skel_destroy();
}

int main(int argc, char *argv[]) {
	
	int socket;
	
	if(argc != 4){
		printf("Use: tree-server <port> \n");
		printf("Example: ./binary/tree_server 5000 127.0.0.1 2181\n");
		return -1;
	}

	socket = network_server_init(atoi(argv[1]));
	
	tree_skel_init(argv[1], argv[2],argv[3]);
	
	int v = network_main_loop(socket);
	
	network_server_close();

	return v;
}
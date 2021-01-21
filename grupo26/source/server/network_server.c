

#include <signal.h>
#include "message/message-private.h"
#include "inet.h"
#include "server/network_server.h"

//print function
void print_msg(struct message_t *msg);

static volatile int keep_looping = 1;

void sigint_handler(int dummy) {
    keep_looping = 0;
}

int sockserv, connsock;
struct sockaddr_in server, client;
int nbytes;
socklen_t size_client;

void ctrl_c() {
    printf("\n\tCTRL+C pressed.\n\tShutting down Server.\n\n");
    tree_skel_destroy();
    network_server_close();
    printf("\n\tShutdown Completed.\n");
    exit(0);
}
int network_server_init(short port) {

    // CTRL+C
    signal(SIGINT,ctrl_c);

    // INTERRUPT
    signal(SIGPIPE,SIG_IGN);

    if (port < 0)
        return -1;

    if ((sockserv = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        perror("Network Server:\n\tError creating socket.\n");
        return -1;
    }

    int enable = 1;
    if (setsockopt(sockserv, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("Network Server:\n\tError re-using  socket.\n");
        return -1;
    }

    
    server.sin_family = AF_INET;
    server.sin_port = htons(port);     
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockserv,(struct sockaddr *) &server,sizeof(server))) {
        perror("Error Binding.\n");
        close(sockserv);
        return -1;
    }

    if (listen(sockserv, 0) < 0) {
        perror("Error Listening.\n");
        close(sockserv);
        return -1;
    }

    printf("Server Online.\n");

    return sockserv;

}

int network_main_loop(int listening_socket){

	if  (listening_socket < 0) {
		return -1;
	}
	struct pollfd *connections = malloc(sizeof(struct pollfd*)*NFDESC);
	if(connections == NULL){
		return -1;
	}
	struct sockaddr_in client;
	socklen_t size_client = sizeof(struct sockaddr);
	int msg_process_result, send_msg_result, closed = 0, nfds, kfds, i;

	printf("\nServer is waiting for a new connection...\n");
	for (i = 0; i < NFDESC; i++){
    		connections[i].fd = -1;
	}

 	connections[0].fd = listening_socket;  // Vamos detetar eventos na welcoming socket
 	connections[0].events = POLLIN;

  	nfds = 1;

	signal(SIGINT, sigint_handler);
	int nClients = 0;

	while((kfds = poll(connections, nfds, TIMEOUT)) >= 0 && keep_looping){
		//caso haja uma nova conexao
		if((connections[0].revents & POLLIN) && (nfds < NFDESC)){
			//coloca-se o cliente em connections
			nClients++;
			if ((connections[nfds].fd = accept(connections[0].fd, (struct sockaddr *) &client, &size_client)) > 0){
				//printf("New client connection %d\n",nClients);
				connections[nfds].events = POLLIN;
				nfds++;
			}
		}

		for(i = 1; i < nfds; i++ ){
			if(connections[i].revents & POLLIN){
				struct message_t* message;
				if ((message = network_receive(connections[i].fd)) == NULL) {
					printf("Connection ended by client\n");
					//Caso ocorra erro de leitura, fecha-se o socket do cliente
					close(connections[i].fd);
					connections[i].fd = -1;
					closed = 1;
					continue;
				}else{
					printf("New message received from the client...\n");
					print_msg(message); // for debugging purposes
					// invoke will update message, returns -1 if something fails
					msg_process_result = invoke(message);
					if (msg_process_result == -1) {
						printf("There was an error while processing the current message\n");
					}
					printf("Message that is going to be sent to the client:\n");
					print_msg(message);
					if ((send_msg_result = network_send(connections[i].fd, message)) > 0) {
						printf("Message was successfuly processed and sent to client\n");
					} else {
						//caso ocorra erro de envio de mensagem, fecha-se o socket do cliente
						close(connections[i].fd);
						connections[i].fd = -1;
						closed = 1;
						continue;
					}
				}
			}

			if((connections[i].revents & POLLHUP) || (connections[i].revents & POLLERR)){
				close(connections[i].fd);
				connections[i].fd = -1;
				closed = 1;
			}
		}
		//se houver fecho de alguma conexao, as posicoes dos clientes
		//vao ser shifted para a esquerda
		if(closed){
			closed = 0;
			int j;
			for(j = 0; j < nfds; j++){
				if(connections[j].fd == -1){
					connections[j].fd = connections[j+1].fd;
					i--;
					nfds--;
				}
			}
		}
	}
	close(listening_socket);
	return 0;
}

struct message_t *network_receive(int client_socket) {

    if (client_socket < 0) return NULL;

    struct message_t *msg = malloc(sizeof(struct message_t));
    if (msg == NULL)
        return NULL;

    msg->message = receive_all(client_socket);

    if (msg->message == NULL) {
        free(msg);
        return NULL;
    }

    return msg;
}

int network_send(int client_socket, struct message_t *msg) {

    if (client_socket <= 0 || msg == NULL || msg->message == NULL)
        return -1;

    return send_all(client_socket,msg->message);
}

int network_server_close() {
    return close(sockserv) == -1 || close(connsock) == -1;
}

void print_msg(struct message_t *msg) {

    if (msg == NULL)
        return;

    printf("\tMESSAGE:\n");
    printf("\t\tcode: %d, c_type: %d\n", msg->message->opcode, msg->message->c_type);
    
    if (msg->message->key != NULL)
        printf("\tKey: %s\n", msg->message->key);
    if (msg->message->data != NULL)
        printf("\tData: %s\n", msg->message->data);
    if (msg->message->data_size != 0)
        printf("\t\tData_size: %d\n", msg->message->data_size);

    printf("\t--- End of MESSAGE ---\n");
}

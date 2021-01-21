/*
# Grupo 26
# Miguel Vasconcelos - 51302
# João Miranda - 47143
# Gonçalo Santos - 51962
*/

#ifndef _tree_SKEL_PRIVATE_H
#define _tree_SKEL_PRIVATE_H

#include "sdmessage.pb-c.h"


struct task_t {
    int op_n; /*o número da operação*/
    int op; /*a operação a executar. op=0 se for um delete, op=1 se for um put*/
    char* key; /*a chave a remover ou adicionar*/
    int data_size;
    char* data; /* os dados a adicionar em caso de put, ou NULL em caso de delete*/
    struct task_t* queue_next; /*proxima task_t na lista (NULL se a task_t atual for a tail)*/
};



/*Funcao que processa task*/
void *process_task(void *params);

/*Funcao que adiciona uma tarefa ah lista de tarefas*/
void queue_add_task(struct task_t *task);

/*Funcao que remove a proxima tarefa a executar da lista de tarefas*/ 
struct task_t *queue_get_task();

/*Funcao para alocar memoria e criar uma tarefa*/
struct task_t* task_create(int op_n, int op, char* key, char* data, int dataSize);

/*Funcao para destruir uma tarefa*/
void task_destroy(struct task_t *tarefa);

/*verificar task*/
int verify(int op_n);

#endif

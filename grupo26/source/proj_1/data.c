/*
# Grupo 26
# Miguel Vasconcelos - 51302
# João Miranda - 47143
# Gonçalo Santos - 51962
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data.h"

/* Função que cria um novo elemento de dados data_t e reserva a memória
 * necessária, especificada pelo parâmetro size
 */
struct data_t *data_create(int size){

  if(size <= 0){
    return NULL;
  }

  struct data_t *new_data =(struct data_t *) malloc(sizeof(struct data_t));
  /*
  if(new_data == NULL){
    return NULL;
  }
  */
  new_data->data=malloc(size);
  /*
  if(new_data->data == NULL){
    free(new_data);
    return NULL;
  }
  */
  new_data->datasize=size;
  return new_data;

}

/* Função idêntica à anterior, mas que inicializa os dados de acordo com
 * o parâmetro data.
 */
struct data_t *data_create2(int size, void *data){

  if(size <= 0 || data == NULL){
    return NULL;
  }

  struct data_t *new_data = (struct data_t *) malloc(sizeof(struct data_t));
  //new_data = data_create(size);


  if(new_data == NULL){
    //data_destroy(new_data);
		return NULL;
  }

  //free(new_data -> data);
	new_data -> data = data ;
  new_data -> datasize = size;
	return new_data;

}

/* Função que elimina um bloco de dados, apontado pelo parâmetro data,
 * libertando toda a memória por ele ocupada.
 */
void data_destroy(struct data_t *data){

  if (data == NULL) {
      return;
  }
  free(data -> data);
	free(data);

}

/* Função que duplica uma estrutura data_t, reservando a memória
 * necessária para a nova estrutura.
 */
struct data_t *data_dup(struct data_t *data){
    if(data == NULL|| data->data == NULL || data -> datasize <=0) {
            return NULL;
        }

    struct data_t *data_copy = (struct data_t *) malloc(sizeof(struct data_t));
    //data_copy = data_create(data->datasize);

    if (data_copy == NULL ) {
        return NULL;
    }
        data_copy-> data = malloc(data ->datasize);
        // memoria alocada com sucesso
        if (data->data == NULL) {
            free(data_copy);
            return NULL;
        }

        memcpy(data_copy->data, data->data, (data->datasize));
        data_copy -> datasize = data -> datasize;
        return data_copy;


}

/* Função que substitui o conteúdo de um elemento de dados data_t.
* Deve assegurar que destroi o conteúdo antigo do mesmo.
*/
void data_replace(struct data_t *data, int new_size, void *new_data){
    if(data==NULL){
      return;
    }
    if(new_size <= 0 || new_data == NULL){
      return;
    }

    free(data->data);

    data-> data = new_data;
    data->datasize = new_size;
}
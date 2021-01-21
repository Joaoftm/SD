/*
# Grupo 26
# Miguel Vasconcelos - 51302
# João Miranda - 47143
# Gonçalo Santos - 51962
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "entry.h"

#include "data.h"


struct entry_t *entry_create(char *key, struct data_t *data){

    if(key == NULL || data == NULL) {
        return NULL;
    }

	struct entry_t *newData = (struct entry_t *) malloc(sizeof(struct entry_t));


	newData->key = key;
	newData->value = data;

  if(newData == NULL||newData->key ==NULL || newData->value==NULL) {
  free(newData);
  return NULL;
}

  return newData;



}

void entry_destroy(struct entry_t *entry){
	if(entry == NULL || entry->value == NULL || entry->key == NULL) {
		return;
	}
	data_destroy(entry->value);
	free(entry->key);
	free(entry);
}

struct entry_t *entry_dup(struct entry_t *entry){

    if(entry == NULL) {
            return NULL;
        }

    struct entry_t *entry_copy = (struct entry_t *) malloc(sizeof(struct entry_t));

    entry_copy -> value = data_dup(entry -> value);
    entry_copy -> key = strdup(entry -> key);

    //entry_copy = entry_create(strdup(entry->key),data_dup(entry->value));

	return entry_copy;
}

void entry_replace(struct entry_t *entry, char *new_key, struct data_t *new_value){
  if(entry==NULL){
    return;
  }
  if(new_key==NULL || new_value == NULL){
    return;
  }
  free(entry->key);
  data_destroy(entry -> value);
  entry -> value = new_value;
  entry -> key = new_key;


}

int entry_compare(struct entry_t *entry1, struct entry_t *entry2){
	int i = strcmp(entry1->key,entry2->key);
	if(i == 0){
		return 0;
	}
	else if(i>0){
		return 1;
	}
	else{
		return -1;
	}
}
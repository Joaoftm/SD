/*
# Grupo 26
# Miguel Vasconcelos - 51302
# João Miranda - 47143
# Gonçalo Santos - 51962
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>

#include "data.h"
#include "entry.h"
#include "tree.h"
#include "tree-private.h"
#include "serialization.h"

int data_to_buffer(struct data_t *data, char **data_buf){
	if(!data || !data_buf){
        return -1;
    }

	int size = data->data ? strlen(data->data)+1 : 0;
	int totalSize = sizeof(int) + size;
	*data_buf = calloc(1, totalSize);
	if(*data_buf){
	int htons_size = htons(size);

	memcpy(*data_buf, &htons_size, sizeof(htons_size));
	memcpy(*data_buf + sizeof(htons_size), data->data, size);
	}

	return totalSize;
}

struct data_t *buffer_to_data(char *data_buf, int data_buf_size){

	int maxSize = sizeof(int);//tamanho buffer + data vazio
	if(data_buf_size < maxSize){
        return NULL;
    }

	struct data_t *data;
	data = calloc(1,sizeof(struct data_t));
	int v = 0;
	memcpy(&v, data_buf, sizeof(v));
	v = ntohs(v);

	memcpy(&data->datasize, &v, sizeof(v));

	data->data = calloc(1,data->datasize);
	memcpy(data->data, data_buf + sizeof(v) , data->datasize);

	return data;
}

int entry_to_buffer(struct entry_t *data, char **entry_buf){

	if(!data || !entry_buf)
    {
        return -1;}

	int totalSize = sizeof(int) + strlen(data->key)+1 + sizeof(int) + data->value->datasize;
	*entry_buf = calloc(1,totalSize);

	int offset = 0;
	int v = htons(strlen(data->key)+1);

	memcpy(*entry_buf, &v, sizeof(v));
	offset += sizeof(v);

	memcpy(*entry_buf + offset, data->key, strlen(data->key)+1);
	offset += strlen(data->key)+1;

	v = htons(data->value->datasize);
	memcpy(*entry_buf + offset, &v, sizeof(v));
	offset += sizeof(v);

	memcpy(*entry_buf + offset, data->value->data, data->value->datasize);
	return totalSize;

}

struct entry_t *buffer_to_entry(char *entry_buf, int entry_buf_size){

	int minSize = 2 * sizeof(int);
	if(entry_buf_size<minSize){
        return NULL;
    }

	struct entry_t *data;
	data = calloc(1,sizeof(entry_buf_size));
	int offset = 0;
	int v = 0;

	memcpy(&v, entry_buf, sizeof(v));
	offset += sizeof(v);
	v = ntohs(v);

	if (v > 0){
		data->key = calloc(1,v + 1);
		memcpy(data->key, entry_buf + offset, v);
		offset += v;
	}


	memcpy(&v, entry_buf + offset, sizeof(v));

	v = ntohs(v);
	data->value = calloc(1, sizeof(int) + v);
	memcpy(&data->value->datasize, &v, sizeof(v));

	offset += sizeof(data->value->datasize);


	if (data->value->datasize > 0){
		data->value->data = calloc(1,v + 1);
		memcpy(data->value->data, entry_buf + offset, data->value->datasize);
	}
	return data;


}

int tree_to_buffer(struct tree_t *tree, char **tree_buf){
	if(tree == NULL || tree_buf == NULL){
		return -1;
	}
	int datasize = tree->entry->value->datasize;
	int strl = strlen(tree->entry->key)+1;
	int treeSize = tree_size(tree);

	char *aux_buff = malloc(sizeof(int) + strl + sizeof(int) + datasize + sizeof(int) + treeSize);
	int offset = 0;

	//tamanho de key
	memcpy(aux_buff, &strl, sizeof(int));
	offset += sizeof(int);

	//passar a key
	memcpy(aux_buff + offset, tree->entry->key, strl);
	offset += strl;
	//tamanho datasize
	memcpy(aux_buff + offset, &datasize, sizeof(int));
	offset += sizeof(int);
	//passa a entry
	memcpy(aux_buff + offset, tree->entry->value, datasize);
	offset += datasize;
	//tamanho da tree
	memcpy(aux_buff + offset, &treeSize, sizeof(int));
	offset += sizeof(int);
	//passa a tree
	memcpy(aux_buff+offset, tree_get_keys(tree), treeSize);

	*tree_buf = aux_buff;
	return offset;
}

struct entry_t *buffer_to_tree(char *tree_buf, int tree_buf_size){
	if (tree_buf == NULL || tree_buf_size <= 0){
		return NULL;
	}

	int offset = 0;
	int strl = 0;
	//tamanho da key
	memcpy(&strl,tree_buf,sizeof(int));
	offset += sizeof(int);

	//descobre key
	char* key = malloc(0);
	memcpy(key,tree_buf + offset, strl);
	offset += strl;

	//datasize
	int datasize = 0;
	memcpy(&datasize,tree_buf + offset, sizeof(int));
	offset += sizeof(int);

	//descobre a data
	void* data_data = malloc(datasize);
	memcpy(data_data,tree_buf + offset, datasize);
	offset += datasize;

	//tree_size
	int tree_size = 0;
	memcpy(&tree_size,tree_buf,sizeof(int));
	offset += sizeof(int);

	struct data_t *data = data_create2(datasize,data_data);
	struct entry_t *entry = entry_create(key,data);

	return entry;
}
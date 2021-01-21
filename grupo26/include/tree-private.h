/*
# Grupo 26
# Miguel Vasconcelos - 51302
# João Miranda - 47143
# Gonçalo Santos - 51962
*/
#ifndef _TREE_PRIVATE_H
#define _TREE_PRIVATE_H

#include "tree.h"


struct tree_t {
	struct entry_t *entry;
	struct tree_t *left;
	struct tree_t *right;
};


struct tree_t *tree_create();

//encontrar o valor mais a baixo(esquerda) na arvore;
struct tree_t *search_min(struct tree_t *tree);

int tree_to_array(struct tree_t *tree,char **rt,int i);

void print_tree(struct tree_t *tree);

int del_nodes(struct tree_t *tree,char *key, struct tree_t *min);

#endif

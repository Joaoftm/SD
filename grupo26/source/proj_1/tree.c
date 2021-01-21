/*
# Grupo 26
# Miguel Vasconcelos - 51302
# João Miranda - 47143
# Gonçalo Santos - 51962
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "entry.h"
#include "data.h"
#include "tree.h"
#include "tree-private.h"


struct tree_t *tree_create(){

    struct tree_t *tree = (struct tree_t *) malloc (sizeof(struct tree_t));
    
    tree->entry = NULL;

    return tree;
}

void tree_destroy(struct tree_t *tree){
    if(tree->entry == NULL){
        return;
    }

    tree_destroy(tree->left);
    tree_destroy(tree->right);

    free(tree);
}


int tree_put(struct tree_t *tree, char *key, struct data_t *value){
  if(key == NULL || value == NULL || tree == NULL){
    return -1;
  }
  //CREATE ROOT
  if(tree->entry == NULL){
    //printf("\n AHAHQUII ENTOROU");
    struct entry_t *new_entry = entry_create(strdup(key),data_dup(value));
    tree->entry = new_entry;

    tree->left = tree_create();
    tree->right = tree_create();

    return 0;
  }
  
  else{
    //TREE COMPARE
    //struct entry_t *new_entry = entry_create(strdup(key),data_dup(value));
    int cmp = strcmp(tree->entry->key,key);
    
    if(cmp > 0){
      //printf("\n AHAHQUII lllll");
      return tree_put(tree->left,strdup(key),data_dup(value));
    }
    if(cmp < 0){
      //printf("\n AHAHQUII RRRR");
      return tree_put(tree->right,strdup(key),data_dup(value));
    }
    if(cmp == 0){
      tree->entry->value = value;
    }

  }

  return 0;

}

struct data_t *tree_get(struct tree_t *tree, char *key){
  if(tree->entry == NULL || key == NULL){
    return NULL;
  }
  
  int cmp = strcmp(tree->entry->key,key);
  
  if(cmp > 0){
    return tree_get(tree->left,strdup(key));
  }
  if(cmp < 0){
    return tree_get(tree->right,strdup(key));
  }
  if(cmp == 0){
    return tree->entry->value;
  }
  return NULL;
}

int tree_del(struct tree_t *tree, char *key){
  if(tree->entry == NULL){
    return -1;
  }
  struct tree_t *min = search_min(tree);
  return del_nodes(tree,key,min);
  
}

int tree_size(struct tree_t *tree){
  if(tree->entry == NULL){
    return 0;
  }
  return 1 + tree_size(tree->right) + tree_size(tree->left);   
}

int tree_height(struct tree_t *tree){
  if (tree->entry==NULL)  
       return 0; 
   else 
   { 
       int lDepth = tree_height(tree->left); 
       int rDepth = tree_height(tree->right); 
  
       if (lDepth > rDepth)  
           return(lDepth+1); 
       else return(rDepth+1); 
   } 
}
char **tree_get_keys(struct tree_t *tree){
  char **rt = malloc(tree_size(tree) * sizeof(char*) + sizeof(NULL));

  if(rt == NULL){
    return NULL;
  }
  tree_to_array(tree,rt,0);
  rt[tree_size(tree)] = NULL;

  return rt;
}

void tree_free_keys(char **keys){
  for(int i = 0;keys[i] != NULL;i++){
    free(keys[i]);
  }
}

struct tree_t *search_min(struct tree_t *tree){
    
    if(tree->entry == NULL)
        return NULL;
    else if(tree->left != NULL)
        return search_min(tree->left);
    return tree;
}

int tree_to_array(struct tree_t *tree,char **rt,int i){
  if(tree == NULL){
    return i;}
  rt[i] = tree->entry->key;
  i++;
  if(tree->left->entry != NULL){
    i = tree_to_array(tree->left, rt, i);}
  if(tree->right->entry != NULL){
    i = tree_to_array(tree->right, rt, i);}

  return i;
}

void print_tree(struct tree_t *tree){
  if(tree->entry != NULL){
    printf("    %s",tree->entry->key);
    printf("  --     -- ");
    printf("--        --");
    
  }

  if(tree->left->entry != NULL){
    printf("%s",tree->left->entry->key);
    print_tree(tree->left);
  }
  if(tree->right->entry != NULL){
    printf("         %s",tree->right->entry->key);
    print_tree(tree->right);
  }

  return;

}

int del_nodes(struct tree_t *tree,char *key, struct tree_t *min){
  int cmp = strcmp(tree->entry->key,key);
  if(cmp > 0){
    return tree_del(tree->left,strdup(key));
  }
  if(cmp < 0){
    return tree_del(tree->right,strdup(key));
  }
  if(cmp == 0){
    //printf("ANDA AQUI \n");

    //if no childs
    if(tree->left == NULL && tree->right == NULL){
      //printf("estamos AQUI MEU  00000");
      tree->entry = NULL;
      tree->entry->key = NULL;
      tree->entry->value = NULL;
      return 0;
    }
    
    //if one child
    if(tree->left != NULL){
      //printf("estamos AQUI MEU  111LLLLL");
      tree->entry = tree->right->entry;  

      if(tree->left->entry != NULL) tree->left = tree->left->left;
      if(tree->right->entry != NULL) tree->right = tree->right->right;
      
      return 0;
    }
    if(tree->right != NULL){
      //printf("estamos AQUI MEU   11111RRRR");
      tree->entry = tree->left->entry;  

      if(tree->left->entry != NULL) tree->left = tree->left->left;
      if(tree->right->entry != NULL) tree->right = tree->right->right;

      return 0;
    }

    //if two childs
    else{
      //printf("estamos AQUI MEU   2222222222");
      //printf("CASO TRAMADO");
      tree->entry = tree->left->entry;
      tree->left->entry = min->entry;
      
      return 0;
    }
  }
  return 0;
}
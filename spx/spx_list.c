/*
 * =====================================================================================
 *
 *       Filename:  spx_list.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2014/06/16 18时18分32秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#include <stdlib.h>

#include "spx_types.h"
#include "spx_defs.h"
#include "spx_alloc.h"
#include "spx_list.h"

struct spx_list *spx_list_new(SpxLogDelegate *log,\
        size_t init_size,\
       SpxListNodeFreeDelegate *node_free,
        err_t *err){
    struct spx_list *list = spx_alloc_alone(sizeof(*list),err);
    if(NULL == list){
        return NULL;
    }
    if(0 == init_size){
        init_size = 16;
    }
    list->log = log;
    list->busy_size = 0;
    list->free_size = init_size;
    list->size = init_size;
    list->node_free = node_free;
    if(0 != init_size){
        list->nodes = spx_alloc(init_size,sizeof(struct spx_list_node),err);
        if(NULL == list->nodes){
            SpxFree(list);
            return NULL;
        }
    }
    return list;
}


struct spx_list *spx_list_init(SpxLogDelegate *log,\
        size_t init_size,\
        SpxListNodeNewDelegate *new,\
        void *arg,\
        SpxListNodeFreeDelegate *free,\
        err_t *err){
    struct spx_list *list = spx_list_new(log,init_size,\
            free,err);
    if(NULL == list){
        return NULL;
    }
    if(NULL != new) {
        size_t i = 0;
        for(;i < init_size; i++){
            void *v = new(i,arg,err);
            if(NULL == v){
                spx_list_free(&list);
                return NULL;
            }
            *err =  spx_list_add(list,v);
            if(0 != *err){
                spx_list_free(&list);
                return NULL;
            }
        }
    }
    return list;

}

void *spx_list_get(struct spx_list *list,int idx){
    size_t i = idx < 0 ? list->size + idx :(size_t) idx;
    if(i >= list->size){
        return NULL;
    }
    struct spx_list_node *node = list->nodes + i;
    return node->v;
}

void *spx_list_get_and_out(struct spx_list *list,int idx){
    size_t i = idx < 0 ? list->busy_size + idx : (size_t) idx;
    if(i >= list->size){
        return NULL;
    }
    struct spx_list_node *node = list->nodes + i;
    void *v = node->v;
    size_t j = i;
    for(j = i; j < list->size; j++){
        if(j + 1 == list->size){
            struct spx_list_node *curr = list->nodes + j;
            curr->v = NULL;
        }
        struct spx_list_node *curr = list->nodes + j;
        struct spx_list_node *next = list->nodes + (j + 1);
        if(NULL != next->v){
            curr->v = next->v;
        }
    }
    list->busy_size --;
    list->free_size ++;
    return v;
}

err_t spx_list_delete(struct spx_list *list,int idx){
    size_t i = idx < 0 ? list->busy_size + idx : (size_t) idx;
    if(i >= list->busy_size){
        return EINVAL;
    }
    size_t j = i;
    for(j = i; j < list->size; j++){
        if(j + 1 == list->size){
            struct spx_list_node *curr = list->nodes + j;
            curr->v = NULL;
        }
        struct spx_list_node *curr = list->nodes + j;
        struct spx_list_node *next = list->nodes + (j + 1);
        if(NULL != next->v){
            curr->v = next->v;
        }
    }
    list->busy_size --;
    list->free_size ++;
    return 0;
}

err_t spx_list_insert(struct spx_list *list,int idx,void *v){
    size_t i = idx < 0 ? list->size + idx : (size_t) idx;
    err_t err = 0;
    if(i >= list->size){
        size_t size = i > 2 * list->size ? i : 2 * list->size;
        struct spx_list_node *new_nodes = spx_realloc(list->nodes,size,&err);
        if(NULL == new_nodes){
            return err;
        }
        list->nodes = new_nodes;
        list->size = size;
        list->free_size = size - list->busy_size;
    }
    size_t  j = list->size;
    for(;j > i;j--){
        struct spx_list_node *curr = list->nodes + j;
        struct spx_list_node *prev = list->nodes + (j -1);
        curr->v = prev->v;
    }
    (list->nodes + i)->v = v;
    list->busy_size ++;
    list->free_size --;
    return 0;
}

err_t spx_list_set(struct spx_list *list,int idx,void *v){
    size_t i = idx < 0 ? list->size + idx : (size_t) idx;
    err_t err = 0;
    if(i >= list->size){
        size_t size = i > 2 * list->size ? i : 2 * list->size;
        struct spx_list_node *new_nodes = spx_realloc(list->nodes,size,&err);
        if(NULL == new_nodes){
            return err;
        }
        list->nodes = new_nodes;
        list->size = size;
        list->free_size = size - list->busy_size;
    }
    struct spx_list_node *node =  list->nodes + i;
    if(NULL != node->v){
        list->node_free(&(node->v));
    }
    node->v = v;
    return 0;
}
err_t spx_list_add(struct spx_list *list,void *v){
    err_t err = 0;
    if(list->busy_size == list->size){
        size_t size = list->size * 2;
        struct spx_list_node *new_nodes = spx_realloc(list->nodes,size,&err);
        if(NULL == new_nodes){
            return err;
        }
        list->nodes = new_nodes;
        list->size = size;
        list->free_size = size - list->busy_size;
    }
    (list->nodes + list->busy_size)->v = v;
    list->busy_size ++;
    list->free_size --;
    return 0;
}

err_t spx_list_free(struct spx_list **list){
    size_t i = 0;
    for(;i < (*list)->size;i++){
        struct spx_list_node *node = (*list)->nodes + i;
        if(NULL != (*list)->node_free){
            (*list)->node_free(&(node->v));
        }
    }
    SpxFree((*list)->nodes);
    SpxFree(*list);
    return 0;
}

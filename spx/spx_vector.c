#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "spx_vector.h"
#include "spx_types.h"
#include "spx_defs.h"
#include "spx_errno.h"
#include "spx_alloc.h"

struct spx_vector *spx_vector_init(SpxLogDelegate *log,\
        SpxVectorValueFreeDelegate *handle,err_t *err){/*{{{*/
    err_t rc = 0;
    struct spx_vector *vector = NULL;
    vector = spx_alloc_alone(sizeof(*vector),err);
    if(NULL == vector) {
        SpxLog2(log,SpxLogError,rc,\
                "alloc vector is fail.");
        return NULL;
    }
    vector->log = log;
    vector->handle = handle;
    vector->size = 0;
    vector->header = NULL;
    return vector;
}/*}}}*/

err_t spx_vector_free(struct spx_vector **vector) {/*{{{*/
    if(NULL == *vector){
        return 0;
    }
    err_t rc = 0;
    struct spx_vector_node *node = NULL;
    while(NULL !=(node =  (*vector)->header)){
        (*vector)->header = (*vector)->header->next;
        if(NULL != (*vector)->handle){
            (*vector)->handle(&(node->v));
        }
        SpxFree(node);
    }
    (*vector)->size = 0;
    (*vector)->tail = NULL;
    SpxFree(*vector);
    return rc;
}/*}}}*/

err_t spx_vector_add(struct spx_vector *vector,void *v){/*{{{*/
    err_t rc = 0;
    struct spx_vector_node *node = NULL;
    node = spx_alloc_alone(sizeof(*node),&rc);
    if(NULL == node) {
        SpxLog2(vector->log,SpxLogError,rc,\
                "alloc vector node is fail.");
        return rc;
    }
    node->v = v;
    node->next = NULL;
    if(NULL == vector->header){
        vector->header = node;
        vector->tail = node;
    }else {
        vector->tail->next = node;
        node->prev = vector->tail;
        vector->tail = node;
        vector->size++;
    }
    return rc;
}/*}}}*/

void *spx_vector_get(struct spx_vector *vector,size_t idx,err_t *err){/*{{{*/
    if(idx > vector->size){
        *err = EINVAL;
        SpxLog1(vector->log,SpxLogError,"the idx overflow the vector size.");
        return NULL;
    }
    struct spx_vector_node *node = vector->header;
    void *v = NULL;
    size_t i = 0;
    for (i = 0; i < vector->size; i++) {
        if(i == idx){
            v = node->v;
            break;
        }
        node = node->next;
    }
    return v;
}/*}}}*/

err_t spx_vector_push(struct spx_vector *vector,void *v){
    err_t rc = 0;
    struct spx_vector_node *node = NULL;
    node = spx_alloc_alone(sizeof(*node),&rc);
    if(NULL == node){
        SpxLog2(vector->log,SpxLogError,rc,\
                "alloc vector node is fail.");
        return rc;
    }
    node->v = v;
    node->next = NULL;
    if(NULL == vector->header){
        vector->header = node;
        vector->tail = node;
    }else {
        vector->tail->next = node;
        node->prev = vector->tail;
        vector->tail = node;
        vector->size++;
    }
    vector->size++;
    return rc;
}

void *spx_vector_pop(struct spx_vector *vector, err_t *err){
    if(NULL == vector || 0 ==  vector->size){
        *err = EINVAL;
        SpxLog1(vector->log,SpxLogError,"the vector argument is fail.");
        return NULL;
    }

    struct spx_vector_node *node = NULL;
    void *v = NULL;
    if(NULL != vector->header){
        node = vector->header;
        vector->header = node->next;
        if(NULL != vector->header){
            vector->header->prev = NULL;
        }
        v = node->v;
        vector->size --;
        SpxFree(node);
    } else {
        v = NULL;
    }
    if(NULL == vector->header){
        vector->tail = NULL;
    }
    return v;
}

struct spx_vector_iter  *spx_vector_iter_new(struct spx_vector *vector,err_t *err){/*{{{*/
    if(NULL == vector){
        *err = EINVAL;
        return NULL;
    }
    struct spx_vector_iter *iter = NULL;
    iter = spx_alloc_alone(sizeof(*iter),err);
    if(NULL == iter){
        SpxLog2(vector->log,SpxLogError,*err,\
                "allo the vector iter is fail.");
        return NULL;
    }
    iter->vector = vector;
    iter->curr = NULL;
    return iter;
}/*}}}*/

err_t spx_vector_iter_free(struct spx_vector_iter **iter){/*{{{*/
    if(NULL == *iter){
        return 0;
    }
    if(NULL == (*iter)->vector){
        return 0;
    }
    SpxFree(*iter);
    return 0;
}/*}}}*/

void *spx_vector_iter_next(struct spx_vector_iter *iter) {/*{{{*/
    if(NULL == iter){
        return NULL;
    }
    struct spx_vector *vector = iter->vector;
    if(NULL == vector){
        return NULL;
    }
    if(iter->curr == iter->vector->tail){
        return NULL;
    }
    void *v= NULL;
    iter->curr = NULL == iter->curr \
                 ? vector->header : iter->curr->next;
    v = iter->curr->v;
    return v;
}/*}}}*/

void spx_vector_iter_reset(struct spx_vector_iter *iter){/*{{{*/
    if(NULL == iter){
        return;
    }
    iter->curr = NULL;
    return;
}/*}}}*/


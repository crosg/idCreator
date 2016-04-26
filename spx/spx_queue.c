/*
 * =====================================================================================
 *
 * this software or lib may be copied only under the terms of the gnu general
 * public license v3, which may be found in the source kit.
 *
 *       Filename:  spx_queue.c
 *        Created:  2014/08/08 17时53分32秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>

#include "spx_types.h"
#include "spx_queue.h"
#include "spx_alloc.h"
#include "spx_defs.h"

struct spx_queue *spx_queue_new(SpxLogDelegate *log,
        SpxQueueNodeFreeDelegate *free,
        err_t *err){
    struct spx_queue *q = (struct spx_queue *) spx_alloc_alone(sizeof(*q),err);
    if(NULL == q){
        SpxLog2(log,SpxLogError,*err,\
                "alloc the queue is fail.");
        return NULL;
    }
    q->free = free;
    q->log = log;
    return q;
}


void *spx_queue_pop(struct spx_queue *q,err_t *err){
    if(NULL == q->tail){
        *err = ENOENT;
        return NULL;
    }
    struct spx_queue_node *p = NULL;
    do{
        p = q->tail;
    }while(!__sync_bool_compare_and_swap(&(q->tail),p,p->prev));
    void *v = p->v;
    SpxFree(p);
    return v;
}

err_t spx_queue_push(struct spx_queue *q,void *v){
    err_t err = 0;
    struct spx_queue_node *n = (struct spx_queue_node *) \
                               spx_alloc_alone(sizeof(*n),&err);
    if(NULL == n){

    }
    struct spx_queue_node *h = q->header;
    n->v = v;
    if(__sync_bool_compare_and_swap(&h,NULL,n)){
        __sync_bool_compare_and_swap(&(q->tail),NULL,n);
        return 0;
    }

    struct spx_queue_node *p = q->header;
    struct spx_queue_node *old = p;
    int trys = 0;
    do{
        while(3 < trys && NULL != p->prev){
            p = p->prev;
        }
        trys++;
    }while(!__sync_bool_compare_and_swap(&(p->prev),NULL,n));
    __sync_bool_compare_and_swap(&(q->header),old,n);
    return 0;
}


err_t spx_queue_free(struct spx_queue **q){
    err_t err = 0;
    err = spx_queue_clear(*q);
    if(ENOENT == err){
        SpxFree(*q);
    } else{
        SpxLog2((*q)->log,SpxLogError,err,
                "clear queue is fail.");
        return err;
    }
    return err;
}

err_t spx_queue_clear(struct spx_queue *q){
    err_t err = 0;
    void *v = NULL;
    while(NULL != ( v=  spx_queue_pop(q,&err))
            && 0 == err){
        q->free(&v);
    }
    return err;
}

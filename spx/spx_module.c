/*
 * =====================================================================================
 *
 * this software or lib may be copied only under the terms of the gnu general
 * public license v3, which may be found in the source kit.
 *
 *       Filename:  spx_thread_context.c
 *        Created:  2014/07/22 17时19分04秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <ev.h>

#include "spx_types.h"
#include "spx_list.h"
#include "spx_fixed_vector.h"
#include "spx_module.h"
#include "spx_alloc.h"
#include "spx_defs.h"
#include "spx_string.h"
#include "spx_io.h"

struct spx_recvive_context_transport{
    SpxReceiveTriggerDelegate *recviver;
    int event;
    SpxLogDelegate *log;
};

struct spx_thread_pending_transport{
    size_t idx;
    struct spx_module_context *mc;
};


spx_private void *spx_thread_context_new(size_t idx,void *arg,err_t *err);
spx_private err_t spx_thread_context_free(void **arg);
spx_private void *spx_receiver_new(size_t idx,void *arg,err_t *err);
spx_private err_t spx_receiver_free(void **arg);
spx_private void *spx_thread_listening(void *arg);

struct spx_thread_context *spx_thread_context_new_alone(SpxLogDelegate *log,err_t *err){
    struct spx_thread_context *tc = (struct spx_thread_context *) \
                                    spx_alloc_alone(sizeof(*tc),err);
    if(NULL == tc){
        SpxLog2(log,SpxLogError,*err,\
                "alloc thread context is fail.");
        return NULL;
    }
//    tc->loop = ev_loop_new(0);
    tc->log = log;
    return tc;

}

spx_private void *spx_thread_context_new(size_t idx,void *arg,err_t *err){/*{{{*/
    SpxLogDelegate *log = (SpxLogDelegate *) arg;
    struct spx_thread_context *tc = (struct spx_thread_context *) \
                                    spx_alloc_alone(sizeof(*tc),err);
    if(NULL == tc){
        SpxLog2(log,SpxLogError,*err,\
                "alloc thread context is fail.");
        return NULL;
    }
    tc->idx = idx;
    tc->loop = ev_loop_new(0);

    tc->log = log;
    if(-1 == pipe(tc->pipe)){
        *err = errno;
        ev_loop_destroy(tc->loop);
        SpxFree(tc);
        return NULL;
    }
    spx_set_nb(tc->pipe[0]);
    spx_set_nb(tc->pipe[1]);
    return tc;
}/*}}}*/

spx_private err_t spx_thread_context_free(void **arg){/*{{{*/
    struct spx_thread_context **tc = (struct spx_thread_context **) arg;
    ev_break((*tc)->loop,EVBREAK_ALL);
    ev_loop_destroy((*tc)->loop);
    SpxClose((*tc)->pipe[0]);
    SpxClose((*tc)->pipe[1]);
    SpxFree(*tc);
    return 0;
}/*}}}*/


spx_private void *spx_receiver_new(size_t idx,void *arg,err_t *err){/*{{{*/
    struct spx_recvive_context_transport *rct = (struct spx_recvive_context_transport *) arg;
    if(NULL == rct){
        *err = EINVAL;
        return NULL;
    }
    struct spx_receive_context *t = (struct spx_receive_context *) \
                                    spx_alloc_alone(sizeof(*t),err);
    if(NULL == t){
        SpxLog2(rct->log,SpxLogError,*err,\
                "alloc trigger context is fail.");
        return NULL;
    }
    t->log = rct->log;
    t->idx = idx;
    t->receive_handler = rct->recviver;
    return t;
}/*}}}*/

spx_private err_t spx_receiver_free(void **arg){/*{{{*/
    struct spx_receive_context **t = (struct spx_receive_context **) arg;
    SpxFree(*t);
    return 0;
}/*}}}*/


spx_private void *spx_thread_listening(void *arg){/*{{{*/
    struct spx_thread_pending_transport *tpt = (struct spx_thread_pending_transport *) arg;
    size_t idx = tpt->idx;
    struct spx_module_context *mc = tpt->mc;
    SpxFree(tpt);//free the memory
    struct spx_receive_context *tc = spx_list_get(mc->receive_triggers,idx);
    struct spx_thread_context *stc = spx_list_get(mc->threadpool,idx);
    ev_io_init(&(tc->watcher),tc->receive_handler,stc->pipe[0],EV_READ);
     ev_io_start(stc->loop,&(tc->watcher));
    ev_run(stc->loop,0);
    return NULL;
}/*}}}*/

struct spx_module_context *spx_module_new(\
        SpxLogDelegate *log,\
        u32_t threadsize,\
        size_t stack_size,
        SpxReceiveTriggerDelegate *receive_handler,
        err_t *err){/*{{{*/
    struct spx_module_context *mc = (struct spx_module_context *)\
                                    spx_alloc_alone(sizeof(*mc),err);
    if(NULL == mc){
        SpxLog2(log,SpxLogError,*err,\
                "alloc module context is fail.");
        return NULL;
    }

    mc->log = log;
    mc->threadpool = spx_list_init(log,\
            threadsize,\
            spx_thread_context_new,\
            log,\
            spx_thread_context_free,\
            err);
    if(NULL == mc->threadpool){
        SpxLog2(log,SpxLogError,*err,\
                "alloc threadpool for module is fail.");
        goto r2;
    }

    struct spx_recvive_context_transport rct;
    SpxZero(rct);

    rct.log = log;
    rct.event = EV_READ;
    rct.recviver = receive_handler;;
    mc->receive_triggers = spx_list_init(log,\
            threadsize,\
            spx_receiver_new,\
            &rct,\
            spx_receiver_free,\
            err);
    if(NULL == mc->receive_triggers){
        SpxLog2(log,SpxLogError,*err,\
                "alloc receive triggers are fail.");
        goto r2;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    size_t ostack_size = 0;
    pthread_attr_getstacksize(&attr, &ostack_size);
    do{
        if (ostack_size != stack_size
                && (0 != (*err = pthread_attr_setstacksize(&attr,stack_size)))){
            SpxLog2(log,SpxLogError,*err,\
                    "set thread stack size is fail.");
            goto r1;
        }
        u32_t i = 0;
        struct spx_thread_pending_transport *tpt;
        for( ; i < threadsize; i++){
            tpt = spx_alloc_alone(sizeof(*tpt),err);
            tpt->mc = mc;
            tpt->idx = i;
            struct spx_thread_context *n = spx_list_get(mc->threadpool,i);
            if (0 !=(*err =  pthread_create(&(n->tid), &attr, spx_thread_listening,
                            tpt))){
                SpxLog2(log,SpxLogError,*err,\
                        "create nio thread is fail.");
                goto r1;
            }
        }
    }while(false);
    pthread_attr_destroy(&attr);
    return mc;
r1:
    pthread_attr_destroy(&attr);
r2:
    spx_module_free(&mc);
    return NULL;
}/*}}}*/

err_t spx_module_free(struct spx_module_context **mc){
    //must free thread pool first
    if(NULL != (*mc)->threadpool){
        spx_list_free(&((*mc)->threadpool));
    }
    if(NULL != (*mc)->receive_triggers) {
        spx_list_free(&((*mc)->receive_triggers));
    }
    SpxFree(*mc);
    return 0;
}



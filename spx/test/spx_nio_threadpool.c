/*
 * =====================================================================================
 *
 * this software or lib may be copied only under the terms of the gnu general
 * public license v3, which may be found in the source kit.
 *
 *       Filename:  spx_nio_threadpool.c
 *        Created:  2014/07/21 17时14分27秒
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

#include "include/spx_types.h"
#include "include/spx_alloc.h"
#include "include/spx_defs.h"
#include "include/spx_nio_threadpool.h"
#include "include/spx_list.h"
#include "include/spx_string.h"
#include "include/spx_io.h"

struct spx_thread_context_node{
    SpxLogDelegate *log;
    SpxThreadNotifyDelegate *thread_notify;
};

spx_private void *spx_nio_thread_listen(void *arg);
spx_private void *spx_nio_thread_context_new(size_t idx,void *arg,err_t *err);
spx_private err_t spx_nio_thread_context_free(void **arg);

spx_private void *spx_nio_thread_listen(void *arg){
    struct spx_nio_thread_context *context = (struct spx_nio_thread_context *) arg;
    if(NULL == context){
        return NULL;
    }
    ev_io_init(&(context->watcher),context->thread_notify_handle,context->pipe[0],EV_READ);
    context->watcher.data = context;//libev not the set function
    ev_io_start(context->loop,&(context->watcher));
    ev_run(context->loop,0);
    return NULL;
}

spx_private void *spx_nio_thread_context_new(size_t idx,void *arg,err_t *err){
    struct spx_thread_context_node *tcn = (struct spx_thread_context_node *)arg;
    struct spx_nio_thread_context *context = spx_alloc_alone(sizeof(*context),err);
    if(NULL == context){
        SpxLog2(tcn->log,SpxLogError,*err,\
                "alloc nio thread context is fail.");
        return NULL;
    }

    context->loop = ev_loop_new(EVFLAG_AUTO);
    context->log = tcn->log;
    context->idx = idx;
    context->thread_notify_handle = tcn->thread_notify;
    if(-1 == pipe(context->pipe)){
        SpxLog2(tcn->log,SpxLogError,*err,\
                "open the nio thread pips is fail.");
        *err = errno;
        SpxFree(context);
        return NULL;
    }
    if((0 != (*err = spx_set_nb(context->pipe[0]))) \
            ||(0 != (*err = spx_set_nb(context->pipe[0])))){
        SpxLog2(tcn->log,SpxLogError,*err,\
                "set pipe noblacking is fail.");
        SpxFree(context);
        return NULL;
    }
    return context;
}

spx_private err_t spx_nio_thread_context_free(void **arg){
    struct spx_nio_thread_context **context = (struct spx_nio_thread_context **) arg;
    ev_break((*context)->loop,EVBREAK_ALL);
    ev_loop_destroy((*context)->loop);
    SpxClose((*context)->pipe[0]);
    SpxClose((*context)->pipe[1]);
    SpxFree(*context);
    return 0;
}

struct spx_list *spx_nio_threadpool_create(\
        SpxLogDelegate *log,\
        u32_t threadsize,\
        size_t stack_size,\
        SpxThreadNotifyDelegate *thread_notify_handle,\
        err_t *err){
    struct spx_thread_context_node tcn;
    SpxZero(tcn);
    tcn.thread_notify = thread_notify_handle;
    tcn.log = log;
    struct spx_list *nio_thread_contexts = spx_list_init(log,threadsize,\
            spx_nio_thread_context_new,&tcn,\
            spx_nio_thread_context_free,\
            err);
    if(0 == threadsize){
        SpxLog1(log,SpxLogError,\
                "the argument:threadsize is 0");
        return NULL;
    }

    if(NULL == nio_thread_contexts){
        SpxLog2(log,SpxLogError,*err,\
                "alloc nio thread context is fail.");
        return NULL;
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
        for( ; i < threadsize; i++){
            struct spx_nio_thread_context * n = spx_list_get(nio_thread_contexts,i);
            n->log = log;
            if (0 !=(*err =  pthread_create(&(n->tid), &attr, spx_nio_thread_listen,
                            n))){
                SpxLog2(log,SpxLogError,*err,\
                        "create nio thread is fail.");
                goto r1;
            }
        }
    }while(false);
    pthread_attr_destroy(&attr);
    return nio_thread_contexts;
r1:
    pthread_attr_destroy(&attr);
    spx_list_free(&nio_thread_contexts);
    return NULL;
}

err_t spx_nio_threadpool_free(struct spx_list **nio_thread_contexts){
    return spx_list_free(nio_thread_contexts);
}


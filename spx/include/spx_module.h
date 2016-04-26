/*
 * =====================================================================================
 *
 * this software or lib may be copied only under the terms of the gnu general
 * public license v3, which may be found in the source kit.
 *
 *       Filename:  spx_thread_context.h
 *        Created:  2014/07/22 16时09分11秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 * =====================================================================================
 */
#ifndef _SPX_MODULE_H_
#define _SPX_MODULE_H_
#ifdef __cplusplus
extern "C" {
#endif


#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <ev.h>

#include "spx_types.h"
#include "spx_list.h"
#include "spx_fixed_vector.h"


typedef void (SpxReceiveTriggerDelegate)(struct ev_loop *loop,ev_io *watcher,int revents);
typedef void (SpxDispatchTriggerDelegate)(int revents,void *arg);

struct spx_thread_context{
    struct ev_loop *loop;
    SpxLogDelegate *log;
    size_t idx;
    pthread_t tid;
    int pipe[2];
    void *c;// add the config info at do dsync
};

struct spx_module_context{
    struct spx_list *threadpool;
    //usually,receive notification watcher size is equal to thread size
    //but dispatch notice watcher size maybe hanving to more than thread size,
    //because,receive watcher follow loop and banding with loop by 1:1
    //dispatch watcher is maybe n:1 banding with loop when loop is busy
    //so we set the n equal 2,donot ask me why,this is a magic
    struct spx_list *receive_triggers;
    SpxLogDelegate *log;
};


struct spx_receive_context{
    ev_io watcher;
    SpxReceiveTriggerDelegate *receive_handler;
    SpxLogDelegate *log;
    size_t idx;
};



//today is 2014-07-22,I say to xj about the ydb work over this month,
//but now,I make a mistaken and redo thread-notify,so I can not over the work.
//if he kown this,he want to kill me.is not it??

struct spx_module_context *spx_module_new(\
        SpxLogDelegate *log,\
        u32_t threadsize,\
        size_t stack_size,\
        SpxReceiveTriggerDelegate *receive_handler,\
        err_t *err);

err_t spx_module_free(struct spx_module_context **mc);

//void spx_module_dispatch(SpxDispatchTriggerDelegate *dispatcher,void *msg);

#define SpxModuleDispatch(dispatcher,msg) dispatcher(EV_WRITE,msg)

spx_private struct spx_thread_context *spx_get_thread(struct spx_module_context *mc,size_t idx){
    struct spx_thread_context *stc = spx_list_get(mc->threadpool,idx);
    return stc;
}

struct spx_thread_context *spx_thread_context_new_alone(SpxLogDelegate *log,err_t *err);
#ifdef __cplusplus
}
#endif
#endif

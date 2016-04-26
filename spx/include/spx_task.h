/*
 * =====================================================================================
 *
 * this software or lib may be copied only under the terms of the gnu general
 * public license v3, which may be found in the source kit.
 *
 *       Filename:  spx_dio_context.h
 *        Created:  2014/07/24 17时53分07秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 * =====================================================================================
 */
#ifndef _SPX_TASK_H_
#define _SPX_TASK_H_
#ifdef __cplusplus
extern "C" {
#endif


#include <stdlib.h>
#include <stdio.h>
#include <ev.h>

#include "spx_types.h"
#include "spx_defs.h"
#include "spx_message.h"
#include "spx_properties.h"


    //play a trick
    //and look at me how to implemant sub-calss
    //it must be putted into sub-class with begin.
    //if you do not understand,please no modifition.
#define SpxDioMembers \
    int fd

//    struct spx_dio_file{
//        SpxDioMembers;
//    };

    struct spx_task_context;
    typedef void (SpxDioDelegate)(struct ev_loop *loop,ev_io *watcher,int revents);
    typedef err_t (SpxDioProcessDelegate)(struct ev_loop *loop,int idx,struct spx_task_context *tcontext);

    struct spx_task_context{
        ev_io watcher;
        size_t idx;
        size_t tidx;
        err_t err;
        //the memory means balcking is true
        //and the callback(dio_handler) must keep
        //monopolizing the sharing resource
        //and the noblacking io(Inc.disk io) means false
//        bool_t blacking;
//        SpxDioDelegate *dio_handler;
        SpxDioProcessDelegate *dio_process_handler;
        SpxLogDelegate *log;
        struct spx_job_context *jcontext;
//        int events;
    };

    struct spx_task_context_transport{
        SpxLogDelegate *log;
        SpxDioProcessDelegate *dio_process_handler;
    };

    struct spx_task_pool {
        SpxLogDelegate *log;
        struct spx_fixed_vector *pool;
    };

    extern struct spx_task_pool *g_spx_task_pool;

    void *spx_task_context_new(size_t idx,void *arg,err_t *err);
    err_t spx_task_context_free(void **arg);
    void spx_task_context_clear(struct spx_task_context *tcontext);

struct spx_task_pool *spx_task_pool_new(\
        SpxLogDelegate *log,\
        size_t size,\
        SpxDioProcessDelegate *dio_process_handler,\
        err_t *err);

    struct spx_task_context *spx_task_pool_pop(\
            struct spx_task_pool *pool,\
            err_t *err);

    err_t spx_task_pool_push(\
            struct spx_task_pool *pool,\
            struct spx_task_context *tcontext);

    err_t spx_task_pool_free(\
            struct spx_task_pool **pool);




#ifdef __cplusplus
}
#endif
#endif

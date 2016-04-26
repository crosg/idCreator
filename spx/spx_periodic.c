/*************************************************************
 *                     _ooOoo_
 *                    o8888888o
 *                    88" . "88
 *                    (| -_- |)
 *                    O\  =  /O
 *                 ____/`---'\____
 *               .'  \\|     |//  `.
 *              /  \\|||  :  |||//  \
 *             /  _||||| -:- |||||-  \
 *             |   | \\\  -  /// |   |
 *             | \_|  ''\---/''  |   |
 *             \  .-\__  `-`  ___/-. /
 *           ___`. .'  /--.--\  `. . __
 *        ."" '<  `.___\_<|>_/___.'  >'"".
 *       | | :  `- \`.;`\ _ /`;.`/ - ` : | |
 *       \  \ `-.   \_ __\ /__ _/   .-` /  /
 *  ======`-.____`-.___\_____/___.-`____.-'======
 *                     `=---='
 *  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *           佛祖保佑       永无BUG
 *
 * ==========================================================================
 *
 * this software or lib may be copied only under the terms of the gnu general
 * public license v3, which may be found in the source kit.
 *
 *       Filename:  spx_periodic.c
 *        Created:  2014/10/22 10时44分15秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 ***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#include "spx_alloc.h"
#include "spx_types.h"
#include "spx_defs.h"
#include "spx_periodic.h"
#include "spx_atomic.h"
#include "spx_thread.h"
#include "spx_periodic.h"


spx_private void *spx_periodic_run_async(void *arg);
spx_private void *spx_periodic_exec_and_run_async(void *arg);

spx_private void *spx_periodic_run_async(void *arg){/*{{{*/
    SpxTypeConvert2(struct spx_periodic,t,arg);
    t->status = SpxperiodicRunning;
    struct timeval timespan;
    do{
        if(SpxAtomicVCas(t->status,SpxperiodicSetPausing,SpxperiodicPausing)){
            pthread_mutex_lock(t->mlock);
            pthread_cond_wait(t->clock,t->mlock);
            pthread_mutex_unlock(t->mlock);
        }
        timespan.tv_sec = t->sec;
        timespan.tv_usec = t->usec;
        pthread_testcancel();
        t->err = select(0,NULL,NULL,NULL,&timespan);
        pthread_testcancel();
        if(0 > t->err) {
            SpxLog2(t->log,SpxLogError,t->err,
                    "async periodic by select is error.");
        } else if(0 == t->err) {
            pthread_testcancel();
            t->periodic_handler(t->arg);
            pthread_testcancel();
        } else {
            if(!(EAGAIN == errno || EWOULDBLOCK == errno)) {
                SpxLog2(t->log,SpxLogError,t->err,
                        "async periodic by select is error.");
            }
        }
    }while(t->is_run);
    return NULL;
}/*}}}*/

spx_private void *spx_periodic_exec_and_run_async(void *arg){/*{{{*/
    SpxTypeConvert2(struct spx_periodic,t,arg);
    t->status = SpxperiodicRunning;
    t->periodic_handler(t->arg);
    struct timeval timespan;
    do{
        if(SpxAtomicVCas(t->status,SpxperiodicSetPausing,SpxperiodicPausing)){
            pthread_mutex_lock(t->mlock);
            pthread_cond_wait(t->clock,t->mlock);
            pthread_mutex_unlock(t->mlock);
        }
        timespan.tv_sec = t->sec;
        timespan.tv_usec = t->usec;
        pthread_testcancel();
        t->err = (select(0,NULL,NULL,NULL,&timespan));
        pthread_testcancel();
        if(0 > t->err) {
            SpxLog2(t->log,SpxLogError,t->err,
                    "async periodic by select is error.");
        } else if(0 == t->err) {
            pthread_testcancel();
            t->periodic_handler(t->arg);
            pthread_testcancel();
        } else {
            if(!(EAGAIN == errno || EWOULDBLOCK == errno)) {
                SpxLog2(t->log,SpxLogError,t->err,
                        "async periodic by select is error.");
            }
        }
    }while(t->is_run);
    return NULL;
}/*}}}*/

    void spx_periodic_sleep(int sec,int usec){
    struct timeval timespan;
    timespan.tv_sec = sec;
    timespan.tv_usec = usec;
    select(0,NULL,NULL,NULL,&timespan);
    return;
}

void spx_periodic_run(SpxLogDelegate *log,
        u32_t secs,u64_t usecs,
        SpxPeriodicDelegate *periodic_handler,
        void *arg,
        err_t *err){/*{{{*/
    if(NULL == periodic_handler
            || (0 == secs && 0 == usecs)){
        *err = EINVAL;
        return;
    }
    struct timeval timespan;
    while(true) {
        timespan.tv_sec = secs;
        timespan.tv_usec = usecs;
        *err = (select(0,NULL,NULL,NULL,&timespan));
        if(0 > *err) {
            SpxLog2(log,SpxLogError,*err,
                    "periodic by select is error.");
        } else if(0 == *err) {
            periodic_handler(arg);
        } else {
            if(!(EAGAIN == errno || EWOULDBLOCK == errno)) {
                SpxLog2(log,SpxLogError,*err,
                        "periodic by select is error.");
            }
        }
    }
}/*}}}*/

void spx_periodic_run_once(SpxLogDelegate *log,
        u32_t secs,u64_t usecs,
        SpxPeriodicDelegate *periodic_handler,
        void *arg,
        err_t *err){/*{{{*/
    if(NULL == periodic_handler
            || (0 == secs && 0 == usecs)){
        *err = EINVAL;
        return;
    }
    struct timeval timespan;
    timespan.tv_sec = secs;
    timespan.tv_usec = usecs;
    *err = (select(0,NULL,NULL,NULL,&timespan));
    if(0 > *err) {
        SpxLog2(log,SpxLogError,*err,
                "periodic by select is error.");
    } else if(0 == *err) {
        periodic_handler(arg);
    } else {
        if(!(EAGAIN == errno || EWOULDBLOCK == errno)) {
            SpxLog2(log,SpxLogError,*err,
                    "periodic by select is error.");
        }
    }
}/*}}}*/

void spx_periodic_exec_and_run(SpxLogDelegate *log,
        u32_t secs,u64_t usecs,
        SpxPeriodicDelegate *periodic_handler,
        void *arg,
        err_t *err){/*{{{*/
    if(NULL == periodic_handler
            || (0 == secs && 0 == usecs)){
        *err = EINVAL;
        return;
    }
    periodic_handler(arg);
    spx_periodic_run(log,secs,usecs,periodic_handler,arg,err);
}/*}}}*/


struct spx_periodic *spx_periodic_async_run(SpxLogDelegate *log,
        u32_t secs,u64_t usecs,
        SpxPeriodicDelegate *periodic_handler,
        void *arg,
        size_t stacksize,
        err_t *err){/*{{{*/
    if(NULL == periodic_handler
            || (0 == secs && 0 == usecs)){
        *err = EINVAL;
        return NULL;
    }
    struct spx_periodic *t = (struct spx_periodic *)
        spx_alloc_alone(sizeof(*t),err);
    if(NULL == t) {
        return NULL;
    }
    t->is_run = true;
    t->sec = secs;
    t->usec = usecs;
    t->log = log;
    t->arg = arg;
    t->periodic_handler = periodic_handler;
    t->mlock = spx_thread_mutex_new(log,err);
    if(NULL == t->mlock){
        goto r1;
    }
    t->clock = spx_thread_cond_new(log,err);
    if(NULL == t->clock){
        goto r1;
    }

    t->tid = spx_thread_new_cancelability(log,
            stacksize,spx_periodic_run_async,(void *) t,err);
    if(0 == t->tid){
        goto r1;
    }
    return t;
r1:
    if(NULL != t->clock){
        spx_thread_cond_free(&(t->clock));
    }
    if(NULL != t->mlock){
        spx_thread_mutex_free(&(t->mlock));
    }
    if(NULL != t){
        SpxFree(t);
    }
    return NULL;
}/*}}}*/

struct spx_periodic *spx_periodic_async_run_once(SpxLogDelegate *log,
        u32_t secs,u64_t usecs,
        SpxPeriodicDelegate *periodic_handler,
        void *arg,
        size_t stacksize,
        err_t *err){/*{{{*/
    if(NULL == periodic_handler
            || (0 == secs && 0 == usecs)){
        *err = EINVAL;
        return NULL;
    }
    struct spx_periodic *t = (struct spx_periodic *)
        spx_alloc_alone(sizeof(*t),err);
    if(NULL == t) {
        return NULL;
    }
    t->is_run = false;
    t->sec = secs;
    t->usec = usecs;
    t->log = log;
    t->arg = arg;
    t->periodic_handler = periodic_handler;
    t->mlock = spx_thread_mutex_new(log,err);
    if(NULL == t->mlock){
        goto r1;
    }
    t->clock = spx_thread_cond_new(log,err);
    if(NULL == t->clock){
        goto r1;
    }

    t->tid = spx_thread_new_cancelability(log,
            stacksize,spx_periodic_run_async,(void *) t,err);
    if(0 == t->tid){
        goto r1;
    }
    return t;
r1:
    if(NULL != t->clock){
        spx_thread_cond_free(&(t->clock));
    }
    if(NULL != t->mlock){
        spx_thread_mutex_free(&(t->mlock));
    }
    if(NULL != t){
        SpxFree(t);
    }
    return NULL;
}/*}}}*/

struct spx_periodic *spx_periodic_async_exec_and_run(SpxLogDelegate *log,
        u32_t secs,u64_t usecs,
        SpxPeriodicDelegate *periodic_handler,
        void *arg,
        size_t stacksize,
        err_t *err){/*{{{*/
    if(NULL == periodic_handler
            || (0 == secs && 0 == usecs)){
        *err = EINVAL;
        return NULL;
    }
    struct spx_periodic *t = (struct spx_periodic *)
        spx_alloc_alone(sizeof(*t),err);
    if(NULL == t) {
        return NULL;
    }
    t->is_run = true;
    t->sec = secs;
    t->usec = usecs;
    t->log = log;
    t->arg = arg;
    t->periodic_handler = periodic_handler;
    t->mlock = spx_thread_mutex_new(log,err);
    if(NULL == t->mlock){
        goto r1;
    }
    t->clock = spx_thread_cond_new(log,err);
    if(NULL == t->clock){
        goto r1;
    }

    t->tid = spx_thread_new_cancelability(log,
            stacksize,spx_periodic_run_async,(void *) t,err);
    if(0 == t->tid){
        goto r1;
    }
    return t;
r1:
    if(NULL != t->clock){
        spx_thread_cond_free(&(t->clock));
    }
    if(NULL != t->mlock){
        spx_thread_mutex_free(&(t->mlock));
    }
    if(NULL != t){
        SpxFree(t);
    }
    return NULL;
}/*}}}*/

struct spx_periodic *spx_periodic_exec_and_async_run(SpxLogDelegate *log,
        u32_t secs,u64_t usecs,
        SpxPeriodicDelegate *periodic_handler,
        void *arg,
        size_t stacksize,
        err_t *err){/*{{{*/
    if(NULL == periodic_handler
            || (0 == secs && 0 == usecs)){
        *err = EINVAL;
        return NULL;
    }
    periodic_handler(arg);
    return spx_periodic_async_run(log,secs,usecs,periodic_handler,arg,stacksize,err);
}/*}}}*/

bool_t spx_periodic_async_suspend(struct spx_periodic *t){/*{{{*/
    return SpxAtomicVCas(t->status,SpxperiodicRunning,SpxperiodicSetPausing);
}/*}}}*/

bool_t spx_periodic_async_resume(struct spx_periodic *t){/*{{{*/
    if(SpxAtomicVCas(t->status,SpxperiodicSetPausing,SpxperiodicRunning)){
        return true;
    }else {
        if(SpxAtomicVCas(t->status,SpxperiodicPausing,SpxperiodicRunning)){
            pthread_mutex_lock(t->mlock);
            pthread_cond_signal(t->clock);
            pthread_mutex_unlock(t->mlock);
            return true;
        }
    }
    return false;
}/*}}}*/

void spx_periodic_stop(
        struct spx_periodic **periodic,
        bool_t isblocking
        ){/*{{{*/
    struct spx_periodic *t = *periodic;
    SpxAtomicVSet(t->is_run,false);
    pthread_cancel(t->tid);
    if(isblocking){
        pthread_join(t->tid,NULL);
    }
    if(SpxperiodicSetPausing == t->status
            || SpxperiodicPausing == t->status){
        spx_periodic_async_resume(t);
    }
    pthread_join(t->tid,NULL);
    if(NULL != t->mlock){
        spx_thread_mutex_free(&(t->mlock));
    }
    if(NULL != t->clock){
        spx_thread_cond_free(&(t->clock));
    }
    SpxFree(*periodic);
}/*}}}*/

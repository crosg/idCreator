/*
 * =====================================================================================
 *
 * this software or lib may be copied only under the terms of the gnu general
 * public license v3, which may be found in the source kit.
 *
 *       Filename:  spx_threadpool.h
 *        Created:  2014/09/01 11时41分08秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 *
 * =====================================================================================
 */

#ifndef _SPX_THREADPOOL_H_
#define _SPX_THREADPOOL_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "spx_types.h"
#include "spx_list.h"

typedef void (SpxThreadpoolExecuteDelegate)(void *);

enum spx_pooling_thread_state {
	uninitialized, initializing, initialized, uninstalling, uninstalled,
};

struct spx_threadpool;

struct spx_thread {
    SpxLogDelegate *log;
	pthread_t id;
	pthread_mutex_t mutex_locker;
	pthread_cond_t run_locker;
	SpxThreadpoolExecuteDelegate *func;
	struct spx_threadpool *parent;
	void *arg;
	struct spx_thread *next;
} thread_t;

struct spx_threadpool {
    SpxLogDelegate *log;
//    struct spx_thread **threads;
    struct spx_thread *busy;
    struct spx_thread *free;
	pthread_mutex_t mutex_locker;
	pthread_cond_t run_locker;
	pthread_cond_t full_locker;
	pthread_cond_t empty_locker;
	enum spx_pooling_thread_state state;
	size_t totalsize;
    size_t currsize;
    size_t thread_stack_size;
    size_t waits;
//	size_t idx;
};

struct spx_threadpool *spx_threadpool_new(SpxLogDelegate *log,
        size_t max,size_t thread_stack_size,err_t *err);
err_t spx_threadpool_execute(struct spx_threadpool *p,
        SpxThreadpoolExecuteDelegate *func,void *arg);
err_t spx_threadpool_free(struct spx_threadpool **p);





#ifdef __cplusplus
}
#endif
#endif

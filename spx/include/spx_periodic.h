#ifndef _SPX_PERIODIC_H_
#define _SPX_PERIODIC_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include"spx_types.h"

    /**
     * Remark: periodic is not the same with timeout-timer.
     * this periodic is not acuracy,because periodic and callback
     * in the same poll,and if callback is blocking or the time-conuming
     * handler,such as:disk-io,network io,bigger-compute...
     * they are all need more cpu time to deal.
     * so if you want to the periodic become acuracy,you maybe having noblocking.
     * and if you want to control everyting and you are not er-ba-dao in C,
     * you can use spx_sleep or maked select DIY.
     * well,usually,the periodic handler execute is later than the appointed time.
     * if you use functions by named *aync*,you will create a pthread
     *  to deal async.
     * */

    typedef void *(SpxPeriodicDelegate)(void *arg);

#define SpxperiodicRunning 0
#define SpxperiodicSetPausing 1
#define SpxperiodicPausing 2

    struct spx_periodic{
        SpxLogDelegate *log;
        SpxPeriodicDelegate *periodic_handler;
        u32_t sec;
        u64_t usec;
        err_t err;
        bool_t is_run;
        i32_t status;
        void *arg;
        pthread_t tid;
        pthread_mutex_t *mlock;
        pthread_cond_t *clock;
    };


    void spx_periodic_sleep(int sec,int usec);

    void spx_periodic_run(SpxLogDelegate *log,
            u32_t secs,u64_t usecs,
            SpxPeriodicDelegate *timeout_handler,
            void *arg,
            err_t *err);

    struct spx_periodic *spx_periodic_async_run(SpxLogDelegate *log,
            u32_t secs,u64_t usecs,
            SpxPeriodicDelegate *timeout_handler,
            void *arg,
            size_t stacksize,
            err_t *err);

    void spx_periodic_run_once(SpxLogDelegate *log,
            u32_t secs,u64_t usecs,
            SpxPeriodicDelegate *timeout_handler,
            void *arg,
            err_t *err);

    struct spx_periodic *spx_periodic_async_run_once(SpxLogDelegate *log,
            u32_t secs,u64_t usecs,
            SpxPeriodicDelegate *timeout_handler,
            void *arg,
            size_t stacksize,
            err_t *err);

    void spx_periodic_exec_and_run(SpxLogDelegate *log,
            u32_t secs,u64_t usecs,
            SpxPeriodicDelegate *timeout_handler,
            void *arg,
            err_t *err);

    struct spx_periodic *spx_periodic_async_exec_and_run(SpxLogDelegate *log,
            u32_t secs,u64_t usecs,
            SpxPeriodicDelegate *timeout_handler,
            void *arg,
            size_t stacksize,
            err_t *err);

    struct spx_periodic *spx_periodic_exec_and_async_run(SpxLogDelegate *log,
            u32_t secs,u64_t usecs,
            SpxPeriodicDelegate *timeout_handler,
            void *arg,
            size_t stacksize,
            err_t *err);


    /**
     * pausing the periodic at next poll
     */
    bool_t spx_periodic_async_suspend(struct spx_periodic *t);
    bool_t spx_periodic_async_resume(struct spx_periodic *t);

    void spx_periodic_stop(
            struct spx_periodic **periodic,
            bool_t isblocking);

#ifdef __cplusplus
}
#endif
#endif

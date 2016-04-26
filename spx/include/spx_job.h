/*
 * =====================================================================================
 *
 *       Filename:  spx_job.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2014/06/09 17时43分11秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#ifndef _SPX_JOB_H_
#define _SPX_JOB_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <ev.h>

#include "spx_types.h"
#include "spx_defs.h"
#include "spx_message.h"
#include "spx_properties.h"
#include "spx_module.h"

#define SpxNioLifeCycleNormal 0
#define SpxNioLifeCycleHeader 1
#define SpxNioLifeCycleBody 2
#define SpxNioLifeCycleFile 3

#define SpxNioMooreNormal 0
#define SpxNioMooreRequest 1
#define SpxNioMooreResponse 2

    extern struct spx_job_pool *g_spx_job_pool;

    struct spx_job_context;
    //    typedef void (SpxNioDelegate)(struct ev_loop *loop,ev_io *watcher,int revents);
    typedef void (SpxNioDelegate)(int revents,void *arg);
    typedef bool_t (SpxVerifyHeaderDelegate)(struct spx_job_context *jc);
    typedef bool_t (SpxBeforeRecviveBodyDelegate)(struct spx_job_context *jc);
    typedef void (SpxDisposeDelegate)(struct spx_job_context *jc);


    //    typedef void (SpxNioBodyProcessDelegate)(struct ev_loop *loop,
    //            int fd,struct spx_job_context *jcontext);
    //    typedef bool_t (SpxNioHeaderValidatorDelegate)(struct spx_job_context *jcontext);
    //    typedef void (SpxNioHeaderValidatorFailDelegate)(struct spx_job_context *jcontext);
    //    typedef void (SpxNotifyDelegate)(ev_io *watcher,int revents);
    //    typedef bool_t (SpxNioBodyProcessBeforeDelegate)(struct spx_job_context *jcontext);
    //    typedef void (SpxNioBodyProcessAfterDelegate)(struct spx_job_context *jcontext);


    struct spx_job_context_transport{
        u32_t timeout;
        u32_t waitting;
        u32_t trytimes;
        size_t pooling_size;
        size_t mbuff_size;
        size_t keep_mbuff_count;
        SpxNioDelegate *nio_reader;
        SpxNioDelegate *nio_writer;
        SpxVerifyHeaderDelegate *verifyHeaderHandler;
        SpxBeforeRecviveBodyDelegate *beforeRecviveBodyHandler;
        SpxDisposeDelegate *disposeReaderHandler;
        SpxDisposeDelegate *disposeWriterHandler;
        //        SpxNioHeaderValidatorDelegate *reader_header_validator;
        //        SpxNioHeaderValidatorFailDelegate *reader_header_validator_fail;
        //        SpxNioBodyProcessDelegate *reader_body_process;
        //        SpxNioBodyProcessDelegate *writer_body_process;
        //use for judge enable lazy-recv
        //        SpxNioBodyProcessBeforeDelegate *reader_body_process_before;
        void *config;
        SpxLogDelegate *log;
    };

    struct spx_job_context{
        //        ev_io watcher;
        struct spx_mpool *mpool;
        int fd;
        int use;
        size_t idx;
        err_t err;
        u32_t timeout;
        size_t offset;
        u32_t curr_trytimes;
        u32_t trytimes;
        u32_t waitting;
        //        ev_timer timer;
        SpxNioDelegate *nio_writer;
        SpxNioDelegate *nio_reader;
        SpxLogDelegate *log;
        u32_t lifecycle;
        u32_t moore;
        struct spx_thread_context *tc;

        struct spx_msg_header *reader_header;
        struct spx_msg *reader_header_ctx;
        struct spx_msg *reader_body_ctx;
        struct spx_msg_header *writer_header;
        struct spx_msg *writer_header_ctx;
        struct spx_msg *writer_body_ctx;

        SpxVerifyHeaderDelegate *verifyHeaderHandler;
        SpxBeforeRecviveBodyDelegate *beforeRecviveBodyHandler;
        SpxDisposeDelegate *disposeReaderHandler;
        SpxDisposeDelegate *disposeWriterHandler;

        //        SpxNioHeaderValidatorDelegate *reader_header_validator;
        //        SpxNioHeaderValidatorFailDelegate *reader_header_validator_fail;
        //        SpxNioBodyProcessBeforeDelegate *reader_body_process_before;
        //        SpxNioBodyProcessDelegate *reader_body_process;
        //        SpxNioBodyProcessDelegate *writer_body_process;

        string_t client_ip;
        time_t request_timespan;

        void *config;
        void *data;//all client data,but the job is not manager

        /*
         * if lazy recv,must set the offset in the header by client
         * and the part of recved must in the end of the body
         */
        bool_t is_lazy_recv;

        bool_t is_sendfile;
        int sendfile_fd;
        off_t sendfile_begin;
        size_t sendfile_size;

        struct spx_job_context *prev;
        struct spx_job_context *next;

    };

    struct spx_job_pool {
        SpxLogDelegate *log;
        struct spx_job_context *header;
        struct spx_job_context *tail;
        struct spx_job_context *busy_header;
        struct spx_job_context *busy_tail;
        size_t size;
        size_t busysize;
        pthread_mutex_t *locker;
        //        struct spx_fixed_vector *pool;
    };

    void *spx_job_context_new(size_t idx,void *arg,err_t *err);
    err_t spx_job_context_free(void **arg);
    void spx_job_context_clear(struct spx_job_context *jcontext);
    void spx_job_context_reset(struct spx_job_context *jcontext);

    struct spx_job_pool *spx_job_pool_new(SpxLogDelegate *log,
            void *config,
            size_t size,u32_t timeout,u32_t waitting,u32_t trytimes,
            SpxNioDelegate *nio_reader,
            SpxNioDelegate *nio_writer,
            SpxVerifyHeaderDelegate *verifyHeaderHandler,
            SpxBeforeRecviveBodyDelegate *beforeRecviveBodyHandler,
            SpxDisposeDelegate *disposeReaderHandler,
            SpxDisposeDelegate *disposeWriterHandler,
            err_t *err);

    struct spx_job_context *spx_job_pool_pop(struct spx_job_pool *pool,err_t *err);
    err_t spx_job_pool_push(struct spx_job_pool *pool,struct spx_job_context *jcontext);
    err_t spx_job_pool_free(struct spx_job_pool **pool);


#ifdef __cplusplus
}
#endif
#endif

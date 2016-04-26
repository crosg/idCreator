/*
 * =====================================================================================
 *
 *       Filename:  spx_job_context_.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2014/06/09 17时42分46秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#include <stdlib.h>

#include "spx_defs.h"
#include "spx_alloc.h"
#include "spx_fixed_vector.h"
#include "spx_errno.h"
#include "spx_string.h"
#include "spx_message.h"
#include "spx_job.h"
#include "spx_time.h"
#include "spx_thread.h"


struct spx_job_pool *g_spx_job_pool = NULL;

void *spx_job_context_new(size_t idx,void *arg,err_t *err){/*{{{*/
    struct spx_job_context_transport *jct = (struct spx_job_context_transport *) arg;
    struct spx_job_context *jc = NULL;
    jc = spx_alloc_alone(sizeof(*jc),err);
    if(NULL == jc){
        return NULL;
    }

    jc->log = jct->log;
    jc->idx = idx;
    jc->trytimes = jct->trytimes;
    jc->timeout = jct->timeout;
    jc->nio_reader = jct->nio_reader;
    jc->nio_writer = jct->nio_writer;
    jc->verifyHeaderHandler = jct->verifyHeaderHandler;
    jc->beforeRecviveBodyHandler = jct->beforeRecviveBodyHandler;
    jc->disposeReaderHandler = jct->disposeReaderHandler;
    jc->disposeWriterHandler = jct->disposeWriterHandler;
    jc->waitting = jct->waitting;
    jc->config = jct->config;
    jc->is_lazy_recv = false;
    jc->is_sendfile = false;
    return jc;

}/*}}}*/

err_t spx_job_context_free(void **arg){/*{{{*/
    struct spx_job_context **jc = (struct spx_job_context **) arg;
    spx_job_context_clear(*jc);
    SpxFree(*jc);
    return 0;
}/*}}}*/

void spx_job_context_clear(struct spx_job_context *jc){/*{{{*/
    if(NULL != jc->reader_body_ctx){
        SpxMsgFree(jc->reader_body_ctx);
    }

    if(NULL != jc->client_ip){
        SpxStringFree(jc->client_ip);
    }

    if(NULL != jc->reader_header){
        SpxFree(jc->reader_header);
    }
    if(NULL != jc->reader_header_ctx){
        SpxMsgFree(jc->reader_header_ctx);
    }

    if(NULL != (jc)->writer_header){
        SpxFree((jc)->writer_header);
    }
    if(NULL != (jc)->writer_header_ctx){
        SpxMsgFree(jc->writer_header_ctx);
    }
    if(NULL != (jc)->writer_body_ctx){
        SpxMsgFree(jc->writer_body_ctx);
    }
    if(NULL != jc->data){
        jc->data = NULL;
    }

    if(jc->is_sendfile){
        if(0 != jc->sendfile_fd){
            SpxClose(jc->sendfile_fd);
        }
        jc->sendfile_begin = 0;
        jc->sendfile_size = 0;
    }
    SpxClose(jc->fd);
    jc->err = 0;
    jc->is_sendfile = false;
    jc->offset = 0;
    jc->curr_trytimes = 0;
    jc->moore = SpxNioMooreNormal;
    jc->request_timespan = 0;
}/*}}}*/


void spx_job_context_reset(struct spx_job_context *jc){/*{{{*/
    if(NULL != jc->reader_header){
        SpxFree(jc->reader_header);
    }
    if(NULL != jc->reader_header_ctx){
        spx_msg_free(&((jc)->reader_header_ctx));
    }
    if(NULL != jc->reader_body_ctx){
        spx_msg_free(&((jc)->reader_body_ctx));
    }

    if(NULL != (jc)->writer_header){
        SpxFree((jc)->writer_header);
    }
    if(NULL != (jc)->writer_header_ctx){
        spx_msg_free(&((jc)->writer_header_ctx));
    }
    if(NULL != (jc)->writer_body_ctx){
        spx_msg_free(&((jc)->writer_body_ctx));
    }

    if(jc->is_sendfile){
        if(0 != jc->sendfile_fd){
            SpxClose(jc->sendfile_fd);
        }
        jc->sendfile_begin = 0;
        jc->sendfile_size = 0;
    }
    jc->err = 0;
    jc->offset = 0;
    jc->is_sendfile = false;
    jc->curr_trytimes = 0;
    jc->moore = SpxNioMooreNormal;
    jc->request_timespan = spx_now();
}/*}}}*/


struct spx_job_pool *spx_job_pool_new(SpxLogDelegate *log,
        void *config,
        size_t size,u32_t timeout,u32_t waitting,u32_t trytimes,
        SpxNioDelegate *nio_reader,
        SpxNioDelegate *nio_writer,
        SpxVerifyHeaderDelegate *verifyHeaderHandler,
        SpxBeforeRecviveBodyDelegate *beforeRecviveBodyHandler,
        SpxDisposeDelegate *disposeReaderHandler,
        SpxDisposeDelegate *disposeWriterHandler,
        err_t *err){
    if(0 == size){
        *err = EINVAL;
    }
    struct spx_job_pool *pool = NULL;
    pool = spx_alloc_alone(sizeof(*pool),err);
    if(NULL == pool){
        return NULL;
    }

    pool->log = log;
    pool->size = size;
    pool->locker = spx_thread_mutex_new(log,err);
    if(!pool->locker){
        SpxFree(pool);
        SpxLog2(log,SpxLogError,*err,\
                "alloc vector locker is fail.");
        return NULL;
    }

    size_t i = 0;
    for(; i < size; i++) {
        struct spx_job_context *jc = (struct spx_job_context *)
            spx_alloc_alone(sizeof(*jc),err);
        if(!jc){
            SpxFree(pool);
            return NULL;
        }

        jc->log = log;
        jc->idx = i;
        jc->waitting = waitting;
        jc->timeout = timeout;
        jc->nio_reader = nio_reader;
        jc->nio_writer = nio_writer;
        jc->trytimes = trytimes;
        jc->verifyHeaderHandler = verifyHeaderHandler;
        jc->beforeRecviveBodyHandler = beforeRecviveBodyHandler;
        jc->disposeReaderHandler = disposeReaderHandler;
        jc->disposeWriterHandler = disposeWriterHandler;
        jc->config = config;
        jc->is_lazy_recv = false;
        jc->is_sendfile = false;

        if(!pool->header){
            pool->header = jc;
            pool->tail = jc;
        }else {
            pool->tail->next = jc;
            jc->prev = pool->tail;
            pool->tail = jc;
        }
    }

    return pool;
}

struct spx_job_context *spx_job_pool_pop(struct spx_job_pool *pool,err_t *err){
    struct spx_job_context *jc = NULL;
    if(!pthread_mutex_lock(pool->locker)) {
        if(pool->tail){
            jc = pool->tail;
            pool->tail = jc->prev;
            if(pool->tail){
                pool->tail->next = NULL;
            }else {
                pool->header = NULL;
            }
            jc->prev = NULL;
            pool->busysize ++;

            if(!pool->busy_header){
                pool->busy_header = jc;
                pool->busy_tail = jc;
            }else {
                pool->busy_tail->next = jc;
                jc->prev = pool->busy_tail;
                pool->busy_tail = jc;
            }
        }
    }

    pthread_mutex_unlock(pool->locker);
    return jc;
}

err_t spx_job_pool_push(struct spx_job_pool *pool,struct spx_job_context *jc){
    spx_job_context_clear(jc);
    if(!pthread_mutex_lock(pool->locker)) {
        if(jc == pool->busy_header){
            if(jc->next){ //header but not tail
                pool->busy_header = jc->next;
                pool->busy_header->prev = NULL;
            } else { // header and tail
                pool->busy_header = NULL;
                pool->busy_tail = NULL;
            }
        }
        else if(jc == pool->busy_tail){
            if(jc->prev){
                pool->busy_tail = jc->prev;
                pool->busy_tail->next = NULL;
            } else {
                pool->busy_header = NULL;
                pool->busy_tail = NULL;
            }
        } else {
            jc->next->prev = jc->prev;
            jc->prev->next = jc->next;
        }
        jc->next = NULL;
        jc->prev = NULL;

        if(!pool->header){
            pool->header = jc;
            pool->tail = jc;
        }else {
            pool->tail->next = jc;
            jc->prev = pool->tail;
            pool->tail = jc;
        }
        pool->busysize --;

//        if(1 == pool->busysize){
//            pool->busy_header = NULL;
//            pool->busy_tail = NULL;
//        } else {
//            if(jc->prev){
//                jc->prev->next = jc->next;
//            }else {
//                pool->busy_header = jc->next;
//            }
//            if(jc->next){
//                jc->next->prev = jc->prev;
//            } else {
//                pool->busy_tail = jc->prev;
//            }
//        }
//
//        pool->busysize --;
    }

    pthread_mutex_unlock(pool->locker);
    return 0;
}

err_t spx_job_pool_free(struct spx_job_pool **pool){
    err_t err = 0;
    SpxFree(*pool);
    return err;
}




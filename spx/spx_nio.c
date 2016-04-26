/*
 * =====================================================================================
 *
 *       Filename:  spx_nio.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2014/06/09 17时43分46秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <ev.h>
#include <sys/sendfile.h>

#include "spx_types.h"
#include "spx_job.h"
#include "spx_io.h"
#include "spx_defs.h"
#include "spx_nio.h"
#include "spx_errno.h"
#include "spx_message.h"
#include "spx_alloc.h"
#include "spx_task.h"





err_t  spx_dio_regedit_async(ev_async *w,
        SpxAsyncDelegate *reader,void *data){/*{{{*/
    ev_async_init(w,reader);
    w->data = data;
    return 0;
}/*}}}*/


void spx_dio_regedit_stat(ev_stat *w,string_t path,double ts,SpxSateDelegate *cb,void *arg){
    ev_stat_init(w,cb,path,ts);
    w->data = arg;

}

void spx_dio_async_launch(struct ev_loop *loop,ev_async *w,
        SpxAsyncDelegate *hander,void *data){
    ev_async_init(w,hander);
    w->data = data;
    ev_async_start(loop,w);
    ev_async_send(loop,w);
}

//the job context will push to pool
void spx_nio_reader_with_timeout(int revents,void *arg){/*{{{*/
    struct spx_job_context *jc = (struct spx_job_context *) arg;
    if(NULL == jc) return;

    if (EV_ERROR & revents) {
        SpxLogFmt1(jc->log,SpxLogError,
                "waitting reading from client:%s is timeout,"
                "and close the client and push job-context to pool.",
                jc->client_ip);
        spx_job_pool_push(g_spx_job_pool,jc);
        return;
    }

    if(EV_TIMEOUT & revents) {
        if(jc->trytimes <= (jc->curr_trytimes++)){
            SpxLogFmt1(jc->log,SpxLogError,
                    "waitting client:%s to max times:%d."
                    "then close the client and push job-context to pool.",
                    jc->client_ip,jc->curr_trytimes);
            spx_job_pool_push(g_spx_job_pool,jc);
            return;
        } else {
            ev_once(jc->tc->loop,jc->fd,EV_READ,jc->waitting,jc->nio_reader,(void *) jc);
            SpxLogFmt1(jc->log,SpxLogWarn,
                    "re-try waitting client:%s again."
                    "current try-times is :%d.",
                    jc->client_ip,jc->curr_trytimes);
            return;
        }
    }

    err_t err = 0;
    if(EV_READ & revents){
        //        size_t len = 0;
        if(SpxNioLifeCycleHeader == jc->lifecycle){
            if(NULL == jc->reader_header_ctx) {
                struct spx_msg *ctx = spx_msg_new(SpxMsgHeaderSize,&err);
                if(NULL == ctx){
                    jc->err = err;
                    SpxLogFmt2(jc->log,SpxLogError,err,\
                            "alloc reader header msg is fail."
                            "client ip:%s.",
                            jc->client_ip);
                    spx_job_pool_push(g_spx_job_pool,jc);
                    return;
                }
                jc->reader_header_ctx = ctx;
            }

            err = spx_read_msg_ack(jc->fd,jc->reader_header_ctx,SpxMsgHeaderSize , &(jc->offset));
            if(err) {
                if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err) {
                    if(SpxMsgHeaderSize != jc->offset) {
                        ev_once(jc->tc->loop,jc->fd,EV_READ,jc->waitting,jc->nio_reader,(void *) jc);
                        return;
                    }
                } else {
                    jc->err = err;
                    SpxLogFmt2(jc->log,SpxLogError,err,\
                            "read header from client:%s is fail."
                            "header length:%d,real recv length:%d."
                            "then push job to pool.",
                            jc->client_ip,SpxMsgHeaderSize,jc->offset);
                    spx_job_pool_push(g_spx_job_pool,jc);
                    return;
                }
            }

            spx_msg_seek(jc->reader_header_ctx,0,SpxMsgSeekSet);
            struct spx_msg_header *header = spx_msg_to_header(jc->reader_header_ctx,&err);
            if(NULL == header){
                jc->err = err;
                SpxLogFmt2(jc->log,SpxLogError,err,
                        "convert msg-ctx to header is fail."
                        "client ip:%s.",
                        jc->client_ip);
                spx_job_pool_push(g_spx_job_pool,jc);
                return;
            }
            jc->reader_header = header;
            if(header->bodylen < header->offset){
                jc->err = EINVAL;
                SpxLogFmt1(jc->log,SpxLogError,
                        "body len is 0 and no need read."
                        "client:%s."
                        "proto:%d,version:%d.",
                        jc->client_ip,
                        jc->reader_header->protocol,
                        jc->reader_header->version);
                spx_job_pool_push(g_spx_job_pool,jc);
                return;
            }

            if(NULL != jc->verifyHeaderHandler) {
                // if validator return false,then the function must push jc to pool
                if(!jc->verifyHeaderHandler(jc)){
                    return;
                }
            }

            if(NULL != jc->beforeRecviveBodyHandler){
                // if body_process_before return false,then the function must push jc to pool
                if(!jc->beforeRecviveBodyHandler(jc)){
                    return;
                }
            }
            jc->lifecycle = SpxNioLifeCycleBody;
            jc->offset = 0;
            //            len = 0;
        }

        if(SpxNioLifeCycleBody == jc->lifecycle) {
            struct spx_msg_header *header = jc->reader_header;
            if(NULL == header){
                jc->err = ENXIO;
                SpxLogFmt1(jc->log,SpxLogError,
                        "reader header is null."
                        "client:%s.",
                        jc->client_ip);
                spx_job_pool_push(g_spx_job_pool,jc);
                return;
            }

            //only have header,the request is also dealed
            if(0 != header->offset || 0 != header->bodylen) {
                if(jc->is_lazy_recv){
                    if(NULL == jc->reader_body_ctx) {
                        struct spx_msg *ctx = spx_msg_new(header->offset,&(jc->err));
                        if(NULL == ctx){
                            SpxLogFmt2(jc->log,SpxLogError,jc->err,
                                    "alloc body buffer is fail."
                                    "client:%s."
                                    "proto:%d,version:%d.",
                                    jc->client_ip,
                                    jc->reader_header->protocol,
                                    jc->reader_header->version);
                            spx_job_pool_push(g_spx_job_pool,jc);
                            return;
                        }
                        jc->reader_body_ctx = ctx;
                    }

                    err = spx_read_msg_ack(jc->fd,jc->reader_body_ctx,jc->reader_header->offset , &(jc->offset));
                    if(err) {
                        if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err) {
                            //                        jc->offset += len;
                            if(header->offset != jc->offset) {
                                ev_once(jc->tc->loop,jc->fd,EV_READ,jc->waitting,jc->nio_reader,(void *) jc);
                                return;
                            }
                        } else {
                            jc->err = err;
                            SpxLogFmt2(jc->log,SpxLogError,err,\
                                    "read header from client:%s is fail."
                                    "body offset length:%d,real recv length:%d."
                                    "then push job to pool.",
                                    jc->client_ip,header->offset,jc->offset);
                            spx_job_pool_push(g_spx_job_pool,jc);
                            return;
                        }
                    }
                } else {
                    if(NULL == jc->reader_body_ctx) {
                        struct spx_msg *ctx = spx_msg_new(header->bodylen,&(jc->err));
                        if(NULL == ctx){
                            SpxLogFmt2(jc->log,SpxLogError,jc->err,
                                    "alloc body buffer is fail.client:%s.",
                                    "proto:%d,version:%d.",
                                    jc->client_ip,
                                    jc->reader_header->protocol,
                                    jc->reader_header->version);
                            spx_job_pool_push(g_spx_job_pool,jc);
                            return;
                        }
                        jc->reader_body_ctx = ctx;
                    }

                    err = spx_read_msg_ack(jc->fd,jc->reader_body_ctx,header->bodylen ,&(jc->offset));
                    if(err) {
                        if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err) {
                            //                        jc->offset += len;
                            if(header->bodylen != jc->offset) {
                                ev_once(jc->tc->loop,jc->fd,EV_READ,jc->waitting,jc->nio_reader,(void *) jc);
                                return;
                            }
                        } else {
                            jc->err = err;
                            SpxLogFmt2(jc->log,SpxLogError,err,\
                                    "read header from client:%s is fail."
                                    "body length:%d,real recv length:%d."
                                    "then push job to pool.",
                                    jc->client_ip,header->bodylen,jc->offset);
                            spx_job_pool_push(g_spx_job_pool,jc);
                            return;
                        }
                    }
                }
            }
            if(NULL != jc->disposeReaderHandler){
                jc->disposeReaderHandler(jc);
            }
        }
    }
}/*}}}*/

void spx_nio_writer_with_timeout(int revents,void *arg){/*{{{*/
    struct spx_job_context *jc = (struct spx_job_context *) arg;
    if(NULL == jc) return;

    if (EV_ERROR & revents) {
        SpxLogFmt1(jc->log,SpxLogError,
                "waitting writing to client:%s is fail,"
                "and close the client and push job-context to pool.",
                jc->client_ip);
        spx_job_pool_push(g_spx_job_pool,jc);
        return;
    }

    if(EV_TIMEOUT & revents) {
        if(jc->trytimes <= (jc->curr_trytimes++)){
            SpxLogFmt1(jc->log,SpxLogError,
                    "waitting writeting to client:%s to max times:%d."
                    "then close the client and push job-context to pool.",
                    jc->client_ip,jc->curr_trytimes);
            spx_job_pool_push(g_spx_job_pool,jc);
            return;
        } else {
            ev_once(jc->tc->loop,jc->fd,EV_WRITE,jc->waitting,jc->nio_writer,(void *) jc);
            SpxLogFmt1(jc->log,SpxLogWarn,
                    "re-try waitting writing to client:%s again."
                    "current try-times is :%d.",
                    jc->client_ip,jc->curr_trytimes);
            return;
        }
    }

    err_t err = 0;
    if(EV_WRITE & revents) {
        if(SpxNioLifeCycleHeader == jc->lifecycle){
            if(NULL == jc->writer_header_ctx) {
                struct spx_msg *ctx = spx_header_to_msg(jc->writer_header,SpxMsgHeaderSize,&err);
                if(NULL == ctx){
                    jc->err = err;
                    SpxLog2(jc->log,SpxLogError,err,\
                            "convert writer header to msg ctx is fail."\
                            "and forced push jc to pool.");
                    spx_job_pool_push(g_spx_job_pool,jc);
                    return;
                }
                jc->writer_header_ctx = ctx;
            }

            err = spx_write_msg_ack(jc->fd,jc->writer_header_ctx,&(jc->offset));
            if(err) {
                if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err) {
                    if(SpxMsgHeaderSize != jc->offset) {
                        ev_once(jc->tc->loop,jc->fd,EV_WRITE,jc->waitting,jc->nio_writer,(void *) jc);
                        return;
                    }
                } else {
                    jc->err = err;
                    SpxLogFmt2(jc->log,SpxLogError,jc->err,
                            "write  header to client :%s is fail.len:%d."
                            "proto:%d,version:%d."
                            "and forced push jc to pool.",
                            jc->client_ip,jc->offset,
                            jc->writer_header->protocol,
                            jc->writer_header->version);
                    spx_job_pool_push(g_spx_job_pool,jc);
                    return;
                }
            }
            jc->offset = 0;
            jc->lifecycle = SpxNioLifeCycleBody;
        }

        if(SpxNioLifeCycleBody == jc->lifecycle) {
            if(NULL != jc->writer_body_ctx  && 0 != jc->writer_header->bodylen) {//no metedata
                size_t len = 0 == jc->writer_header->offset ? jc->writer_header->bodylen : jc->writer_header->offset;
                err = spx_write_msg_ack(jc->fd,jc->writer_body_ctx,&(jc->offset));
                if(err) {
                    if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err) {
                        if(len != jc->offset) {
                            ev_once(jc->tc->loop,jc->fd,EV_WRITE,jc->waitting,jc->nio_writer,(void *) jc);
                            return;
                        }
                    } else {
                        jc->err = err;
                        SpxLogFmt2(jc->log,SpxLogError,jc->err,\
                                "write  body or prebody to client :%s is fail.len:%d."
                                "proto:%d,version:%d."
                                "and forced push jc to pool.",
                                jc->client_ip,len,
                                jc->writer_header->protocol,
                                jc->writer_header->version);
                        spx_job_pool_push(g_spx_job_pool,jc);
                        return;
                    }
                }
            }
            jc->lifecycle = SpxNioLifeCycleFile;
            jc->offset = 0;
        }

        if(SpxNioLifeCycleFile == jc->lifecycle){
            if(jc->is_sendfile){
                int sendbytes = 0;
                u64_t unitsize = 10 * 1024;
                int want = jc->sendfile_size - jc->offset >= unitsize ? unitsize : jc->sendfile_size - jc->offset;
                sendbytes = sendfile(jc->fd,jc->sendfile_fd,(off_t *) &(jc->offset),want);
                if(0 <= sendbytes){
                    if(jc->offset < jc->sendfile_size){
                        ev_once(jc->tc->loop,jc->fd,EV_WRITE,jc->waitting,jc->nio_writer,(void *) jc);
                        return;
                    }
                } else {
                    if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err) {
                        ev_once(jc->tc->loop,jc->fd,EV_WRITE,jc->waitting,jc->nio_writer,(void *) jc);
                        return;
                    } else {
                        jc->err = err;
                        SpxLogFmt2(jc->log,SpxLogError,jc->err,
                                "sendfile to client :%s is fail.len:%d."
                                "proto:%d,version:%d."
                                "and forced push jc to pool.",
                                jc->client_ip,jc->offset,
                                jc->writer_header->protocol,
                                jc->writer_header->version);
                        spx_job_context_clear(jc);
                        return;
                    }
                }
            }
            jc->lifecycle = SpxNioLifeCycleNormal;
        }

        if(NULL != jc->disposeWriterHandler){
            jc->disposeWriterHandler(jc);
        }
        return;
    }
}/*}}}*/

//the job context is not push to pool and will clear it only
void spx_nio_reader_with_timeout_fast(int revents,void *arg){/*{{{*/
    struct spx_job_context *jc = (struct spx_job_context *) arg;
    if(NULL == jc) return;

    if (EV_ERROR & revents) {
        SpxLogFmt1(jc->log,SpxLogError,
                "waitting reading from client:%s is timeout,"
                "and close the client and push job-context to pool.",
                jc->client_ip);
        spx_job_context_clear(jc);
        return;
    }

    if(EV_TIMEOUT & revents) {
        if(jc->trytimes <= (jc->curr_trytimes++)){
            SpxLogFmt1(jc->log,SpxLogError,
                    "waitting client:%s to max times:%d."
                    "then close the client and push job-context to pool.",
                    jc->client_ip,jc->curr_trytimes);
            spx_job_context_clear(jc);
            return;
        } else {
            ev_once(jc->tc->loop,jc->fd,EV_READ,jc->waitting,jc->nio_reader,(void *) jc);
            SpxLogFmt1(jc->log,SpxLogWarn,
                    "re-try waitting client:%s again."
                    "current try-times is :%d.",
                    jc->client_ip,jc->curr_trytimes);
            return;
        }
    }

    err_t err = 0;
    if(EV_READ & revents){
        if(SpxNioLifeCycleHeader == jc->lifecycle){
            if(NULL == jc->reader_header_ctx) {
                struct spx_msg *ctx = spx_msg_new(SpxMsgHeaderSize,&err);
                if(NULL == ctx){
                    jc->err = err;
                    SpxLogFmt2(jc->log,SpxLogError,err,\
                            "alloc reader header msg is fail."
                            "client ip:%s.",
                            jc->client_ip);
                    spx_job_context_clear(jc);
                    return;
                }
                jc->reader_header_ctx = ctx;
            }

            err = spx_read_msg_ack(jc->fd,jc->reader_header_ctx,SpxMsgHeaderSize , &(jc->offset));
            if(err) {
                if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err) {
                    if(SpxMsgHeaderSize != jc->offset) {
                        ev_once(jc->tc->loop,jc->fd,EV_READ,jc->waitting,jc->nio_reader,(void *) jc);
                        return;
                    }
                } else {
                    jc->err = err;
                    SpxLogFmt2(jc->log,SpxLogError,err,\
                            "read header from client:%s is fail."
                            "header length:%d,real recv length:%d."
                            "then push job to pool.",
                            jc->client_ip,SpxMsgHeaderSize,jc->offset);
                    spx_job_context_clear(jc);
                    return;
                }
            }

            spx_msg_seek(jc->reader_header_ctx,0,SpxMsgSeekSet);
            struct spx_msg_header *header = spx_msg_to_header(jc->reader_header_ctx,&err);
            if(NULL == header){
                jc->err = err;
                SpxLogFmt2(jc->log,SpxLogError,err,
                        "convert msg-ctx to header is fail."
                        "client ip:%s.",
                        jc->client_ip);
                spx_job_context_clear(jc);
                return;
            }
            jc->reader_header = header;
            if(header->bodylen < header->offset){
                jc->err = EINVAL;
                SpxLogFmt1(jc->log,SpxLogError,
                        "body len is 0 and no need read."
                        "client:%s."
                        "proto:%d,version:%d.",
                        jc->client_ip,
                        jc->reader_header->protocol,
                        jc->reader_header->version);
                spx_job_context_clear(jc);
                return;
            }

            if(NULL != jc->verifyHeaderHandler) {
                // if validator return false,then the function must push jc to pool
                if(!jc->verifyHeaderHandler(jc)){
                    return;
                }
            }

            if(NULL != jc->beforeRecviveBodyHandler){
                // if body_process_before return false,then the function must push jc to pool
                if(!jc->beforeRecviveBodyHandler(jc)){
                    return;
                }
            }
            jc->lifecycle = SpxNioLifeCycleBody;
            jc->offset = 0;
        }

        if(SpxNioLifeCycleBody == jc->lifecycle) {
            struct spx_msg_header *header = jc->reader_header;
            if(NULL == header){
                jc->err = ENXIO;
                SpxLogFmt1(jc->log,SpxLogError,
                        "reader header is null."
                        "client:%s.",
                        jc->client_ip);
                spx_job_context_clear(jc);
                return;
            }

            //only have header,the request is also dealed
            if(0 != header->offset || 0 != header->bodylen) {
                if(jc->is_lazy_recv){
                    if(NULL == jc->reader_body_ctx) {
                        struct spx_msg *ctx = spx_msg_new(header->offset,&(jc->err));
                        if(NULL == ctx){
                            SpxLogFmt2(jc->log,SpxLogError,jc->err,
                                    "alloc body buffer is fail."
                                    "client:%s."
                                    "proto:%d,version:%d.",
                                    jc->client_ip,
                                    jc->reader_header->protocol,
                                    jc->reader_header->version);
                            spx_job_context_clear(jc);
                            return;
                        }
                        jc->reader_body_ctx = ctx;
                    }

                    err = spx_read_msg_ack(jc->fd,jc->reader_body_ctx,jc->reader_header->offset , &(jc->offset));
                    if(err) {
                        if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err) {
                            //                        jc->offset += len;
                            if(header->offset != jc->offset) {
                                ev_once(jc->tc->loop,jc->fd,EV_READ,jc->waitting,jc->nio_reader,(void *) jc);
                                return;
                            }
                        } else {
                            jc->err = err;
                            SpxLogFmt2(jc->log,SpxLogError,err,\
                                    "read header from client:%s is fail."
                                    "body offset length:%d,real recv length:%d."
                                    "then push job to pool.",
                                    jc->client_ip,header->offset,jc->offset);
                            spx_job_context_clear(jc);
                            return;
                        }
                    }
                } else {
                    if(NULL == jc->reader_body_ctx) {
                        struct spx_msg *ctx = spx_msg_new(header->bodylen,&(jc->err));
                        if(NULL == ctx){
                            SpxLogFmt2(jc->log,SpxLogError,jc->err,
                                    "alloc body buffer is fail.client:%s.",
                                    "proto:%d,version:%d.",
                                    jc->client_ip,
                                    jc->reader_header->protocol,
                                    jc->reader_header->version);
                            spx_job_context_clear(jc);
                            return;
                        }
                        jc->reader_body_ctx = ctx;
                    }

                    err = spx_read_msg_ack(jc->fd,jc->reader_body_ctx,header->bodylen ,&(jc->offset));
                    if(err) {
                        if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err) {
                            //                        jc->offset += len;
                            if(header->bodylen != jc->offset) {
                                ev_once(jc->tc->loop,jc->fd,EV_READ,jc->waitting,jc->nio_reader,(void *) jc);
                                return;
                            }
                        } else {
                            jc->err = err;
                            SpxLogFmt2(jc->log,SpxLogError,err,\
                                    "read header from client:%s is fail."
                                    "body length:%d,real recv length:%d."
                                    "then push job to pool.",
                                    jc->client_ip,header->bodylen,jc->offset);
                            spx_job_context_clear(jc);
                            return;
                        }
                    }
                }
            }
            if(NULL != jc->disposeReaderHandler){
                jc->disposeReaderHandler(jc);
            }
        }
    }
}/*}}}*/

void spx_nio_writer_with_timeout_fast(int revents,void *arg){/*{{{*/
    struct spx_job_context *jc = (struct spx_job_context *) arg;
    if(NULL == jc) return;

    if (EV_ERROR & revents) {
        SpxLogFmt1(jc->log,SpxLogError,
                "waitting writing to client:%s is fail,"
                "and close the client and push job-context to pool.",
                jc->client_ip);
        spx_job_context_clear(jc);
        return;
    }

    if(EV_TIMEOUT & revents) {
        if(jc->trytimes <= (jc->curr_trytimes++)){
            SpxLogFmt1(jc->log,SpxLogError,
                    "waitting writeting to client:%s to max times:%d."
                    "then close the client and push job-context to pool.",
                    jc->client_ip,jc->curr_trytimes);
            spx_job_context_clear(jc);
            return;
        } else {
            ev_once(jc->tc->loop,jc->fd,EV_WRITE,jc->waitting,jc->nio_writer,(void *) jc);
            SpxLogFmt1(jc->log,SpxLogWarn,
                    "re-try waitting writing to client:%s again."
                    "current try-times is :%d.",
                    jc->client_ip,jc->curr_trytimes);
            return;
        }
    }

    err_t err = 0;
    if(EV_WRITE & revents) {
        if(SpxNioLifeCycleHeader == jc->lifecycle){
            if(NULL == jc->writer_header_ctx) {
                struct spx_msg *ctx = spx_header_to_msg(jc->writer_header,SpxMsgHeaderSize,&err);
                if(NULL == ctx){
                    jc->err = err;
                    SpxLog2(jc->log,SpxLogError,err,\
                            "convert writer header to msg ctx is fail."\
                            "and forced push jc to pool.");
                    spx_job_context_clear(jc);
                    return;
                }
                jc->writer_header_ctx = ctx;
            }

            err = spx_write_msg_ack(jc->fd,jc->writer_header_ctx,&(jc->offset));
            if(err) {
                if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err) {
                    if(SpxMsgHeaderSize != jc->offset) {
                        ev_once(jc->tc->loop,jc->fd,EV_WRITE,jc->waitting,jc->nio_writer,(void *) jc);
                        return;
                    }
                } else {
                    jc->err = err;
                    SpxLogFmt2(jc->log,SpxLogError,jc->err,
                            "write  header to client :%s is fail.len:%d."
                            "proto:%d,version:%d."
                            "and forced push jc to pool.",
                            jc->client_ip,jc->offset,
                            jc->writer_header->protocol,
                            jc->writer_header->version);
                    spx_job_context_clear(jc);
                    return;
                }
            }
            jc->offset = 0;
            jc->lifecycle = SpxNioLifeCycleBody;
        }

        if(SpxNioLifeCycleBody == jc->lifecycle) {
            if(NULL != jc->writer_body_ctx ) {//no metedata
                size_t len = 0 == jc->writer_header->offset ? jc->writer_header->bodylen : jc->writer_header->offset;
                err = spx_write_msg_ack(jc->fd,jc->writer_body_ctx,&(jc->offset));
                if(err) {
                    if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err) {
                        if(len != jc->offset) {
                            ev_once(jc->tc->loop,jc->fd,EV_WRITE,jc->waitting,jc->nio_writer,(void *) jc);
                            return;
                        }
                    } else {
                        jc->err = err;
                        SpxLogFmt2(jc->log,SpxLogError,jc->err,\
                                "write  body or prebody to client :%s is fail.len:%d."
                                "proto:%d,version:%d."
                                "and forced push jc to pool.",
                                jc->client_ip,len,
                                jc->writer_header->protocol,
                                jc->writer_header->version);
                        spx_job_context_clear(jc);
                        return;
                    }
                }
            }
            jc->lifecycle = SpxNioLifeCycleFile;
            jc->offset = 0;
        }



        if(SpxNioLifeCycleFile == jc->lifecycle){
            if(jc->is_sendfile){
                int sendbytes = 0;
                u64_t unitsize = 10 * 1024;
                int want = jc->sendfile_size - jc->offset >= unitsize ? unitsize : jc->sendfile_size - jc->offset;
                sendbytes = sendfile(jc->fd,jc->sendfile_fd,(off_t *) &(jc->offset),want);
                if(0 <= sendbytes){
                    if(jc->offset < jc->sendfile_size){
                        ev_once(jc->tc->loop,jc->fd,EV_WRITE,jc->waitting,jc->nio_writer,(void *) jc);
                        return;
                    }
                } else {
                    if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err) {
                        ev_once(jc->tc->loop,jc->fd,EV_WRITE,jc->waitting,jc->nio_writer,(void *) jc);
                        return;
                    } else {
                        jc->err = err;
                        SpxLogFmt2(jc->log,SpxLogError,jc->err,
                                "sendfile to client :%s is fail.len:%d."
                                "proto:%d,version:%d."
                                "and forced push jc to pool.",
                                jc->client_ip,jc->offset,
                                jc->writer_header->protocol,
                                jc->writer_header->version);
                        spx_job_context_clear(jc);
                        return;
                    }
                }
            }
            jc->lifecycle = SpxNioLifeCycleNormal;
        }

        if(NULL != jc->disposeWriterHandler){
            jc->disposeWriterHandler(jc);
        }
        return;
    }
}/*}}}*/

//the job context is not push to pool or clear and allways call the callback
void spx_nio_reader_with_timeout_ack(int revents,void *arg){/*{{{*/
    struct spx_job_context *jc = (struct spx_job_context *) arg;
    if(NULL == jc) return;

    if (EV_ERROR & revents) {
        SpxLogFmt1(jc->log,SpxLogError,
                "waitting reading from client:%s is timeout,"
                "and close the client and push job-context to pool.",
                jc->client_ip);
        spx_job_context_clear(jc);
        return;
    }

    if(EV_TIMEOUT & revents) {
        if(jc->trytimes <= (jc->curr_trytimes++)){
            SpxLogFmt1(jc->log,SpxLogError,
                    "waitting client:%s to max times:%d."
                    "then close the client and push job-context to pool.",
                    jc->client_ip,jc->curr_trytimes);
            spx_job_context_clear(jc);
            return;
        } else {
            ev_once(jc->tc->loop,jc->fd,EV_READ,jc->waitting,jc->nio_reader,(void *) jc);
            SpxLogFmt1(jc->log,SpxLogWarn,
                    "re-try waitting client:%s again."
                    "current try-times is :%d.",
                    jc->client_ip,jc->curr_trytimes);
            return;
        }
    }

    err_t err = 0;
    if(EV_READ & revents){
        if(SpxNioLifeCycleHeader == jc->lifecycle){
            if(NULL == jc->reader_header_ctx) {
                struct spx_msg *ctx = spx_msg_new(SpxMsgHeaderSize,&err);
                if(NULL == ctx){
                    jc->err = err;
                    SpxLogFmt2(jc->log,SpxLogError,err,\
                            "alloc reader header msg is fail."
                            "client ip:%s.",
                            jc->client_ip);
                    spx_job_context_clear(jc);
                    return;
                }
                jc->reader_header_ctx = ctx;
            }

            err = spx_read_msg_ack(jc->fd,jc->reader_header_ctx,SpxMsgHeaderSize , &(jc->offset));
            if(err) {
                if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err) {
                    if(SpxMsgHeaderSize != jc->offset) {
                        ev_once(jc->tc->loop,jc->fd,EV_READ,jc->waitting,jc->nio_reader,(void *) jc);
                        return;
                    }
                } else {
                    jc->err = err;
                    SpxLogFmt2(jc->log,SpxLogError,err,\
                            "read header from client:%s is fail."
                            "header length:%d,real recv length:%d."
                            "then push job to pool.",
                            jc->client_ip,SpxMsgHeaderSize,jc->offset);
                    spx_job_context_clear(jc);
                    return;
                }
            }

            spx_msg_seek(jc->reader_header_ctx,0,SpxMsgSeekSet);
            struct spx_msg_header *header = spx_msg_to_header(jc->reader_header_ctx,&err);
            if(NULL == header){
                jc->err = err;
                SpxLogFmt2(jc->log,SpxLogError,err,
                        "convert msg-ctx to header is fail."
                        "client ip:%s.",
                        jc->client_ip);
                spx_job_context_clear(jc);
                return;
            }
            jc->reader_header = header;
            if(header->bodylen < header->offset){
                jc->err = EINVAL;
                SpxLogFmt1(jc->log,SpxLogError,
                        "body len is 0 and no need read."
                        "client:%s."
                        "proto:%d,version:%d.",
                        jc->client_ip,
                        jc->reader_header->protocol,
                        jc->reader_header->version);
                spx_job_context_clear(jc);
                return;
            }

            if(NULL != jc->verifyHeaderHandler) {
                // if validator return false,then the function must push jc to pool
                if(!jc->verifyHeaderHandler(jc)){
                    return;
                }
            }

            if(NULL != jc->beforeRecviveBodyHandler){
                // if body_process_before return false,then the function must push jc to pool
                if(!jc->beforeRecviveBodyHandler(jc)){
                    return;
                }
            }
            jc->lifecycle = SpxNioLifeCycleBody;
            jc->offset = 0;
        }

        if(SpxNioLifeCycleBody == jc->lifecycle) {
            struct spx_msg_header *header = jc->reader_header;
            if(NULL == header){
                jc->err = ENXIO;
                SpxLogFmt1(jc->log,SpxLogError,
                        "reader header is null."
                        "client:%s.",
                        jc->client_ip);
                spx_job_context_clear(jc);
                return;
            }

            //only have header,the request is also dealed
            if(0 != header->offset || 0 != header->bodylen) {
                if(jc->is_lazy_recv){
                    if(NULL == jc->reader_body_ctx) {
                        struct spx_msg *ctx = spx_msg_new(header->offset,&(jc->err));
                        if(NULL == ctx){
                            SpxLogFmt2(jc->log,SpxLogError,jc->err,
                                    "alloc body buffer is fail."
                                    "client:%s."
                                    "proto:%d,version:%d.",
                                    jc->client_ip,
                                    jc->reader_header->protocol,
                                    jc->reader_header->version);
                            spx_job_context_clear(jc);
                            return;
                        }
                        jc->reader_body_ctx = ctx;
                    }

                    err = spx_read_msg_ack(jc->fd,jc->reader_body_ctx,jc->reader_header->offset , &(jc->offset));
                    if(err) {
                        if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err) {
                            //                        jc->offset += len;
                            if(header->offset != jc->offset) {
                                ev_once(jc->tc->loop,jc->fd,EV_READ,jc->waitting,jc->nio_reader,(void *) jc);
                                return;
                            }
                        } else {
                            jc->err = err;
                            SpxLogFmt2(jc->log,SpxLogError,err,\
                                    "read header from client:%s is fail."
                                    "body offset length:%d,real recv length:%d."
                                    "then push job to pool.",
                                    jc->client_ip,header->offset,jc->offset);
                            spx_job_context_clear(jc);
                            return;
                        }
                    }
                } else {
                    if(NULL == jc->reader_body_ctx) {
                        struct spx_msg *ctx = spx_msg_new(header->bodylen,&(jc->err));
                        if(NULL == ctx){
                            SpxLogFmt2(jc->log,SpxLogError,jc->err,
                                    "alloc body buffer is fail.client:%s.",
                                    "proto:%d,version:%d.",
                                    jc->client_ip,
                                    jc->reader_header->protocol,
                                    jc->reader_header->version);
                            spx_job_context_clear(jc);
                            return;
                        }
                        jc->reader_body_ctx = ctx;
                    }

                    err = spx_read_msg_ack(jc->fd,jc->reader_body_ctx,header->bodylen ,&(jc->offset));
                    if(err) {
                        if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err) {
                            //                        jc->offset += len;
                            if(header->bodylen != jc->offset) {
                                ev_once(jc->tc->loop,jc->fd,EV_READ,jc->waitting,jc->nio_reader,(void *) jc);
                                return;
                            }
                        } else {
                            jc->err = err;
                            SpxLogFmt2(jc->log,SpxLogError,err,\
                                    "read header from client:%s is fail."
                                    "body length:%d,real recv length:%d."
                                    "then push job to pool.",
                                    jc->client_ip,header->bodylen,jc->offset);
                            spx_job_context_clear(jc);
                            return;
                        }
                    }
                }
            }
            if(NULL != jc->disposeReaderHandler){
                jc->disposeReaderHandler(jc);
            }
        }
    }
}/*}}}*/

void spx_nio_writer_with_timeout_ack(int revents,void *arg){/*{{{*/
    struct spx_job_context *jc = (struct spx_job_context *) arg;
    if(NULL == jc) return;

    if (EV_ERROR & revents) {
        SpxLogFmt1(jc->log,SpxLogError,
                "waitting writing to client:%s is fail,"
                "and close the client and push job-context to pool.",
                jc->client_ip);
        spx_job_context_clear(jc);
        return;
    }

    if(EV_TIMEOUT & revents) {
        if(jc->trytimes <= (jc->curr_trytimes++)){
            SpxLogFmt1(jc->log,SpxLogError,
                    "waitting writeting to client:%s to max times:%d."
                    "then close the client and push job-context to pool.",
                    jc->client_ip,jc->curr_trytimes);
            spx_job_context_clear(jc);
            return;
        } else {
            ev_once(jc->tc->loop,jc->fd,EV_WRITE,jc->waitting,jc->nio_writer,(void *) jc);
            SpxLogFmt1(jc->log,SpxLogWarn,
                    "re-try waitting writing to client:%s again."
                    "current try-times is :%d.",
                    jc->client_ip,jc->curr_trytimes);
            return;
        }
    }

    err_t err = 0;
    if(EV_WRITE & revents) {
        if(SpxNioLifeCycleHeader == jc->lifecycle){
            if(NULL == jc->writer_header_ctx) {
                struct spx_msg *ctx = spx_header_to_msg(jc->writer_header,SpxMsgHeaderSize,&err);
                if(NULL == ctx){
                    jc->err = err;
                    SpxLog2(jc->log,SpxLogError,err,\
                            "convert writer header to msg ctx is fail."\
                            "and forced push jc to pool.");
                    spx_job_context_clear(jc);
                    return;
                }
                jc->writer_header_ctx = ctx;
            }

            err = spx_write_msg_ack(jc->fd,jc->writer_header_ctx,&(jc->offset));
            if(err) {
                if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err) {
                    if(SpxMsgHeaderSize != jc->offset) {
                        ev_once(jc->tc->loop,jc->fd,EV_WRITE,jc->waitting,jc->nio_writer,(void *) jc);
                        return;
                    }
                } else {
                    jc->err = err;
                    SpxLogFmt2(jc->log,SpxLogError,jc->err,
                            "write  header to client :%s is fail.len:%d."
                            "proto:%d,version:%d."
                            "and forced push jc to pool.",
                            jc->client_ip,jc->offset,
                            jc->writer_header->protocol,
                            jc->writer_header->version);
                    spx_job_context_clear(jc);
                    return;
                }
            }
            jc->offset = 0;
            jc->lifecycle = SpxNioLifeCycleBody;
        }

        if(SpxNioLifeCycleBody == jc->lifecycle) {
            if(NULL != jc->writer_body_ctx ) {//no metedata
                size_t len = 0 == jc->writer_header->offset ? jc->writer_header->bodylen : jc->writer_header->offset;
                err = spx_write_msg_ack(jc->fd,jc->writer_body_ctx,&(jc->offset));
                if(err) {
                    if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err) {
                        if(len != jc->offset) {
                            ev_once(jc->tc->loop,jc->fd,EV_WRITE,jc->waitting,jc->nio_writer,(void *) jc);
                            return;
                        }
                    } else {
                        jc->err = err;
                        SpxLogFmt2(jc->log,SpxLogError,jc->err,\
                                "write  body or prebody to client :%s is fail.len:%d."
                                "proto:%d,version:%d."
                                "and forced push jc to pool.",
                                jc->client_ip,len,
                                jc->writer_header->protocol,
                                jc->writer_header->version);
                        spx_job_context_clear(jc);
                        return;
                    }
                }
            }
            jc->lifecycle = SpxNioLifeCycleFile;
            jc->offset = 0;
        }

        if(SpxNioLifeCycleFile == jc->lifecycle){
            if(jc->is_sendfile){
                size_t len = 0;
                err = spx_sendfile_ack(jc->fd,jc->sendfile_fd,\
                        jc->sendfile_begin + jc->offset,jc->sendfile_size - jc->offset,&(len));
                jc->offset += len;
//                err = spx_sendfile_ack(jc->fd,jc->sendfile_fd,
//                        jc->sendfile_begin + jc->offset,jc->sendfile_size - jc->offset,&(jc->offset));
                if(err) {
                    if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err) {
                        if(jc->sendfile_size != jc->offset) {
                            ev_once(jc->tc->loop,jc->fd,EV_WRITE,jc->waitting,jc->nio_writer,(void *) jc);
                            return;
                        }
                    } else {
                        jc->err = err;
                        SpxLogFmt2(jc->log,SpxLogError,jc->err,
                                "sendfile to client :%s is fail.len:%d."
                                "proto:%d,version:%d."
                                "and forced push jc to pool.",
                                jc->client_ip,jc->offset,
                                jc->writer_header->protocol,
                                jc->writer_header->version);
                        spx_job_context_clear(jc);
                        return;
                    }
                }
            }
            jc->lifecycle = SpxNioLifeCycleNormal;
        }

        if(NULL != jc->disposeWriterHandler){
            jc->disposeWriterHandler(jc);
        }
        return;
    }
}/*}}}*/



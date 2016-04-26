/*
 * =====================================================================================
 *
 *       Filename:  spx_io.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2014/06/12 11时36分55秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include  <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#ifdef SpxMac
#include <sys/uio.h>
#endif

#ifdef SpxLinux
#include <sys/sendfile.h>
#endif


#include "spx_types.h"
#include "spx_defs.h"
#include "spx_errno.h"
#include "spx_message.h"
#include "spx_alloc.h"

err_t spx_read(int fd,byte_t *buf,const size_t size,size_t *len){/*{{{*/
    SpxErrReset;
    *len = 0;
    err_t err = 0;
    i64_t rc = 0;
    while(*len < size){
        rc = read(fd,((char *) buf) + *len,size - *len);
        if(0 > rc){
            if(EAGAIN == errno || EWOULDBLOCK == errno || EINTR == errno){
                SpxErrReset;
                continue;
            }
            err = errno;
            break;
        }else if(0 == rc){
            break;
        }else {
            *len += rc;
        }
    }
    if(0 != err) return err;
    if(size != *len) return EIO;
    return 0;
}/*}}}*/

err_t spx_write(int fd,byte_t *buf,const size_t size,size_t *len){/*{{{*/
    SpxErrReset;
    *len = 0;
    err_t err = 0;
    i64_t rc = 0;
    while(*len < size){
        rc = write(fd,((char *) buf) + *len,size - *len);
        if(0 > rc){
            if(EAGAIN == errno || EWOULDBLOCK == errno || EINTR == errno){
                SpxErrReset;
                continue;
            }
            err = errno;
            break;
        }else if(0 == rc){
            break;
        }else {
            *len += rc;
        }
    }
    if(0 != err) return err;
    if(size != *len) return EIO;
    return 0;
}/*}}}*/

err_t spx_read_nb(int fd,byte_t *buf,const size_t size,size_t *len){/*{{{*/
    SpxErrReset;
    *len = 0;
    err_t err = 0;
    i64_t rc = 0;
    while(*len < size){
        rc = read(fd,((char *) buf) + *len,size - *len);
        if(0 > rc){
            if(EAGAIN == errno || EWOULDBLOCK == errno || EINTR == errno){
                SpxErrReset;
                continue;
            }
            err = errno;
            break;
        }else if(0 == rc){
            break;
        }else {
            *len += rc;
        }
    }
    if(0 != err) return err;
    if(size != *len) return EIO;
    return 0;
}/*}}}*/


err_t spx_read_ack(int fd,byte_t *buf,const size_t size,size_t *len){/*{{{*/
    SpxErrReset;
    err_t err = 0;
    i64_t rc = 0;
    while(*len < size){
        rc = read(fd,((char *) buf) + *len,size - *len);
        if(0 > rc){
            err = errno;
            break;
        }else if(0 == rc){
            break;
        }else {
            *len += rc;
        }
    }
    if(0 != err) return err;
    if(size != *len) return EIO;
    return 0;
}/*}}}*/

    err_t spx_read_msg_ack(int fd,struct spx_msg *msg,const size_t size,size_t *len){
        return spx_read_ack(fd,(byte_t *) msg->buf,size,len);
    }

err_t spx_perread_ack(int fd,byte_t *buf,const size_t size,size_t *len){/*{{{*/
    SpxErrReset;
    *len = 0;
    err_t err = 0;
    i64_t rc = 0;
    while(*len < size){
        rc = recv(fd,buf + *len,size - *len,MSG_PEEK);
        if(0 > rc){
            err = errno;
            break;
        }else if(0 == rc){
            break;
        }else {
            *len += rc;
        }
    }
    if(0 != err) return err;
    if(size != *len) return EIO;
    return 0;
}/*}}}*/


err_t spx_write_ack(int fd,byte_t *buf,const size_t size,size_t *len){/*{{{*/
    SpxErrReset;
    *len = 0;
    err_t err = 0;
    i64_t rc = 0;
    while(*len < size){
        rc = write(fd,((char *) buf) + *len,size - *len);
        if(0 > rc){
            err = errno;
            break;
        }else if(0 == rc){
            break;
        }else {
            *len += rc;
        }
    }
    if(0 != err) return err;
    if(size != *len) return EIO;
    return 0;
}/*}}}*/

err_t spx_write_msg_ack(int fd,struct spx_msg *ctx,size_t *len){
    return spx_write_ack(fd,(byte_t *) ctx->buf,ctx->busylen,len);
}

err_t spx_sendfile_ack(int sock,int fd,off_t offset,size_t size,size_t *len){/*{{{*/
    off_t offset_new = offset;
    off_t want = (off_t) size;
    err_t err = 0;
    *len = 0;
#ifdef SpxMac
    while(true){
        //want is a in/out paras means want to send length when in and sended
        //length when return
        if(0 != sendfile(fd,sock,offset_new,&want,NULL, 0)){
            if(EAGAIN == errno || EWOULDBLOCK == errno || EINTR == errno){
                *len += want;
                if(*len == size){
                    err = 0;
                    break;
                }
                offset_new += want;
                want = size - *len;
                err = errno;
                return err;
            }else {
                err = errno;
                return err;
            }
        }
        *len += want;
        if(*len == size){
            err = 0;
            break;
        }
        offset_new += want;
        want = size - *len;
    }

#endif

#ifdef SpxLinux
    while(true){
        i64_t sendbytes = 0;
        sendbytes = sendfile(sock,fd,&offset_new,want);
        if(-1 == sendbytes){
            if(0 < errno) {
                err = errno;
                break;
            }
        }
        *len += sendbytes;
        if(*len == size){
            break;
        }
        offset_new = offset + *len;
        offset = offset_new;
        want = size - *len;
    }
#endif
    if(0 != err) return err;
    if(size != *len) return EIO;
    return 0;
}/*}}}*/



err_t spx_write_nb(int fd,byte_t *buf,const size_t size,size_t *len){/*{{{*/
    SpxErrReset;
    *len = 0;
    err_t err = 0;
    i64_t rc = 0;
    while(*len < size){
        rc = write(fd,((char *) buf) + *len,size - *len);
        if(0 > rc){
            if(EAGAIN == errno || EWOULDBLOCK == errno || EINTR == errno){
                SpxErrReset;
                continue;
            }
            err = errno;
            break;
        }else if(0 == rc){
            break;
        }else {
            *len += rc;
        }
    }
    if(0 != err) return err;
    if(size != *len) return EIO;
    return 0;
}/*}}}*/

err_t spx_read_to_msg(int fd,struct spx_msg *ctx,const size_t size,size_t *len){/*{{{*/
    err_t err = spx_read(fd,(byte_t *) ctx->last ,size,len);
    spx_msg_seek(ctx,spx_msg_len(ctx),SpxMsgSeekSet);
    ctx->busylen = *len;
    return err;
}/*}}}*/

err_t spx_read_to_msg_nb(int fd,struct spx_msg *ctx,const size_t size,size_t *len){/*{{{*/
    err_t err = spx_read_nb(fd,(byte_t *) ctx->last,size,len);
    spx_msg_seek(ctx,spx_msg_len(ctx),SpxMsgSeekSet);
    ctx->busylen = *len;
    return err;
}/*}}}*/

err_t spx_write_from_msg(int fd,struct spx_msg *ctx,const size_t size,size_t *len){/*{{{*/
    return  spx_write(fd,(byte_t *) ctx->buf,size,len);
}/*}}}*/

err_t spx_write_from_msg_nb(int fd,struct spx_msg *ctx,const size_t size,size_t *len){/*{{{*/
    return  spx_write_nb(fd,(byte_t *) ctx->buf,size,len);
}/*}}}*/


err_t spx_set_nb(int fd) {/*{{{*/
    int flags;
    err_t err = 0;
    if (-1 == (flags = fcntl(fd, F_GETFL))) {
        err = errno;
        return -1;
    }
    flags |= O_NONBLOCK;
    if (-1 == fcntl(fd, F_SETFL, flags)) {
        err = errno;
        return -1;
    }
    return err;
}/*}}}*/

err_t spx_fwrite_string(FILE *fp,string_t s,size_t size,size_t *len){/*{{{*/
    SpxErrReset;
    *len = 0;
    err_t err = 0;
    i64_t rc = 0;
    while(*len < size){
        rc = fwrite(s + *len,sizeof(char),size,fp);
        if(0 > rc){
            if(EAGAIN == errno || EWOULDBLOCK == errno || EINTR == errno){
                SpxErrReset;
                continue;
            }
            err = errno;
            break;
        }else if(0 == rc){
            break;
        }else {
            *len += rc;
        }
    }
    if(0 != err) return err;
    if(size != *len) return EIO;
    return 0;
}/*}}}*/

err_t spx_sendfile(int sock,int fd,off_t offset,size_t size,size_t *len){/*{{{*/
    off_t offset_new = offset;
    off_t want = (off_t) size;
    err_t err = 0;
    *len = 0;
#ifdef SpxMac
    while(true){
        //want is a in/out paras means want to send length when in and sended
        //length when return
        if(0 != sendfile(fd,sock,offset_new,&want,NULL, 0)){
            if(EAGAIN == errno || EWOULDBLOCK == errno || EINTR == errno){
                SpxErrReset;
                *len += want;
                if(*len == size){
                    err = 0;
                    break;
                }
                offset_new += want;
                want = size - *len;
                continue;
            }else {
                err = errno;
                return err;
            }
        }
        *len += want;
        if(*len == size){
            err = 0;
            break;
        }
        offset_new += want;
        want = size - *len;
    }

#endif

#ifdef SpxLinux
    while(true){
        i64_t sendbytes = 0;
        sendbytes = sendfile(sock,fd,&offset_new,want);
        if(-1 == sendbytes){
            if(EAGAIN == errno || EWOULDBLOCK == errno || EINTR == errno){
                SpxErrReset;

                *len += sendbytes;
                if(*len == size){
                    break;
                }
                offset_new = offset + *len;
                want = size - *len;
                continue;
            }else {
                err = errno;
                return err;
            }
        }
        *len += sendbytes;
        if(*len == size){
            break;
        }
        offset_new = offset + *len;
        want = size - *len;
    }
#endif
    if(0 != err) return err;
    if(size != *len) return EIO;
    return 0;
}/*}}}*/


err_t spx_write_context(SpxLogDelegate *log,int fd,struct spx_msg_context *ctx){/*{{{*/
    err_t err = 0;
    size_t len = 0;
    struct spx_msg *hctx = spx_header_to_msg(ctx->header,SpxMsgHeaderSize,&err);
    if(NULL == hctx){
        SpxLog2(log,SpxLogError,err,
                "header to header-ctx is fail.");
        return err;
    }
    err = spx_write_from_msg(fd,hctx,SpxMsgHeaderSize,&len);
    if(0 != err ){
        SpxLogFmt2(log,SpxLogError,err,
                "write header size:%d,realsize:%d.",
                SpxMsgHeaderSize,len);
        goto r1;
    }

    len = 0;
    if(ctx->is_sendfile){
        err = spx_write_from_msg(fd,ctx->body,ctx->header->offset,&len);
        if(0 != err) {
            SpxLogFmt2(log,SpxLogError,err,
                    "write leading-data of body buffer:%lld,realsize:%lld.",
                    ctx->header->offset,len);
            goto r1;
        }

        len = 0;
        err = spx_sendfile(fd,ctx->sendfile_fd,ctx->sendfile_begin,ctx->sendfile_size,&len);
        if(0 != err) {
            SpxLogFmt2(log,SpxLogError,err,
                    "sendfile size:%lld,realsize:%lld.",
                    ctx->sendfile_size,len);
            goto r1;
        }
    } else {
        err = spx_write_from_msg(fd,ctx->body,ctx->header->bodylen,&len);
        if(0 != err) {
            SpxLogFmt2(log,SpxLogError,err,
                    "write body buffer:%lld,realsize:%lld.",
                    ctx->header->bodylen,len);
            goto r1;
        }
    }
r1:
    if(NULL != hctx){
        SpxMsgFree(hctx);
    }
    return err;
}/*}}}*/

err_t spx_write_context_nb(SpxLogDelegate *log,int fd,struct spx_msg_context *ctx){/*{{{*/
    err_t err = 0;
    size_t len = 0;
    struct spx_msg *hctx = spx_header_to_msg(ctx->header,SpxMsgHeaderSize,&err);
    if(NULL == hctx){
        SpxLog2(log,SpxLogError,err,
                "header convert to msg-ctx is fail.");
        return err;
    }

    err = spx_write_from_msg_nb(fd,hctx,SpxMsgHeaderSize,&len);
    if(0 != err) {
        SpxLogFmt2(log,SpxLogError,err,
                "write header size:%lld,realsize:%lld.",
                SpxMsgHeaderSize,len);
        goto r1;
    }

    len = 0;
    if(ctx->is_sendfile){
        err = spx_write_from_msg_nb(fd,ctx->body,ctx->header->offset,&len);
        if(0 != err){
            SpxLogFmt2(log,SpxLogError,err,
                    "write leading-date of body buffer:%lld,realsize:%lld.",
                    ctx->header->offset,len);
            goto r1;
        }
        len = 0;
        err = spx_sendfile(fd,ctx->sendfile_fd,ctx->sendfile_begin,ctx->sendfile_size,&len);
        if(0 != err ){
            SpxLogFmt2(log,SpxLogError,err,
                    "sendfile size:%lld,realsize:%lld.",
                    ctx->sendfile_size,len);
            goto r1;
        }
    } else {
        err = spx_write_from_msg_nb(fd,ctx->body,ctx->header->bodylen,&len);
        if(0 != err ){
            SpxLogFmt2(log,SpxLogError,err,
                    "write body buffer:%lld,realsize:%lld.",
                    ctx->header->bodylen,len);
            goto r1;
        }
    }
r1:
    if(NULL != hctx){
        SpxMsgFree(hctx);
    }
    return err;
}/*}}}*/

struct spx_msg_header *spx_read_header(SpxLogDelegate *log,int fd,err_t *err){/*{{{*/
    size_t len = 0;
    struct spx_msg *hbuff = spx_msg_new(SpxMsgHeaderSize,err);
    if(NULL == hbuff){
        SpxLog2(log,SpxLogError,*err,
                "new msg-ctx for header is fail.");
        return NULL;
    }

    *err = spx_read_to_msg(fd,hbuff,SpxMsgHeaderSize,&len);
    if(0 != *err){
        SpxLogFmt2(log,SpxLogError,*err,
                "recv header buff:%d,realsize:%d.",
                SpxMsgHeaderSize,len);
        SpxMsgFree(hbuff);
        return NULL;
    }

    struct spx_msg_header *h = spx_msg_to_header(hbuff,err);
    if(NULL == h){
        SpxLog2(log,SpxLogError,*err,
                "convert msg-ctx to header is fail.");
        SpxMsgFree(hbuff);
        return NULL;
    }
    return h;
}/*}}}*/

struct spx_msg_header *spx_read_header_nb(SpxLogDelegate *log,int fd,err_t *err){/*{{{*/
    size_t len = 0;
    struct spx_msg *hbuff = spx_msg_new(SpxMsgHeaderSize,err);
    if(NULL == hbuff){
        SpxLog2(log,SpxLogError,*err,
                "new msg-ctx for header is fail.");
        return NULL;
    }

    *err = spx_read_to_msg_nb(fd,hbuff,SpxMsgHeaderSize,&len);
    if(0 != *err){
        SpxLogFmt2(log,SpxLogError,*err,
                "recv header buff:%d,realsize:%d.",
                SpxMsgHeaderSize,len);
        SpxMsgFree(hbuff);
        return NULL;
    }

    struct spx_msg_header *h = spx_msg_to_header(hbuff,err);
    if(NULL == h){
        SpxLog2(log,SpxLogError,*err,
                "convert msg-ctx to header is fail.");
        SpxMsgFree(hbuff);
        return NULL;
    }
    return h;
}/*}}}*/

struct spx_msg *spx_read_body(SpxLogDelegate *log,int fd,size_t size,err_t *err){/*{{{*/
    size_t len = 0;
    struct spx_msg *ctx = spx_msg_new(size,err);
    if(NULL == ctx){
        SpxLog2(log,SpxLogError,*err,
                "new msg-ctx for header is fail.");
        return NULL;
    }
    *err = spx_read_to_msg(fd,ctx,size,&len);
    if(0 != *err){
        SpxLogFmt2(log,SpxLogError,*err,
                "read body buffer:%lld realsize:%lld.",
                size,len);
        SpxMsgFree(ctx);
        return NULL;
    }
    return ctx;
}/*}}}*/


struct spx_msg *spx_read_body_nb(SpxLogDelegate *log,int fd,size_t size,err_t *err){/*{{{*/
    size_t len = 0;
    struct spx_msg *ctx = spx_msg_new(size,err);
    if(NULL == ctx){
        SpxLog2(log,SpxLogError,*err,
                "new msg-ctx for header is fail.");
        return NULL;
    }
    *err = spx_read_to_msg_nb(fd,ctx,size,&len);
    if(0 != *err){
        SpxLogFmt2(log,SpxLogError,*err,
                "read body buffer:%lld realsize:%lld.",
                size,len);
        SpxMsgFree(ctx);
        return NULL;
    }
    return ctx;
}/*}}}*/


err_t spx_lazy_recv(SpxLogDelegate *log,int fd,int sock,size_t size){/*{{{*/
    err_t err = 0;
    size_t len = 0;
    struct spx_msg *ctx = spx_msg_new(SpxKB,&err);
    if(NULL == ctx){
        SpxLogFmt2(log,SpxLogError,err,
                "alloc msg for lazy recv is fail.msg size:%lld.",
                SpxKB);
        return err;
    }
    size_t totalsize = SpxMax(size,SpxKB);
    size_t recvbytes = 0;
    while(recvbytes < totalsize){
        size_t recvs = SpxMin(SpxKB,(totalsize - recvbytes));
        err = spx_read_to_msg(sock,ctx,recvs,&len);
        if(0 != err){
            SpxLogFmt2(log,SpxLogError,err,
                    "lazy recv is fail.recvsize:%lld,realsize:%lld,writed size:%lld.",
                    recvs,len,recvbytes);
            break;
        }
        len = 0;
        err = spx_write_from_msg(fd,ctx,recvs,&len);
        if(0 != err){
            SpxLogFmt2(log,SpxLogError,err,
                    "write by lazy recv is fail.recvsize:%lld,realsize:%lld,writed size:%lld.",
                    recvs,len,recvbytes);
            break;
        }
        recvbytes += recvs;
        spx_msg_clear(ctx);
    }
    SpxMsgFree(ctx);
    return err;
}/*}}}*/

err_t spx_lazy_recv_nb(SpxLogDelegate *log,int fd,int sock,size_t size){/*{{{*/
    err_t err = 0;
    size_t len = 0;
    struct spx_msg *ctx = spx_msg_new(SpxKB,&err);
    if(NULL == ctx){
        SpxLogFmt2(log,SpxLogError,err,
                "alloc msg for lazy recv is fail.msg size:%lld.",
                SpxKB);
        return err;
    }
    size_t totalsize = SpxMax(size,SpxKB);
    size_t recvbytes = 0;
    while(recvbytes < totalsize){
        size_t recvs = SpxMin(SpxKB,(totalsize - recvbytes));
        err = spx_read_to_msg_nb(sock,ctx,recvs,&len);
        if(0 != err ){
            SpxLogFmt2(log,SpxLogError,err,
                    "lazy recv is fail.recvsize:%lld,realsize:%lld,writed size:%lld.",
                    recvs,len,recvbytes);
            break;
        }
        len = 0;
        err = spx_write_from_msg_nb(fd,ctx,recvs,&len);
        if(0 != err ){
            SpxLogFmt2(log,SpxLogError,err,
                    "write by lazy recv is fail.recvsize:%lld,realsize:%lld,writed size:%lld.",
                    recvs,len,recvbytes);
            break;
        }
        recvbytes += recvs;
        spx_msg_clear(ctx);
    }
    SpxMsgFree(ctx);
    return err;
}/*}}}*/

err_t spx_lazy_mmap(SpxLogDelegate *log,char *ptr,int sock,size_t size,off_t begin){/*{{{*/
    err_t err = 0;
    size_t len = 0;
    byte_t *ctx = spx_alloc(SpxKB,sizeof(byte_t),&err);
    if(NULL == ctx){
        SpxLogFmt2(log,SpxLogError,err,
                "alloc buffer for lazy recv is fail.,msg size:%lld.",
                SpxKB);
        return err;
    }
    size_t totalsize = SpxMax(size,SpxKB);
    size_t recvbytes = 0;
    while(recvbytes < totalsize){
        size_t recvs = SpxMin(SpxKB,(totalsize - recvbytes));
        err = spx_read(sock,ctx,recvs,&len);
        if(0 != err ){
            SpxLogFmt2(log,SpxLogError,err,
                    "lazy recv is fail.recvsize:%lld,realsize:%lld,writed size:%lld.",
                    recvs,len,recvbytes);
            break;
        }
        len = 0;
        memcpy(ptr + begin + recvbytes,ctx,recvs);
        recvbytes += recvs;
        memset(ctx,0,recvs);
    }
    SpxFree(ctx);
    return err;
}/*}}}*/

err_t spx_lazy_mmap_nb(SpxLogDelegate *log,char *ptr,int sock,size_t size,off_t begin){/*{{{*/
    err_t err = 0;
    size_t len = 0;
    byte_t *ctx = spx_alloc(SpxKB,sizeof(byte_t),&err);
    if(NULL == ctx){
        SpxLogFmt2(log,SpxLogError,err,
                "alloc buffer for lazy recv is fail.,msg size:%lld.",
                SpxKB);
        return err;
    }
    size_t totalsize = SpxMax(size,SpxKB);
    size_t recvbytes = 0;
    while(recvbytes < totalsize){
        size_t recvs = SpxMin(SpxKB,(totalsize - recvbytes));
        err = spx_read_nb(sock,ctx,recvs,&len);
        if(0 != err ){
            SpxLogFmt2(log,SpxLogError,err,
                    "lazy recv is fail.recvsize:%lld,realsize:%lld,writed size:%lld.",
                    recvs,len,recvbytes);
            break;
        }
        len = 0;
        memcpy(ptr + begin + recvbytes,ctx,recvs);
        recvbytes += recvs;
        memset(ctx,0,recvs);
    }
    SpxFree(ctx);
    return err;
}/*}}}*/

/*
 * =====================================================================================
 *
 *       Filename:  spx_io.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2014/06/12 11时47分07秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#ifndef _SPX_IO_H_
#define _SPX_IO_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "spx_types.h"
#include "spx_message.h"
#include "spx_string.h"

#define SpxWriteOpen(filename,is_clear) \
    open(filename,is_clear \
            ? O_RDWR|O_CREAT|O_TRUNC \
            : O_RDWR|O_CREAT|O_APPEND,0744)
#define SpxReadOpen(filename) open(filename,O_RDONLY)
#define SpxFWriteOpen(filename,is_clear) fopen(filename,is_clear ? "w" : "a")
#define SpxFReadOpen(filename) fopen(filename,"r")

#define SpxFileExist(filename) ( 0 == access(filename,F_OK))
#define SpxFileReadable(filename) (0 == access(filename,R_OK))
#define SpxFileWritable(filename) (0 == access(filename,W_OK))
#define SpxFileExecutable(filename) (0 == access(filename,X_OK))

#define SpxMmap(fd,begin,len)  mmap(NULL,len,PROT_READ | PROT_WRITE ,MAP_SHARED,fd,begin)

    err_t spx_read(int fd,byte_t *buf,const size_t size,size_t *len);
    err_t spx_write(int fd,byte_t *buf,const size_t size,size_t *len);
    err_t spx_read_nb(int fd,byte_t *buf,const size_t size,size_t *len);
    err_t spx_write_nb(int fd,byte_t *buf,const size_t size,size_t *len);
    err_t spx_read_to_msg(int fd,struct spx_msg *ctx,const size_t size,size_t *len);
    err_t spx_read_to_msg_nb(int fd,struct spx_msg *ctx,const size_t size,size_t *len);
    err_t spx_write_from_msg(int fd,struct spx_msg *ctx,const size_t size,size_t *len);
    err_t spx_write_from_msg_nb(int fd,struct spx_msg *ctx,const size_t size,size_t *len);
    err_t spx_fwrite_string(FILE *fp,string_t s,size_t size,size_t *len);

    err_t spx_read_ack(int fd,byte_t *buf,const size_t size,size_t *len);
    err_t spx_perread_ack(int fd,byte_t *buf,const size_t size,size_t *len);
    err_t spx_write_ack(int fd,byte_t *buf,const size_t size,size_t *len);
    err_t spx_read_msg_ack(int fd,struct spx_msg *msg,const size_t size,size_t *len);
    err_t spx_write_msg_ack(int fd,struct spx_msg *ctx,size_t *len);
    err_t spx_sendfile_ack(int sock,int fd,off_t offset,size_t size,size_t *len);



    err_t spx_sendfile(int sock,int fd,off_t offset,size_t size,size_t *len);
    err_t spx_set_nb(int fd);

    spx_private spx_inline size_t spx_mmap_form_msg(char *p,off_t offset,struct spx_msg *ctx){
        size_t s = (size_t) ( ctx->last - ctx->buf);
        memcpy( p + offset,ctx->buf,s);
        return s;
    }

    spx_private spx_inline size_t spx_mmap_form_msg_with_offset(char *p,
            off_t offset,struct spx_msg *ctx,off_t off){
        size_t s = (size_t) (ctx->last  - (ctx->buf + off));
        memcpy( p + offset,ctx->buf,s);
        return s;
    }



    err_t spx_write_context(SpxLogDelegate *log,int fd,struct spx_msg_context *ctx);
    err_t spx_write_context_nb(SpxLogDelegate *log,int fd,struct spx_msg_context *ctx);
    struct spx_msg_header *spx_read_header(SpxLogDelegate *log,int fd,err_t *err);
    struct spx_msg_header *spx_read_header_nb(SpxLogDelegate *log,int fd,err_t *err);
    struct spx_msg *spx_read_body(SpxLogDelegate *log,int fd,size_t size,err_t *err);
    struct spx_msg *spx_read_body_nb(SpxLogDelegate *log,int fd,size_t size,err_t *err);
    err_t spx_lazy_recv(SpxLogDelegate *log,int fd,int sock,size_t size);
    err_t spx_lazy_recv_nb(SpxLogDelegate *log,int fd,int sock,size_t size);
    err_t spx_lazy_mmap(SpxLogDelegate *log,char *ptr,int sock,size_t size,off_t begin);
    err_t spx_lazy_mmap_nb(SpxLogDelegate *log,char *ptr,int sock,size_t size,off_t begin);


#ifdef __cplusplus
}
#endif
#endif

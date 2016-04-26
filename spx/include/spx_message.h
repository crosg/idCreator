#ifndef SPX_MESSAGE_H
#define SPX_MESSAGE_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <sys/types.h>

#include "spx_types.h"

#define SpxMsgHeaderSize (3 * sizeof(u32_t) + 2 * sizeof(u64_t) + sizeof(char))

#define SpxMsgSeekSet 0
#define SpxMsgSeekCurrent 1
#define SpxMsgSeekEnd 2

    struct spx_msg{
        uchar_t *buf;
        uchar_t *last;
        size_t busylen;
        size_t s;
        err_t err;
    };


    //add the keepalive for unclosed-connection
    //but if you use unclosed-connection,you must regedit event by yourself
    //when the request is doing over
    struct spx_msg_header{
        u32_t version;
        u32_t protocol;
        u64_t bodylen;
        u64_t offset;
        bool_t is_keepalive;
        u32_t err;
    };

    struct spx_msg_context{
        struct spx_msg_header *header;
        struct spx_msg *body;
        bool_t is_lazy_recv;
        bool_t is_sendfile;
        int sendfile_fd;
        off_t sendfile_begin;
        size_t sendfile_size;
    };

    struct spx_msg *spx_msg_new(const size_t len,err_t *err);
    err_t spx_msg_free(struct spx_msg **ctx);
    err_t spx_msg_seek(struct spx_msg *ctx,off_t offset,int whence);
    void spx_msg_peek(struct spx_msg *ctx,off_t off);
    void spx_msg_front(struct spx_msg *ctx);
    err_t spx_msg_align(struct spx_msg *ctx,off_t offset);
    void spx_msg_clear(struct spx_msg *ctx);

    err_t spx_msg_pack_int( struct spx_msg *ctx,const int v);
    err_t spx_msg_pack_i8(struct spx_msg *ctx,const i8_t v);
    err_t spx_msg_pack_i32( struct spx_msg *ctx,const i32_t v);
    err_t spx_msg_pack_i64( struct spx_msg *ctx,const i64_t v);
    err_t spx_msg_pack_u8( struct spx_msg *ctx,const u8_t v);
    err_t spx_msg_pack_u32( struct spx_msg *ctx,const u32_t v);
    err_t spx_msg_pack_u64( struct spx_msg *ctx,const u64_t v);
    err_t spx_msg_pack_double( struct spx_msg *ctx,const double v);
    err_t spx_msg_pack_float( struct spx_msg *ctx,const float v);
    err_t spx_msg_pack_true( struct spx_msg *ctx);
    err_t spx_msg_pack_false( struct spx_msg *ctx);
    err_t spx_msg_pack_string( struct spx_msg *ctx,string_t s);
    err_t spx_msg_pack_fixed_string( struct spx_msg *ctx,string_t s,size_t len);
    err_t spx_msg_pack_ubytes( struct spx_msg *ctx,const ubyte_t *b,const size_t len);
    err_t spx_msg_pack_bytes( struct spx_msg *ctx,const byte_t *b,const size_t len);
    err_t spx_msg_pack_fixed_chars( struct spx_msg *ctx,const char *b,const size_t len);

    int spx_msg_unpack_int( struct spx_msg *ctx);
    i8_t spx_msg_unpack_i8( struct spx_msg *ctx);
    i32_t spx_msg_unpack_i32( struct spx_msg *ctx);
    i64_t spx_msg_unpack_i64( struct spx_msg *ctx);
    u8_t spx_msg_unpack_u8( struct spx_msg *ctx);
    u32_t spx_msg_unpack_u32( struct spx_msg *ctx);
    u64_t spx_msg_unpack_u64( struct spx_msg *ctx);
    double spx_msg_unpack_double( struct spx_msg *ctx);
    float spx_msg_unpack_float( struct spx_msg *ctx);
    bool_t spx_msg_unpack_bool( struct spx_msg *ctx);
    string_t spx_msg_unpack_string( struct spx_msg *ctx,const size_t len,err_t *err);
    ubyte_t *spx_msg_unpack_ubytes( struct spx_msg *ctx,const size_t len,err_t *err);
    byte_t *spx_msg_unpack_bytes( struct spx_msg *ctx,const size_t len,err_t *err);


    struct spx_msg_header *spx_msg_to_header(struct spx_msg *ctx,err_t *err);
    struct spx_msg *spx_header_to_msg(struct spx_msg_header *header,size_t len,err_t *err);

void spx_header_pack(char *buf,struct spx_msg_header *h);
void spx_header_unpack(char *buf,struct spx_msg_header *h);

 void spx_msg_i2b(uchar_t *b,const i32_t n);
 i32_t spx_msg_b2i(uchar_t *b);
 void spx_msg_l2b(uchar_t *b,const i64_t n) ;
 i64_t spx_msg_b2l(uchar_t *b);
 void spx_msg_i2b_le(uchar_t *b,const i32_t n);
    i32_t spx_msg_b2i_le(uchar_t *b);

void spx_msg_ul2b(uchar_t *b,const u64_t n);
u64_t spx_msg_b2ul(uchar_t *b);

    spx_private spx_inline size_t spx_msg_size(struct spx_msg *ctx){
        return ctx->s;
    }

    spx_private spx_inline size_t spx_msg_len(struct spx_msg *ctx){
        return ctx->busylen;
    }

#define SpxMsgFree(ctx) \
    do{ \
        if(NULL != (ctx)){ \
            spx_msg_free(&(ctx)); \
            ctx = NULL; \
        } \
    }while(false)

#ifdef __cplusplus
}
#endif
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "spx_alloc.h"
#include "spx_errno.h"
#include "spx_message.h"
#include "spx_types.h"
#include "spx_string.h"
#include "spx_defs.h"

union d2i{
    double v;
    uint64_t i;
};

union f2i{
    float v;
    uint32_t i;
};


struct spx_msg *spx_msg_new(const size_t len,err_t *err){/*{{{*/
    if (0 == len){
        *err = EINVAL;
        return NULL;
    }
    struct spx_msg *ctx = spx_alloc_alone(sizeof(*ctx),err);
    if(NULL == ctx){
        return NULL;
    }
    ctx->buf = spx_alloc_alone(len ,err);
    if(NULL == ctx->buf){
        goto r1;
    }
    ctx->last = ctx->buf;
    ctx->s = len;
    ctx->busylen = 0;
    return ctx;
r1:
    SpxFree(ctx);
    return NULL;
}/*}}}*/

err_t spx_msg_free(struct spx_msg **ctx){/*{{{*/
    if(NULL != (*ctx)->buf){
        SpxFree((*ctx)->buf);
    }
    (*ctx)->last = NULL;
    (*ctx)->s = 0;
    SpxFree(*ctx);
    return 0;
}/*}}}*/

err_t spx_msg_seek(struct spx_msg *ctx,off_t offset,int whence){
    if(NULL == ctx || NULL == ctx->buf){
        return EINVAL;
    }
    switch(whence){
        case SpxMsgSeekSet :{
                                ctx->last =(uchar_t *) SpxMemIncr(ctx->buf,offset);
                                break;
                            }
        case SpxMsgSeekCurrent:{
                                   ctx->last =(uchar_t *) SpxMemIncr(ctx->last,offset);
                                   break;
                               }
        case SpxMsgSeekEnd:{
                               ctx->last =(uchar_t *) SpxMemIncr(ctx->buf,spx_msg_size(ctx) + offset);
                               break;
                           }
    }
    return 0;
}

void spx_msg_peek(struct spx_msg *ctx,off_t off){
    ctx->last = (uchar_t*) (ctx->buf + off);
}
    void spx_msg_front(struct spx_msg *ctx){
        ctx->last = ctx->buf;
    }

err_t spx_msg_align(struct spx_msg *ctx,off_t offset){
    return spx_msg_seek(ctx,offset,SpxMsgSeekCurrent);
}

void spx_msg_clear(struct spx_msg *ctx){
    SpxZeroLen(ctx->buf,ctx->s);
    ctx->last = ctx->buf;
    ctx->busylen = 0;
    ctx->err = 0;
}


err_t spx_msg_pack_int( struct spx_msg *ctx,const int v){/*{{{*/
    return spx_msg_pack_i32(ctx,(i32_t) v);
}/*}}}*/

err_t spx_msg_pack_i8(struct spx_msg *ctx,const i8_t v){/*{{{*/
    if(NULL == ctx) return EINVAL;
    *(ctx->last) = (char) v;
    (ctx->last)++;
    ctx->busylen++;
    return 0;
}/*}}}*/

err_t spx_msg_pack_i32( struct spx_msg *ctx,const i32_t v){/*{{{*/
    if(NULL == ctx) return EINVAL;
    spx_msg_i2b(ctx->last,v);
    (ctx->last) += sizeof(i32_t);
    ctx->busylen += sizeof(i32_t);
    return 0;
}/*}}}*/

err_t spx_msg_pack_i64( struct spx_msg *ctx,const i64_t v) {/*{{{*/
    if (NULL == ctx) return EINVAL;
    spx_msg_l2b(ctx->last,v);
    (ctx->last) += sizeof(i64_t);
    ctx->busylen += sizeof(i64_t);
    return 0;
}/*}}}*/
err_t spx_msg_pack_u8( struct spx_msg *ctx,const u8_t v){/*{{{*/
    if (NULL == ctx) return EINVAL;
    *(ctx->last) = (uchar_t) v;
    (ctx->last)++;
    ctx->busylen ++;
    return 0;
}/*}}}*/
err_t spx_msg_pack_u32( struct spx_msg *ctx,const u32_t v){/*{{{*/
    if(NULL == ctx) return EINVAL;
    spx_msg_i2b(ctx->last,(i32_t) v);
    (ctx->last) += sizeof(u32_t);
    ctx->busylen += sizeof(u32_t);
    return 0;
}/*}}}*/
err_t spx_msg_pack_u64( struct spx_msg *ctx,const u64_t v){/*{{{*/
    if(NULL == ctx) return EINVAL;
    spx_msg_l2b(ctx->last,(i64_t) v);
    (ctx->last) += sizeof(u64_t);
    ctx->busylen += sizeof(u64_t);
    return 0;
}/*}}}*/
err_t spx_msg_pack_double( struct spx_msg *ctx,const double v){/*{{{*/
    if (NULL == ctx) return EINVAL;
    union d2i n;
    SpxZero(n);
    n.v = v;
    spx_msg_l2b(ctx->last,n.i);
    (ctx->last) += sizeof(n.i);
    (ctx->busylen) += sizeof(n.i);
    return 0;
}/*}}}*/
err_t spx_msg_pack_float( struct spx_msg *ctx,const float v){/*{{{*/
    if (NULL == ctx) return EINVAL;
    union f2i n;
    SpxZero(n);
    n.v = v;
    spx_msg_i2b(ctx->last,n.i);
    (ctx->last) += sizeof(n.i);
    (ctx->busylen) += sizeof(n.i);
    return 0;
}/*}}}*/
err_t spx_msg_pack_true( struct spx_msg *ctx){/*{{{*/
    if (NULL == ctx) return EINVAL;
    *(ctx->last) = (uchar_t) true;
    (ctx->last)++;
    ctx->busylen ++;
    return 0;
}/*}}}*/
err_t spx_msg_pack_false( struct spx_msg *ctx){/*{{{*/
    if (NULL == ctx) return EINVAL;
    *(ctx->last) = (uchar_t) false;
    (ctx->last)++;
    ctx->busylen ++;
    return 0;
}/*}}}*/
err_t spx_msg_pack_string( struct spx_msg *ctx,string_t s){/*{{{*/
    if (NULL == ctx) return EINVAL;
    ctx->last = SpxMemcpy(ctx->last,s,spx_string_len(s));
    ctx->busylen += spx_string_len(s);
    return 0;
}/*}}}*/
err_t spx_msg_pack_fixed_string( struct spx_msg *ctx,string_t s,size_t len){/*{{{*/
    if (NULL == ctx) return EINVAL;
    ctx->last = SpxMemcpy(ctx->last,s,spx_string_len(s));
    ctx->last += len - spx_string_len(s);
    ctx->busylen += len;
    return 0;
}/*}}}*/
err_t spx_msg_pack_ubytes( struct spx_msg *ctx,const ubyte_t *b,const size_t len){/*{{{*/
    if (NULL == ctx) return EINVAL;
    ctx->last = SpxMemcpy(ctx->last,b,len);
    ctx->busylen += len;
    return 0;
}/*}}}*/
err_t spx_msg_pack_bytes( struct spx_msg *ctx,const byte_t *b,const size_t len){/*{{{*/
    if (NULL == ctx) return EINVAL;
    ctx->last =  SpxMemcpy(ctx->last,b,len);
    ctx->busylen += len;
    return 0;
}/*}}}*/


err_t spx_msg_pack_fixed_chars( struct spx_msg *ctx,const char *b,const size_t len){/*{{{*/
    if (NULL == ctx) return EINVAL;
    size_t size = strlen(b);
    ctx->last =  SpxMemcpy(ctx->last,b,size);
    ctx->last += len - size;
    ctx->busylen += len;
    return 0;
}/*}}}*/

int spx_msg_unpack_int( struct spx_msg *ctx){/*{{{*/
    return spx_msg_unpack_i32(ctx);
}/*}}}*/
i8_t spx_msg_unpack_i8( struct spx_msg *ctx){/*{{{*/
    if (NULL == ctx) return EINVAL;
    i8_t n = 0;
    n = (i8_t) *(ctx->last);
    ctx->last++;
    return n;
}/*}}}*/
i32_t spx_msg_unpack_i32( struct spx_msg *ctx){/*{{{*/
    if (NULL == ctx) return EINVAL;
    i32_t v = spx_msg_b2i(ctx->last);
    ctx->last += sizeof(i32_t);
    return v;
}/*}}}*/
i64_t spx_msg_unpack_i64( struct spx_msg *ctx){/*{{{*/
    if (NULL == ctx) return EINVAL;
    i64_t v = spx_msg_b2l(ctx->last);
    ctx->last += sizeof(i64_t);
    return v;
}/*}}}*/
u8_t spx_msg_unpack_u8( struct spx_msg *ctx){/*{{{*/
    if (NULL == ctx) return EINVAL;
    i8_t n = 0;
    n = (i8_t) *(ctx->last);
    ctx->last++;
    return n;
}/*}}}*/
u32_t spx_msg_unpack_u32( struct spx_msg *ctx){/*{{{*/
    if (NULL == ctx) return EINVAL;
    u32_t v = spx_msg_b2i(ctx->last);
    ctx->last += sizeof(u32_t);
    return v;
}/*}}}*/
u64_t spx_msg_unpack_u64( struct spx_msg *ctx){/*{{{*/
    if (NULL == ctx) return EINVAL;
    u64_t v = spx_msg_b2l(ctx->last);
    ctx->last += sizeof(u64_t);
    return v;
}/*}}}*/
double spx_msg_unpack_double( struct spx_msg *ctx){/*{{{*/
    if (NULL == ctx) return EINVAL;
    union d2i n;
    SpxZero(n);
    n.i = (u64_t) spx_msg_b2l(ctx->last);
    ctx->last += sizeof(n.i);
    return n.v;
}/*}}}*/
float spx_msg_unpack_float( struct spx_msg *ctx){/*{{{*/
    if (NULL == ctx) return EINVAL;
    union f2i n;
    SpxZero(n);
    n.i = (u32_t) spx_msg_b2i(ctx->last);
    ctx->last += sizeof(n.i);
    return n.v;
}/*}}}*/
bool_t spx_msg_unpack_bool( struct spx_msg *ctx){/*{{{*/
    if (NULL == ctx) return EINVAL;
    u8_t n = spx_msg_unpack_u8(ctx);
    return (bool_t) n;
}/*}}}*/
string_t spx_msg_unpack_string( struct spx_msg *ctx,\
        const size_t len,err_t *err){/*{{{*/
    string_t p = NULL;
    p = spx_string_newlen(ctx->last,len,err);
    ctx->last += len;
    return p;
}/*}}}*/
ubyte_t *spx_msg_unpack_ubytes( struct spx_msg *ctx,const size_t len,err_t *err){/*{{{*/
    ubyte_t *buff = spx_alloc(len,sizeof(byte_t),err);
    if(NULL == buff){
        return NULL;
    }
    size_t alive = ctx->s - (ctx->buf - ctx->last);
    memcpy(buff,ctx->last,(int) SpxMin(alive,len));
    ctx->last += len;
    return buff;
}/*}}}*/
byte_t *spx_msg_unpack_bytes( struct spx_msg *ctx,const size_t len,err_t *err){/*{{{*/
    return (byte_t *) spx_msg_unpack_ubytes(ctx,len, err);
}/*}}}*/


struct spx_msg_header *spx_msg_to_header(struct spx_msg *ctx,err_t *err){/*{{{*/
    struct spx_msg_header *header = NULL;
    if(NULL == ctx){
        *err = EINVAL;
        return NULL;
    }

    header = spx_alloc_alone(sizeof(*header),err);
    if(NULL == header){
        return NULL;
    }
    header->version = spx_msg_unpack_u32(ctx);
    header->protocol = spx_msg_unpack_u32(ctx);
    header->bodylen = spx_msg_unpack_u64(ctx);
    header->offset = spx_msg_unpack_u64(ctx);
    header->is_keepalive = spx_msg_unpack_bool(ctx);
    header->err = spx_msg_unpack_u32(ctx);
    return header;
}/*}}}*/

struct spx_msg *spx_header_to_msg(struct spx_msg_header *header,size_t len,err_t *err){/*{{{*/
    if(NULL == header){
        *err = EINVAL;
        return NULL;
    }
    struct spx_msg *ctx = spx_msg_new(len,err);
    if(NULL == ctx){
        return NULL;
    }
    spx_msg_pack_u32(ctx,header->version);
    spx_msg_pack_u32(ctx,header->protocol);
    spx_msg_pack_u64(ctx,header->bodylen);
    spx_msg_pack_u64(ctx,header->offset);
    if(header->is_keepalive){
        spx_msg_pack_true(ctx);
    } else {
        spx_msg_pack_false(ctx);
    }
    spx_msg_pack_u32(ctx,header->err);
    return ctx;
}/*}}}*/


void spx_header_unpack(char *buf,struct spx_msg_header *h){/*{{{*/
    uchar_t *tmp = (uchar_t *) buf;
    h->version = spx_msg_b2i(tmp);
    h->protocol = spx_msg_b2i(tmp + sizeof(u32_t));
    h->bodylen = spx_msg_b2l(tmp + (2 * sizeof(u32_t)));
    h->offset = spx_msg_b2l(tmp + ( 2 * sizeof(u32_t) + sizeof(u64_t)));
    h->is_keepalive = (bool_t) *(tmp + ( 2 * sizeof(u32_t) + 2 * sizeof(u64_t)));
    h->err = spx_msg_b2i(tmp + ( 2 * sizeof(u32_t) + 2 * sizeof(u64_t) + sizeof(char)));
}/*}}}*/

void spx_header_pack(char *buf,struct spx_msg_header *h){/*{{{*/
    uchar_t *tmp = (uchar_t *) buf;
    spx_msg_i2b(tmp,h->version);
    spx_msg_i2b(tmp + sizeof(u32_t),h->protocol);
    spx_msg_l2b(tmp + (2 * sizeof(u32_t)),h->bodylen);
    spx_msg_l2b(tmp + (2 * sizeof(u32_t) + sizeof(u64_t)),h->offset);
    if(h->is_keepalive){
        *(tmp + (2 * sizeof(u32_t) + 2 * sizeof(u64_t))) = (uchar_t) true;
    } else {
        *(tmp + (2 * sizeof(u32_t) + 2 * sizeof(u64_t))) = (uchar_t) false;
    }
    spx_msg_i2b(tmp + (2 * sizeof(u32_t) + 2 * sizeof(u64_t) + sizeof(char)),h->err);
}/*}}}*/




void spx_msg_i2b(uchar_t *b,const i32_t n){/*{{{*/
    *b++ = (n >> 24) & 0xFF;
    *b++ = (n >> 16) & 0xFF;
    *b++ = (n >> 8) & 0xFF;
    *b++ = n & 0xFF;
}/*}}}*/

i32_t spx_msg_b2i(uchar_t *b){/*{{{*/
    i32_t n =  (i32_t ) ((((i32_t) (*b)) << 24)
            | (((i32_t) (*(b + 1))) << 16)
            | (((i32_t) (*(b+2))) << 8)
            | ((i32_t) (*(b+3))));
    return n;
}/*}}}*/

void spx_msg_i2b_le(uchar_t *b,const i32_t n){/*{{{*/
    *b++ = n & 0xFF;
    *b++ = (n >> 8) & 0xFF;
    *b++ = (n >> 16) & 0xFF;
    *b++ = (n >> 24) & 0xFF;
}/*}}}*/

i32_t spx_msg_b2i_le(uchar_t *b){
    i32_t n =  (i32_t ) (((i32_t) (*b))
            | (((i32_t) (*(b + 1))) << 8)
            | (((i32_t) (*(b+2))) << 16)
            | (((i32_t) (*(b+3))) << 24 ));
    return n;
}

void spx_msg_l2b(uchar_t *b,const i64_t n){/*{{{*/
    *b++ = (n >> 56) & 0xFF;
    *b++ = (n >> 48) & 0xFF;
    *b++ = (n >> 40) & 0xFF;
    *b++ = (n >> 32) & 0xFF;
    *b++ = (n >> 24) & 0xFF;
    *b++ = (n >> 16) & 0xFF;
    *b++ = (n >> 8) & 0xFF;
    *b++ = n & 0xFF;
}/*}}}*/

i64_t spx_msg_b2l(uchar_t *b){/*{{{*/
    i64_t n =  (((i64_t) (*b)) << 56)
        | (((i64_t) (*(b+1))) << 48)
        | (((i64_t) (*(b + 2))) << 40)
        | (((i64_t) (*(b + 3))) << 32)
        | (((i64_t) (*(b + 4))) << 24)
        | (((i64_t) (*(b + 5))) << 16)
        | (((i64_t) (*(b + 6))) << 8)
        | ((i64_t) (*(b + 7)));
    return n;
}/*}}}*/


void spx_msg_ul2b(uchar_t *b,const u64_t n){/*{{{*/
    *b++ = (n >> 56) & 0xFF;
    *b++ = (n >> 48) & 0xFF;
    *b++ = (n >> 40) & 0xFF;
    *b++ = (n >> 32) & 0xFF;
    *b++ = (n >> 24) & 0xFF;
    *b++ = (n >> 16) & 0xFF;
    *b++ = (n >> 8) & 0xFF;
    *b++ = n & 0xFF;
}/*}}}*/
u64_t spx_msg_b2ul(uchar_t *b){/*{{{*/
    i64_t n =  (((u64_t) (*b)) << 56)
        | (((u64_t) (*(b+1))) << 48)
        | (((u64_t) (*(b + 2))) << 40)
        | (((u64_t) (*(b + 3))) << 32)
        | (((u64_t) (*(b + 4))) << 24)
        | (((u64_t) (*(b + 5))) << 16)
        | (((u64_t) (*(b + 6))) << 8)
        | ((u64_t) (*(b + 7)));
    return n;
}/*}}}*/



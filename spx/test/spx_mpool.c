#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "spx_alloc.h"
#include "spx_types.h"
//#include "spx_errno.h"
#include "spx_mpool.h"
#include "spx_object.h"

#define mem_align_ptr(p, a)                                                   \
    (ubyte_t *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

#define SpxMemPoolingSizeDefault (4 * SpxKB)
#define SpxMemPoolBufferSizeDefault (512 * SpxKB)

struct spx_mpool *spx_mpool_new(
        SpxLogDelegate *log,
        const size_t pooling_size,
        const size_t mbuff_size,
        const size_t keep_mbuff_size,
        err_t *err){/*{{{*/
    size_t real_pooing_size =
        0 == pooling_size ? SpxMemPoolingSizeDefault : pooling_size;

    size_t real_buffer_size =
        0 == mbuff_size
        ? SpxMemPoolBufferSizeDefault
        : SpxAlign(mbuff_size,SpxAlignSize);

    size_t real_keep_mbuff_size =
        0 == keep_mbuff_size
        ? SpxAlign(4 * SpxMB,real_buffer_size)
        : SpxAlign(keep_mbuff_size,real_buffer_size);

    struct spx_mpool *pool = (struct spx_mpool *)
        spx_object_new(sizeof(*pool),err);
    if(NULL == pool){
        return NULL;
    }
    pool->pooling_size = real_pooing_size;
    pool->log = log;
    pool->lg_header  = NULL;
    pool->lg_tail = NULL;
    pool->mbuff_size = SpxAlign(real_buffer_size,SpxAlignSize);
    pool->keep_mbuff_size = real_keep_mbuff_size;

    struct spx_mbuff *mbuff = (struct spx_mbuff *)
        spx_object_new(sizeof(struct spx_mbuff) + pool->mbuff_size,err);
    if(NULL == mbuff){
        SpxObjectFree(pool);
        return NULL;
    }
    pool->mb_header = mbuff;
    pool->mb_curr = mbuff;

    mbuff->ptr =SpxMemIncr(mbuff , sizeof(struct spx_mbuff));
    mbuff->freesize = pool->mbuff_size;
    return pool;
}/*}}}*/

void *spx_mpool_malloc(
       struct spx_mpool *pool,
       const size_t size,
       err_t *err
       ){/*{{{*/
    if(0 == size){
        *err = EINVAL;
        return NULL;
    }
    if(NULL == pool){
        return spx_object_new(size,err);
    }
    size_t largesize = sizeof(struct spx_large) + size;
    size_t realsize = SpxObjectAlignSize + SpxAlign(size,SpxAlignSize);
    if(realsize >= pool->pooling_size){
        struct spx_large *large = (struct spx_large *)
            spx_alloc_alone(largesize,err);//eq new large
        if(NULL ==large){
            return NULL;
        }
        large->spx_object_is_pooling = false;
        large->spx_object_refs = 1;
        large->spx_object_size = size;
        if(NULL == pool->lg_header){
            pool->lg_header = large;
            pool->lg_tail = large;
        }else {
            large->prev = pool->lg_tail;
            pool->lg_tail->next = large;
            pool->lg_tail = large;
        }
        return SpxMemIncr(large, sizeof(struct spx_large));
    } else {
        if(pool->mb_curr->freesize < realsize){
            if(NULL != pool->mb_curr->next){
                pool->mb_curr = pool->mb_curr->next;
            } else {
                struct spx_mbuff *mbuff = (struct spx_mbuff *)
                    spx_object_new(pool->mbuff_size,err);
                if(NULL == mbuff){
                    return NULL;
                }
                mbuff->ptr =SpxMemIncr(mbuff , sizeof(struct spx_mbuff));
                mbuff->freesize = pool->mbuff_size;
                pool->mb_curr->next = mbuff;
                pool->mb_curr = mbuff;
            }
        }
        struct spx_object *o =(struct spx_object *) pool->mb_curr->ptr;
        pool->mb_curr->ptr += realsize;
        o->spx_object_size = SpxAlign(size,SpxAlignSize);
        o->spx_object_refs = 1;
        o->spx_object_is_pooling = true;
        pool->mb_curr->freesize -= realsize;
        return SpxMemIncr(o ,SpxObjectAlignSize);
    }
    return NULL;
}/*}}}*/

void *spx_mpool_alloc(
        struct spx_mpool *pool,
       const size_t numbs,
       const size_t size,
       err_t *err
       ){/*{{{*/
    return spx_mpool_malloc(pool,numbs * size,err);
}/*}}}*/

void *spx_mpool_alloc_alone(
         struct spx_mpool *pool,
        const size_t size,
        err_t *err
        ){/*{{{*/
    return spx_mpool_malloc(pool,size,err);
}/*}}}*/

void *spx_mpool_realloc(
         struct spx_mpool *pool,
         void *p,
        const size_t s,
        err_t *err
        ){/*{{{*/
    if(NULL == p){
        return spx_mpool_malloc(pool,s,err);
    }

    if(NULL == pool){
        return spx_object_renew(p,s,err);
    }

    struct spx_object *o =(struct spx_object *) SpxMemDecr(p, SpxObjectAlignSize);
    if(o->spx_object_is_pooling){
        size_t algined_size = SpxAlign(s,SpxAlignSize);
        if(algined_size == o->spx_object_size){
            return o;
        }

        size_t osize = o->spx_object_size + sizeof(struct spx_object);

        if(algined_size < o->spx_object_size){
            size_t csize = o->spx_object_size - algined_size;
            o->spx_object_size = algined_size;
            if(osize == SpxPtrDecr(pool->mb_curr->ptr,o)){
                pool->mb_curr->freesize += csize;
                pool->mb_curr->ptr -= csize;
            } else {
                memset(SpxMemIncr(o,algined_size),0,csize);
            }
            return o;
        }else {
            size_t esize = algined_size - o->spx_object_size;

            if(osize == SpxPtrDecr(pool->mb_curr->ptr,o)){
                if (esize <= pool->mb_curr->freesize){
                    o->spx_object_size += esize;
                    pool->mb_curr->freesize -= esize;
                    pool->mb_curr->ptr += esize;
                    return o;
                }
            }

            if(NULL != pool->mb_curr->next){
                pool->mb_curr = pool->mb_curr->next;
            } else {
                struct spx_mbuff *mbuff = (struct spx_mbuff *)
                    spx_object_new(pool->mbuff_size,err);
                if(NULL == mbuff){
                    return NULL;
                }
                mbuff->ptr =SpxMemIncr(mbuff , sizeof(struct spx_mbuff));
                mbuff->freesize = pool->mbuff_size;
                pool->mb_curr->next = mbuff;
                pool->mb_curr = mbuff;
                struct spx_object *new =(struct spx_object *) pool->mb_curr->ptr;
                memcpy(new,o,sizeof(struct spx_object) + o->spx_object_size);
                new->spx_object_size = algined_size;
                pool->mb_curr->ptr += sizeof(struct spx_object) +  algined_size;
                pool->mb_curr->freesize -= sizeof(struct spx_object) + algined_size;
                memset(o,0,sizeof(struct spx_object) + o->spx_object_size);
                return SpxMemIncr(new,SpxObjectAlignSize);
            }
        }
    } else {
        struct spx_large *large = (struct spx_large *)
            SpxMemDecr(p, sizeof(struct spx_large));
        struct spx_large *nlarge = (struct spx_large *)
            spx_object_renew(large,s,err);
        if(NULL == nlarge){
            return NULL;
        }
        if(NULL != nlarge->prev){
            nlarge->prev->next = nlarge;
        }else{
            pool->lg_header = nlarge;
        }
        if(NULL != nlarge->next){
            nlarge->next->prev = nlarge;
        }else{
            pool->lg_tail = nlarge;
        }
        return SpxMemIncr(nlarge, sizeof(struct spx_large));
    }
    return NULL;
}/*}}}*/

bool_t spx_mpool_free(struct spx_mpool *pool,
        void *p){/*{{{*/
    if(NULL == pool){
        SpxObjectFree(p);
        return false;//no any operator
    }

    struct spx_object *o =(struct spx_object *) SpxMemDecr(p, SpxObjectAlignSize);
    if(o->spx_object_is_pooling){
        if(0 == (SpxAtomicDecr(&(o->spx_object_refs)))){
            size_t realsize = o->spx_object_size + SpxObjectAlignSize;//just reuse memory in the end
            memset(o,0,realsize);
            if( realsize == SpxPtrDecr(pool->mb_curr->ptr,o)){
                pool->mb_curr->ptr = (char *) o;
                pool->mb_curr->freesize += realsize;
            }
            return true;
        }
    } else {
        if(0 == (SpxAtomicDecr(&(o->spx_object_refs)))){
            struct spx_large *large = (struct spx_large *)
               SpxMemDecr(p, sizeof(struct spx_large));
            if(NULL == large->prev){
                pool->lg_header = large->next;
            } else {
                large->prev->next = large->next;
            }
            if(NULL == large->next){
                pool->lg_tail = large->prev;
            } else {
                large->next->prev = large->prev;
            }
            SpxFree(large);
            return true;
        }
    }
    return false;
}/*}}}*/

bool_t spx_mpool_free_force(struct spx_mpool *pool,
        void *p){/*{{{*/
    if(NULL == pool){
        SpxObjectFreeForce(p);
        return false;//no any operator
    }

    struct spx_object *o =(struct spx_object *) SpxMemDecr(p, SpxObjectAlignSize);
    if(o->spx_object_is_pooling){
        size_t realsize = o->spx_object_size + SpxObjectAlignSize;//just reuse memory in the end
        memset(o,0,realsize);
        if( realsize == SpxPtrDecr(pool->mb_curr->ptr,o)){
            pool->mb_curr->ptr = (char *) o;
            pool->mb_curr->freesize += realsize;
        }
        return true;
    } else {
        struct spx_large *large = (struct spx_large *)
            SpxMemDecr(p, sizeof(struct spx_large));
        if(NULL == large->prev){
            pool->lg_header = large->next;
        } else {
            large->prev->next = large->next;
        }
        if(NULL == large->next){
            pool->lg_tail = large->prev;
        } else {
            large->next->prev = large->prev;
        }
        SpxFree(large);
        return true;
    }
    return false;
}/*}}}*/

err_t spx_mpool_clear(struct spx_mpool *pool){/*{{{*/
    if(NULL == pool){
        return EINVAL;
    }
    struct spx_large *large = NULL;
    while(NULL != (large  = pool->lg_header)){
        pool->lg_header = large->next;
        SpxFree(large);
    }
    pool->lg_header = NULL;
    pool->lg_tail = NULL;
    struct spx_mbuff *mbuff = NULL;
    struct spx_mbuff *header = pool->mb_header;
    size_t count = 0;
    struct spx_mbuff *prev = NULL;
    while(NULL != (mbuff = pool->mb_header)){
        pool->mb_header = mbuff->next;
        if(count < pool->keep_mbuff_size){
            memset(mbuff->buff,0,pool->mbuff_size);
            mbuff->freesize = pool->mbuff_size;
            prev = mbuff;
        } else {
            SpxObjectFree(mbuff);;
            if(NULL != prev){
                prev->next = NULL;
                prev = NULL;
            }
        }
        count += pool->mbuff_size;
    }
    pool->mb_header = header;
    pool->mb_curr = header;
    pool->mb_curr->ptr = header->buff;
    return 0;
}/*}}}*/

err_t spx_mpool_destory(struct spx_mpool *pool){/*{{{*/
    if(NULL == pool){
        return EINVAL;
    }
    struct spx_large *large = NULL;
    while(NULL != (large  = pool->lg_header )){
        pool->lg_header = large->next;
        SpxFree(large);
    }
    pool->lg_header = NULL;
    pool->lg_tail = NULL;
    struct spx_mbuff *mbuff = NULL;
    while(NULL != (mbuff = pool->mb_header)){
        pool->mb_header = mbuff->next;
        SpxObjectFree(mbuff);
    }
    SpxObjectFree(pool);
    return 0;
}/*}}}*/



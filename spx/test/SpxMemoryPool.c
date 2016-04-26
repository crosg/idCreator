/*************************************************************
 *                     _ooOoo_
 *                    o8888888o
 *                    88" . "88
 *                    (| -_- |)
 *                    O\  =  /O
 *                 ____/`---'\____
 *               .'  \\|     |//  `.
 *              /  \\|||  :  |||//  \
 *             /  _||||| -:- |||||-  \
 *             |   | \\\  -  /// |   |
 *             | \_|  ''\---/''  |   |
 *             \  .-\__  `-`  ___/-. /
 *           ___`. .'  /--.--\  `. . __
 *        ."" '<  `.___\_<|>_/___.'  >'"".
 *       | | :  `- \`.;`\ _ /`;.`/ - ` : | |
 *       \  \ `-.   \_ __\ /__ _/   .-` /  /
 *  ======`-.____`-.___\_____/___.-`____.-'======
 *                     `=---='
 *  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *           佛祖保佑       永无BUG
 *
 * ==========================================================================
 *
 * this software or lib may be copied only under the terms of the gnu general
 * public license v3, which may be found in the source kit.
 *
 *       Filename:  SpxMemoryPool.c
 *        Created:  2014/12/03 14时54分33秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 ***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "spx_alloc.h"
#include "spx_types.h"
#include "SpxObject.h"
#include "SpxMemoryPool.h"

#define mem_align_ptr(p, a)                                                   \
    (ubyte_t *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

#define SpxMemPoolingMaxDefault (4 * SpxKB)
#define SpxMemPoolBufferSizeDefault (512 * SpxKB)
#define SpxMemPoolKeepBufferSizeDefault (1 * SpxMB)

struct SpxMemoryPool *spxMemoryPoolNew(
        SpxLogDelegate *log,
        const size_t poolingMax,
        const size_t bufferSize,
        const size_t keepBufferSize,
        err_t *err){/*{{{*/
    size_t realPoolingMax =
        0 == poolingMax ? SpxMemPoolingMaxDefault
        : SpxAlign(poolingMax,SpxAlignSize);

    size_t realBufferSize =
        0 == bufferSize
        ? SpxMemPoolBufferSizeDefault
        : SpxAlign(bufferSize,SpxAlignSize);

    size_t realKeepBufferSize =
        0 == keepBufferSize
        ? SpxMemPoolKeepBufferSizeDefault
        : SpxAlign(keepBufferSize,realBufferSize);

    struct SpxMemoryPool *pool = (struct SpxMemoryPool *)
        spxObjectNew(sizeof(*pool),err);
    if(NULL == pool){
        return NULL;
    }
    pool->poolingMax = realPoolingMax;
    pool->log = log;
    pool->largeObjectHeader  = NULL;
    pool->largeObjectTail = NULL;
    pool->bufferSize = SpxAlign(realBufferSize,SpxAlignSize);
    pool->keepBufferSize = realKeepBufferSize;

    struct SpxMemoryBuffer *buf = (struct SpxMemoryBuffer *)
        spxObjectNew(sizeof(struct SpxMemoryBuffer) + pool->bufferSize,err);
    if(NULL == buf){
        SpxObjectFree(pool);
        return NULL;
    }
    pool->bufferHeader = buf;
    pool->bufferCurrent = buf;

    buf->p =SpxMemIncr(buf , sizeof(struct SpxMemoryBuffer));
    buf->freeSize = pool->bufferSize;
    return pool;
}/*}}}*/

void *spxMemoryPoolAlloc(
        struct SpxMemoryPool *pool,
        const  size_t size,
        err_t *err
        ){/*{{{*/
    if(0 == size){
        *err = EINVAL;
        return NULL;
    }
    if(NULL == pool){
        return spxObjectNew(size,err);
    }
    size_t alignedSize = SpxAlign(size,SpxAlignSize);
    size_t alignedLargeSize = sizeof(struct SpxLargeObject) + alignedSize;
    size_t alignedPoolingSize = SpxObjectAlignSize + alignedSize;
    if(alignedPoolingSize >= pool->poolingMax){
        struct SpxLargeObject *large = (struct SpxLargeObject *)
            spx_alloc(1,alignedLargeSize,err);//eq new large
        if(NULL ==large){
            return NULL;
        }
        large->_spxObjectIsPooling = false;
        large->_spxObjectRefs = 1;
        large->_spxObjectSize = size;
        if(NULL == pool->largeObjectHeader){
            pool->largeObjectHeader = large;
            pool->largeObjectTail = large;
        }else {
            large->prev = pool->largeObjectTail;
            pool->largeObjectTail->next = large;
            pool->largeObjectTail = large;
        }
        return large->buf;
    } else {
        if(pool->bufferCurrent->freeSize < alignedPoolingSize){
            if(NULL != pool->bufferCurrent->next){
                pool->bufferCurrent = pool->bufferCurrent->next;
            } else {
                struct SpxMemoryBuffer *buf = (struct SpxMemoryBuffer *)
                    spxObjectNew(pool->bufferSize,err);
                if(NULL == buf){
                    return NULL;
                }
                buf->p =SpxMemIncr(buf , sizeof(struct SpxMemoryBuffer));
                buf->freeSize = pool->bufferSize;
                pool->bufferCurrent->next = buf;
                pool->bufferCurrent = buf;
            }
        }
        struct SpxObject *o =(struct SpxObject *) pool->bufferCurrent->p;
        pool->bufferCurrent->p += alignedPoolingSize;
        o->_spxObjectSize = alignedSize;
        o->_spxObjectRefs = 1;
        o->_spxObjectIsPooling = true;
        pool->bufferCurrent->freeSize -= alignedPoolingSize;
        return o->buf;
    }
    return NULL;
}/*}}}*/

void *spxMemoryPoolAllocNumbs(
        struct SpxMemoryPool *pool,
        const size_t numbs,
        const size_t size,
        err_t *err
        ){/*{{{*/
    return spxMemoryPoolAlloc(pool,numbs * size,err);
}/*}}}*/

void *spxMemoryPoolReAlloc(
        struct SpxMemoryPool *pool,
        void *p,
        const size_t s,
        err_t *err
        ){/*{{{*/
    if(0 == s){
        *err = EINVAL;
        return NULL;
    }
    if(NULL == p){
        return spxMemoryPoolAlloc(pool,s,err);
    }
    if(NULL == pool){
        return spxObjectReNew(p,s,err);
    }

    struct SpxObject *o =(struct SpxObject *) SpxMemDecr(p, SpxObjectAlignSize);
    if(o->_spxObjectIsPooling){
        size_t alignedSize = SpxAlign(s,SpxAlignSize);
        if(alignedSize == o->_spxObjectSize){
            return p;
        }

        size_t allocedSize = o->_spxObjectSize + sizeof(struct SpxObject);

        if(alignedSize < o->_spxObjectSize){
            size_t decrSize = o->_spxObjectSize - alignedSize;
            o->_spxObjectAvail = decrSize;
            //o is the last object of the buffer
            if(allocedSize == SpxPtrDecr(pool->bufferCurrent->p,o)){
                pool->bufferCurrent->freeSize += decrSize;
                pool->bufferCurrent->p -= decrSize;
            } else {
                //clear end of pointer p
                memset(SpxMemIncr(p,alignedSize),0,decrSize);
            }
            return p;
        }else {
            if(alignedSize + SpxObjectAlignSize <= pool->poolingMax){
                size_t incrSize = alignedSize - o->_spxObjectIsPooling;
                //o is the last object of the buffer
                if(allocedSize == SpxPtrDecr(pool->bufferCurrent->p,o)){
                    if (incrSize <= pool->bufferCurrent->freeSize){
                        o->_spxObjectSize += incrSize;
                        pool->bufferCurrent->freeSize -= incrSize;
                        pool->bufferCurrent->p += incrSize;
                        return p;
                    }
                }

                if(NULL != pool->bufferCurrent->next){
                    pool->bufferCurrent = pool->bufferCurrent->next;
                } else {
                    struct SpxMemoryBuffer *buf = (struct SpxMemoryBuffer *)
                        spxObjectNew(pool->bufferSize,err);
                    if(NULL == buf){
                        return NULL;
                    }
                    buf->p =SpxMemIncr(buf , sizeof(struct SpxMemoryBuffer));
                    buf->freeSize = pool->bufferSize;
                    pool->bufferCurrent->next = buf;
                    pool->bufferCurrent = buf;
                    struct SpxObject *new =(struct SpxObject *) pool->bufferCurrent->p;
                    memcpy(new,o,allocedSize);
                    new->_spxObjectSize = alignedSize;
                    pool->bufferCurrent->p += sizeof(struct SpxObject) +  alignedSize;
                    pool->bufferCurrent->freeSize -= sizeof(struct SpxObject) + alignedSize;
                    memset(o,0,sizeof(struct SpxObject) + o->_spxObjectSize);
                    return new->buf;
                }
            } else {
                // new a large object
                return spxMemoryPoolAlloc(pool,s,err);
            }
        }
    } else {
        struct SpxLargeObject *large = (struct SpxLargeObject *)
            SpxMemDecr(p, sizeof(struct SpxLargeObject));
        struct SpxLargeObject *newLarge = (struct SpxLargeObject *)
            spx_realloc(large,s,err);
        if(NULL == newLarge){
            return NULL;
        }
        if(NULL != newLarge->prev){
            newLarge->prev->next = newLarge;
        }else{
            pool->largeObjectHeader = newLarge;
        }
        if(NULL != newLarge->next){
            newLarge->next->prev = newLarge;
        }else{
            pool->largeObjectTail = newLarge;
        }
        return newLarge->buf;
    }
    return NULL;
}/*}}}*/

bool_t spxMemoryPoolFree(struct SpxMemoryPool *pool,
        void *p){/*{{{*/
    if(NULL == pool){
        SpxObjectFree(p);
        return false;//no any operator
    }

    struct SpxObject *o =(struct SpxObject *) SpxMemDecr(p, SpxObjectAlignSize);
    if(o->_spxObjectIsPooling){
        if(0 == (SpxAtomicDecr(&(o->_spxObjectRefs)))){
            size_t realsize = o->_spxObjectSize + SpxObjectAlignSize;//just reuse memory in the end
            memset(o,0,realsize);
            if( realsize == SpxPtrDecr(pool->bufferCurrent->p,o)){
                pool->bufferCurrent->p = (char *) o;
                pool->bufferCurrent->freeSize += realsize;
            }
            return true;
        }
    } else {
        if(0 == (SpxAtomicDecr(&(o->_spxObjectRefs)))){
            struct SpxLargeObject *large = (struct SpxLargeObject *)
                SpxMemDecr(p, sizeof(struct SpxLargeObject));
            if(NULL == large->prev){
                pool->largeObjectHeader = large->next;
            } else {
                large->prev->next = large->next;
            }
            if(NULL == large->next){
                pool->largeObjectTail = large->prev;
            } else {
                large->next->prev = large->prev;
            }
            if(!large->_spxObjectIsPooling){
                if(0 == SpxAtomicDecr(&(large->_spxObjectRefs))){
                    SpxFree(large);
                    return true;
                }
            }
        }
    }
    return false;
}/*}}}*/

bool_t spxMemoryPoolFreeForce(struct SpxMemoryPool *pool,
        void *p){/*{{{*/
    if(NULL == pool){
        SpxObjectFreeForce(p);
        return false;//no any operator
    }

    struct SpxObject *o =(struct SpxObject *) SpxMemDecr(p, SpxObjectAlignSize);
    if(o->_spxObjectIsPooling){
        size_t realsize = o->_spxObjectSize + SpxObjectAlignSize;//just reuse memory in the end
        memset(o,0,realsize);
        if( realsize == SpxPtrDecr(pool->bufferCurrent->p,o)){
            pool->bufferCurrent->p = (char *) o;
            pool->bufferCurrent->freeSize += realsize;
        }
        return true;
    } else {
        struct SpxLargeObject *large = (struct SpxLargeObject *)
            SpxMemDecr(p, sizeof(struct SpxLargeObject));
        if(NULL == large->prev){
            pool->largeObjectHeader = large->next;
        } else {
            large->prev->next = large->next;
        }
        if(NULL == large->next){
            pool->largeObjectTail = large->prev;
        } else {
            large->next->prev = large->prev;
        }
        SpxFree(large);
        return true;
    }
    return false;
}/*}}}*/

err_t spxMemoryPoolClear(struct SpxMemoryPool *pool){/*{{{*/
    if(NULL == pool){
        return EINVAL;
    }
    struct SpxLargeObject *large = NULL;
    while(NULL != (large  = pool->largeObjectHeader)){
        pool->largeObjectHeader = large->next;
        SpxFree(large);
    }
    pool->largeObjectHeader = NULL;
    pool->largeObjectTail = NULL;
    struct SpxMemoryBuffer *buf = NULL;
    struct SpxMemoryBuffer *header = pool->bufferHeader;
    size_t count = 0;
    struct SpxMemoryBuffer *prev = NULL;
    while(NULL != (buf = pool->bufferHeader)){
        pool->bufferHeader = buf->next;
        if(count < pool->keepBufferSize){
            memset(buf->buf,0,pool->bufferSize);
            buf->freeSize = pool->bufferSize;
            prev = buf;
        } else {
            SpxObjectFreeForce(buf);
            if(NULL != prev){
                prev->next = NULL;
                prev = NULL;
            }
        }
        count += pool->bufferSize;
    }
    pool->bufferHeader = header;
    pool->bufferCurrent = header;
    pool->bufferCurrent->p = header->buf;
    return 0;
}/*}}}*/

err_t spxMemoryPoolDestory(struct SpxMemoryPool *pool){/*{{{*/
    if(NULL == pool){
        return EINVAL;
    }
    struct SpxLargeObject *large = NULL;
    while(NULL != (large  = pool->largeObjectHeader )){
        pool->largeObjectHeader = large->next;
        SpxFree(large);
    }
    pool->largeObjectHeader = NULL;
    pool->largeObjectTail = NULL;
    struct SpxMemoryBuffer *buf = NULL;
    while(NULL != (buf = pool->bufferHeader)){
        pool->bufferHeader = buf->next;
        SpxObjectFreeForce(buf);
    }
    SpxObjectFreeForce(pool);
    return 0;
}/*}}}*/




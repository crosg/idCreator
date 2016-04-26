/***********************************************************************
 *                              _ooOoo_
 *                             o8888888o
 *                             88" . "88
 *                             (| -_- |)
 *                              O\ = /O
 *                          ____/`---'\____
 *                        .   ' \\| |// `.
 *                         / \\||| : |||// \
 *                       / _||||| -:- |||||- \
 *                         | | \\\ - /// | |
 *                       | \_| ''\---/'' | |
 *                        \ .-\__ `-` ___/-. /
 *                     ___`. .' /--.--\ `. . __
 *                  ."" '< `.___\_<|>_/___.' >'"".
 *                 | | : `- \`.;`\ _ /`;.`/ - ` : | |
 *                   \ \ `-. \_ __\ /__ _/ .-` / /
 *           ======`-.____`-.___\_____/___.-`____.-'======
 *                              `=---='
 *           .............................................
 *                    佛祖镇楼                  BUG辟易
 *            佛曰:
 *                    写字楼里写字间，写字间里程序员；
 *                    程序人员写程序，又拿程序换酒钱。
 *                    酒醒只在网上坐，酒醉还来网下眠；
 *                    酒醉酒醒日复日，网上网下年复年。
 *                    但愿老死电脑间，不愿鞠躬老板前；
 *                    奔驰宝马贵者趣，公交自行程序员。
 *                    别人笑我忒疯癫，我笑自己命太贱；
 *                    不见满街漂亮妹，哪个归得程序员？
 * ==========================================================================
 *
 * this software or lib may be copied only under the terms of the gnu general
 * public license v3, which may be found in the source kit.
 *
 *       Filename:  SpxMemoryPool.h
 *        Created:  2014/12/03 10时04分57秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 ****************************************************************************/
#ifndef _SPXMEMORYPOOL_H_
#define _SPXMEMORYPOOL_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>

#include "spx_defs.h"
#include "spx_types.h"
#include "spx_atomic.h"
#include "SpxObject.h"

    struct SpxMemoryBuffer{
        char *p;
        struct SpxMemoryBuffer *next;
        size_t freeSize;
        char buf[0];
    };

    struct SpxLargeObject{//large is extends spx_object;
        struct SpxLargeObject *prev;
        struct SpxLargeObject *next;
        SpxObjectBase ;//must input there top buff
        char buf[0];
    };

    struct SpxMemoryPool{
        SpxLogDelegate *log;
        size_t poolingMax;
        size_t bufferSize;
        size_t keepBufferSize;
        struct SpxMemoryBuffer *bufferHeader;
        struct SpxMemoryBuffer *bufferCurrent;
        struct SpxLargeObject *largeObjectHeader;
        struct SpxLargeObject *largeObjectTail;
    };

    struct SpxMemoryPool *spxMemoryPoolNew(
            SpxLogDelegate *log,
            const size_t poolingMax,
            const size_t bufferSize,
            const size_t keepBufferSize,
            err_t *err
            );

    void *spxMemoryPoolAllocNumbs(
            struct SpxMemoryPool *pool,
            const size_t numbs,
            const size_t size,
            err_t *err
            );

    void *spxMemoryPoolAlloc(
            struct SpxMemoryPool *pool,
            const  size_t size,
            err_t *err
            );

    void *spxMemoryPoolReAlloc(
        struct SpxMemoryPool *pool,
        void *p,
        const size_t s,
        err_t *err
        );

    bool_t spxMemoryPoolFree(
            struct SpxMemoryPool *pool,
            void *p
            );

    bool_t spxMemoryPoolFreeForce(
            struct SpxMemoryPool *pool,
            void *p
            );

    err_t spxMemoryPoolClear(
            struct SpxMemoryPool *pool
            );

    err_t spxMemoryPoolDestory(
            struct SpxMemoryPool *pool
            );

#define SpxMemoryPoolFree(pool,p) \
    do { \
        if(NULL != p && (spxMemoryPoolFree(pool,p))){ \
            p = NULL; \
        } \
    }while(false)

#define SpxMemoryPoolFreeForce(pool,p) \
    do { \
        if(NULL != p) {\
            spxMemoryPoolFreeForce(pool,p);\
            p = NULL; \
        } \
    }while(false)


#define SpxMemoryPoolDestory(p) \
    do { \
        if(NULL != p) {\
            spxMemoryPoolDestory(p); \
            p = NULL; \
        } \
    }while(false)


#ifdef __cplusplus
}
#endif
#endif

#ifdef __cplusplus
}
#endif
#endif

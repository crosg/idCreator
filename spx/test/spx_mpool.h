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
 *       Filename:  spx_mpool.h
 *        Created:  2014/10/11 08时52分00秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 ****************************************************************************/
#ifndef _SPX_MPOOL_H_
#define _SPX_MPOOL_H_
#ifdef __cplusplus
extern "C" {
#endif


#include <stdlib.h>
#include <stdio.h>

#include "spx_defs.h"
#include "spx_types.h"
#include "spx_atomic.h"

    struct spx_mbuff{
        struct spx_mbuff *next;
        size_t freesize;
        char *ptr;
        char buff[0];
    };

    struct spx_large{//large is extends spx_object;
        struct spx_large *prev;
        struct spx_large *next;
        SpxObjectStruct;//must input there top buff
        char buff[0];
    };

    struct spx_mpool{
        SpxLogDelegate *log;
        size_t pooling_size;
        size_t mbuff_size;
        size_t keep_mbuff_size;
        struct spx_mbuff *mb_header;
        struct spx_mbuff *mb_curr;
        struct spx_large *lg_header;
        struct spx_large *lg_tail;
    };

    struct spx_mpool *spx_mpool_new(
            SpxLogDelegate *log,
            const size_t pooling_size,
            const size_t mbuff_size,
            const size_t keep_mbuff_size,
            err_t *err
            );

    void *spx_mpool_malloc(
            struct spx_mpool *pool,
            const size_t size,
            err_t *err
            );

    void *spx_mpool_realloc(
            struct spx_mpool *pool,
            void *p,
            const size_t s,
            err_t *err
            );

    void *spx_mpool_alloc(
            struct spx_mpool *pool,
            const size_t numbs,
            const size_t size,
            err_t *err
            );

    void *spx_mpool_alloc_alone(
            struct spx_mpool *pool,
            const  size_t size,
            err_t *err
            );

    bool_t spx_mpool_free(
            struct spx_mpool *pool,
            void *p
            );

    bool_t spx_mpool_free_force(
            struct spx_mpool *pool,
            void *p
            );

    err_t spx_mpool_clear(
            struct spx_mpool *pool
            );

    err_t spx_mpool_destory(
            struct spx_mpool *pool
            );

#define SpxMemPoolFree(pool,p) \
    do { \
        if(NULL != p && (spx_mpool_free(pool,p))){ \
            p = NULL; \
        } \
    }while(false)

#define SpxMemPoolFreeForce(pool,p) \
    do { \
        if(NULL != p) {\
            spx_mpool_free_force(pool,p);\
            p = NULL; \
        } \
    }while(false)


#define SpxMemPoolDestory(p) \
    do { \
        if(NULL != p) {\
            spx_mpool_destory(p); \
            p = NULL; \
        } \
    }while(false)


#ifdef __cplusplus
}
#endif
#endif

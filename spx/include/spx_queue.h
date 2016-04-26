/*
 * =====================================================================================
 *
 * this software or lib may be copied only under the terms of the gnu general
 * public license v3, which may be found in the source kit.
 *
 *       Filename:  spx_queue.h
 *        Created:  2014/08/08 17时53分42秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 * =====================================================================================
 */
#ifndef _SPX_QUEUE_H_
#define _SPX_QUEUE_H_
#ifdef __cplusplus
extern "C" {
#endif


#include <stdlib.h>
#include <stdio.h>

#include "spx_types.h"

    typedef err_t SpxQueueNodeFreeDelegate(void **v);
    struct spx_queue_node{
        struct spx_queue_node *prev;
        void *v;
    };

    struct spx_queue{
        size_t size;
        SpxLogDelegate *log;
        struct spx_queue_node *header;
        struct spx_queue_node *tail;
        SpxQueueNodeFreeDelegate *free;
    };

    struct spx_queue *spx_queue_new(SpxLogDelegate *log,
            SpxQueueNodeFreeDelegate *free,
            err_t *err);

    void *spx_queue_pop(struct spx_queue *q,err_t *err);
    err_t spx_queue_push(struct spx_queue *q,void *n);
    err_t spx_queue_free(struct spx_queue **q);
    err_t spx_queue_clear(struct spx_queue *q);

#ifdef __cplusplus
}
#endif
#endif

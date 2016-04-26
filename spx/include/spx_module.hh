/*
 * the functions support the mempool.
 * WARN:
 * 1: the functions is not thread-safe.
 * 2 :the alone object can free \
 *      but the small-object cannot free,\
 *      and destroy it by clear the buf
 * 3: the clenup support to res struct,
 *      such as fd,locker and so on
 */

#ifndef SPX_MPOOL_H
#define SPX_MPOOL_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>

#include "spx_types.h"

typedef err_t (SpxMempoolCleanDelegate)(void *e);

struct spx_mpool_buf;
struct spx_mpool_alone;
struct spx_mpool_cleanup;
struct spx_mpool;


extern struct spx_mpool *spx_mpool_init(const size_t size,\
        const size_t limit,err_t *err);
extern void *spx_mpool_alloc(struct spx_mpool * const p,\
        const size_t s,err_t *err);

void *spx_mpool_cleanup_alloc(struct spx_mpool * const p,
        const size_t size,SpxMempoolCleanDelegate *f,
        err_t *err);
err_t spx_mpool_cleanup_free(const struct spx_mpool * const p,
        void **e);
err_t spx_mpool_reset(struct spx_mpool * const p);
extern err_t spx_mpool_free(struct spx_mpool * const p,void **e);
extern err_t spx_mpool_destroy(struct spx_mpool **p);

#ifdef __cplusplus
}
#endif
#endif

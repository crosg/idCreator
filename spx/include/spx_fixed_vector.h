/*
 * =====================================================================================
 *
 *       Filename:  spx_fixed_vector.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2014/06/10 23时01分36秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#ifndef _SPX_FIXED_VECTOR_H_
#define _SPX_FIXED_VECTOR_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "spx_types.h"
#include "spx_vector.h"
#include "spx_thread.h"


    typedef void *SpxFixedVectorValueNewDeledate(size_t idx,void *arg,err_t *err);

    struct spx_fixed_vector{
        struct spx_vector_node *header;
        struct spx_vector_node *tail;
        struct spx_vector_node *busy_header;
        struct spx_vector_node *busy_tail;
        SpxVectorValueFreeDelegate *node_free_handle;
        size_t size;
        size_t busysize;
        SpxLogDelegate *log;
        pthread_mutex_t *locker;
    };


    struct spx_fixed_vector *spx_fixed_vector_new(SpxLogDelegate *log,\
            size_t size,\
            SpxFixedVectorValueNewDeledate *value_new_handle,
            void *arg,\
            SpxVectorValueFreeDelegate *node_free_handle,\
            err_t *err);
    err_t spx_fixed_vector_free(struct spx_fixed_vector **vector);
    err_t spx_fixed_vector_push(struct spx_fixed_vector *vector,void *v);
    void *spx_fixed_vector_pop(struct spx_fixed_vector *vector, err_t *err);


    struct spx_fixed_vector_iter{
        struct spx_fixed_vector *vector;
        struct spx_vector_node *curr;
    };
    struct spx_fixed_vector_iter *spx_fixed_vector_iter_init(struct spx_fixed_vector *vector,\
            err_t *err);
    err_t spx_fixed_vector_iter_destroy(struct spx_vector_iter **iter);
    void *spx_fixed_vector_iter_next(struct spx_vector_iter *iter) ;




#ifdef __cplusplus
}
#endif
#endif

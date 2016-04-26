#ifndef SPX_VECTOR_H
#define SPX_VECTOR_H
#ifdef __cplusplus
extern "C" {
#endif

#include "spx_types.h"

    struct spx_vector_node{
        struct spx_vector_node *prev;
        struct spx_vector_node *next;
        void *v;
    };

    typedef err_t (SpxVectorValueFreeDelegate)(void **v);

    struct spx_vector{
        struct spx_vector_node *header;
        struct spx_vector_node *tail;
        SpxVectorValueFreeDelegate *handle;
        size_t size;
        SpxLogDelegate *log;
    };


    struct spx_vector_iter{
        struct spx_vector *vector;
        struct spx_vector_node *curr;
    };

    struct spx_vector *spx_vector_init(SpxLogDelegate *log,\
            SpxVectorValueFreeDelegate *handle,err_t *err);
    err_t spx_vector_free(struct spx_vector **vector);
    err_t spx_vector_add(struct spx_vector *vector,void *v);
    void *spx_vector_get(struct spx_vector *vector,size_t idx,err_t *err);
    err_t spx_vector_push(struct spx_vector *vector,void *v);
    void *spx_vector_pop(struct spx_vector *vector, err_t *err);




    struct spx_vector_iter *spx_vector_iter_new(struct spx_vector *vector,\
            err_t *err);
    err_t spx_vector_iter_free(struct spx_vector_iter **iter);
    void *spx_vector_iter_next(struct spx_vector_iter *iter) ;
    void spx_vector_iter_reset(struct spx_vector_iter *iter);


#ifdef __cplusplus
}
#endif
#endif

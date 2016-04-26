#ifndef SPX_LIST_H
#define SPX_LIST_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#include "spx_types.h"

    typedef err_t (SpxListNodeFreeDelegate)(void **arg);
    typedef void *(SpxListNodeNewDelegate)(size_t i,void *arg,err_t *err);

    struct spx_list_node{
        void *v;
    };

    struct spx_list{
        SpxLogDelegate *log;
        size_t free_size;
        size_t busy_size;
        size_t size;
        SpxListNodeFreeDelegate *node_free;
        struct spx_list_node *nodes;
    };

    struct spx_list *spx_list_new(SpxLogDelegate *log,\
            size_t init_size,\
            SpxListNodeFreeDelegate *node_free,\
            err_t *err);

    struct spx_list *spx_list_init(SpxLogDelegate *log,\
            size_t init_size,\
            SpxListNodeNewDelegate *new,\
            void *arg,\
            SpxListNodeFreeDelegate *free,\
            err_t *err
            );

    void *spx_list_get(struct spx_list *list,int idx);
    void *spx_list_get_and_out(struct spx_list *list,int idx);
    err_t spx_list_delete(struct spx_list *list,int idx);
    err_t spx_list_insert(struct spx_list *list,int idx,void *v);
    err_t spx_list_add(struct spx_list *list,void *v);
    err_t spx_list_free(struct spx_list **list);
    err_t spx_list_set(struct spx_list *list,int idx,void *v);

#ifdef __cplusplus
}
#endif
#endif

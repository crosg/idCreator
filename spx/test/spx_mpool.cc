#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "spx_mpool.h"
#include "spx_alloc.h"
#include "spx_types.h"
#include "spx_errno.h"

#define mem_align(d, a)     (((d) + (a - 1)) & ~(a - 1))
#define mem_align_ptr(p, a)                                                   \
    (ubyte_t *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))

//one tips,haha!
#define SpxObject \
    size_t size; \
    ubyte_t e[0]

struct spx_mpool_buff{
    struct spx_mpool_buff *last;
    struct spx_mpool_buff *end;
    struct spx_mpool_buff *n;
    ubyte_t b[0];
};

struct spx_mpool_alone{
    struct spx_mpool_alone *p;
    struct spx_mpool_alone *n;
    SpxObject;
};

struct spx_mpool_node{
    SpxObject;
};

struct spx_mpool{
    struct spx_mpool_buff *cbuf;//the pointer to current buffer
    struct spx_mpool_buff *hbuf;//the pointer to header buffer
    struct spx_mpool_alone *as;
    struct spx_mpool_cleanup *cs;
    size_t limit;//the object size in the buf
    size_t size;//the buf size
};

struct spx_mpool_cleanup {
    SpxMempoolCleanDelegate *f;
    struct spx_mpool_cleanup *n;
    struct spx_mpool_cleanup *p;
    ubyte_t e[0];
};

struct spx_mpool *spx_mpool_init(const size_t size,
        const size_t limit,err_t *err){/*{{{*/
    if(0 == size || 0 == limit || (size < limit)){
        *err = EINVAL;
        return NULL;
    }
    struct spx_mpool *p = spx_alloc_alone(sizeof(*p),err);
    if(NULL == p){
        return NULL;
    }
    p->size = size;
    p->limit = limit;
    return p;
}/*}}}*/

void *spx_mpool_alloc(struct spx_mpool * const p,
        const size_t s,err_t *err){/*{{{*/
    if(0 == s || NULL == p){
        *err = EINVAL;
        return NULL;
    }
    void *e = NULL;
    struct spx_mpool_alone *a = NULL;
    if(s > p->limit){//alone
        a = spx_alloc_alone(sizeof(*a),err);
        if(NULL == a){
            return NULL;
        }
        if(NULL == p->as){
            p->as = a;
        }else {
            p->as->p = a;
            a->n = p->as;
            p->as = a;
        }
        a->size = s;
        e = a + sizeof(struct spx_mpool_alone);
    } else {
        struct spx_mpool_buff *b = p->cbuf;
        if(NULL == b) {
            b = spx_memalign_alloc(sizeof(*b) + p->size,err);
            if(NULL == b){
                return NULL;
            }
            b->last = b + sizeof(struct spx_mpool_buff);;
            b->end = b + p->size;
            p->cbuf = b;
            p->hbuf = b;
        }
        size_t size = s + sizeof(struct spx_mpool_node);
        while(true) {
            struct spx_mpool_node *o = NULL;
            o =(struct spx_mpool_node *) mem_align_ptr(b->last,size);
            if(((size_t) ((char *) b->end - (char *) o)) < size){ // ptr diff
                b = p->cbuf->n;
                if(NULL == b){
                    b = spx_memalign_alloc(sizeof(*b) + p->size,err);
                    if(NULL == b){
                        return NULL;
                    }
                    b->last = b + sizeof(struct spx_mpool_buff);
                    b->end = b + p->size;
                    p->cbuf->n = b;
                    p->cbuf = b;
                }
                continue;
            }
            b->last += size;
            o->size = s;
            e = (void *) (o + sizeof(struct spx_mpool_node));
            break;
        }
    }
    return e;
}/*}}}*/

void *spx_mpool_cleanup_alloc(struct spx_mpool * const p,
        const size_t size,SpxMempoolCleanDelegate *f,
        err_t *err){/*{{{*/
    //if the f is null,you must use spx_mpool_alloc
    if(0 == size || NULL == f){
        *err = EINVAL;
        return NULL;
    }
    struct spx_mpool_cleanup *ptr = NULL;
    ptr = spx_alloc_alone(size + sizeof(*ptr),err);
    if(NULL == ptr){
            return NULL;
        }
    ptr->f = f;
    if(NULL == p->cs){
        p->cs = ptr;
    } else {
        ptr->n = p->cs;
        p->cs->p = ptr;
        p->cs = ptr;
    }
    void *e = ptr + sizeof(struct spx_mpool_cleanup);
    return e;

}/*}}}*/

err_t spx_mpool_cleanup_free(const struct spx_mpool * const p,
        void **e){/*{{{*/
    if(NULL == e || NULL == p){
        return EINVAL;
    }
    struct spx_mpool_cleanup *ptr = (struct spx_mpool_cleanup *) (*e) - sizeof(struct spx_mpool_cleanup);
    if(NULL == ptr){
        return ENXIO;
    }
    ptr->f(&ptr);
    SpxFree(ptr);
    return 0;
}/*}}}*/

err_t spx_mpool_reset(struct spx_mpool * const p){/*{{{*/
    //cleanup
    if(NULL == p){
        return EINVAL;
    }
    struct spx_mpool_cleanup *pup = NULL;
    while((NULL != (pup = p->cs))){
        pup->f(pup + sizeof(struct spx_mpool_cleanup));
        p->cs = pup->n;
        SpxFree(pup);
    }

    //lager object
    struct spx_mpool_alone *pa = NULL;
    while((NULL != (pa = p->as))){
        p->as = pa->n;
        SpxFree(pa);
    }

    //buf
    struct spx_mpool_buff *pb = p->hbuf;
    while(NULL != pb){
        memset((char *)(pb + sizeof(struct spx_mpool_buff)), 0,p->size);
        pb->last = pb + sizeof(struct spx_mpool_buff);
        pb = pb->n;
    }
    p->cbuf = p->hbuf;
    return 0;
}/*}}}*/

err_t spx_mpool_free(struct spx_mpool *const p,void **e){/*{{{*/
    err_t rc = 0;
    struct spx_mpool_node *n =(struct spx_mpool_node *) (((char *)(*e)) - sizeof(struct spx_mpool_node));
    if(n->size > p->limit){
        struct spx_mpool_alone *ptr =(struct spx_mpool_alone *)\
                                   *e - sizeof(struct spx_mpool_alone);
        if(NULL == ptr){
            rc = ENXIO;
            return rc;
        }
        if(NULL != ptr->p){
            ptr->p->n = ptr->n;
        } else {
            p->as = ptr->n;
        }
        SpxFree(ptr);
    }
    return rc;
}/*}}}*/

err_t spx_mpool_destroy(struct spx_mpool **p){/*{{{*/
    //cleanup
    if(NULL == p){
        return EINVAL;
    }
    if(NULL == *p) {
        return 0;
    }
    struct spx_mpool_cleanup *pup = NULL;
    while((NULL != (pup = (*p)->cs))){
        pup->f(pup + sizeof(struct spx_mpool_cleanup));
        (*p)->cs = pup->n;
        SpxFree(pup);
    }

    //lager object
    struct spx_mpool_alone *pa = NULL;
    while((NULL != (pa = (*p)->as))){
        (*p)->as = pa->n;
        SpxFree(pa);
    }

    //buf
    struct spx_mpool_buff *pb = NULL;
    while(NULL != (pb = (*p)->hbuf)){
        (*p)->hbuf = pb->n;
        SpxFree(pb);
    }

    SpxFree(*p);
    return 0;
}/*}}}*/


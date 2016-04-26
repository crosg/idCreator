#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "spx_types.h"
#include "spx_defs.h"
#include "spx_alloc.h"

void *spx_malloc(const size_t s,err_t *err){
    if(0 == s){
        *err = EINVAL;
        return NULL;
    }
    void *p = malloc(s);
    if(NULL == p){
        *err = 0 == errno ? ENOMEM : errno;
    }
    return p;
}

void *spx_alloc(const size_t numbs,const size_t s,err_t *err){
    if(0 == s || 0 == numbs){
        *err = EINVAL;
        return NULL;
    }
    void *p = calloc(numbs,s);
    if(NULL == p){
        *err = 0 == errno ? ENOMEM : errno;
        return NULL;
    }
    return p;
}

void *spx_alloc_alone(const size_t s,err_t *err){
    if(0 == s){
        *err = EINVAL;
        return NULL;
    }
    return spx_alloc(1,s,err);
}

void *spx_memalign_alloc(const size_t size,err_t *err) {
    void *p = NULL;
    if(0 != (*err = posix_memalign(&p, SPX_ALIGN_SIZE, size))){
        return NULL;
    }
    return p;
}

void *spx_alloc_mptr(const size_t numbs,err_t *err){
    void *p = calloc(numbs,SpxPtrSize);
    if(NULL == p){
        *err = 0 == errno ? ENOMEM : errno;
    }
    return p;
}

void *spx_realloc(void *p,const size_t size,err_t *err){
    void *ptr = realloc(p,size);
    if(NULL == ptr){
        *err = 0 == errno ? ENOMEM : errno;
        return NULL;
    }
    return ptr;
}

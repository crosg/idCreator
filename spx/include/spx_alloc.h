/*
 * */
#ifndef SPX_ALLOC_H
#define SPX_ALLOC_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#include "spx_types.h"


#if SYSBIT == 32
#define SPX_ALIGN_SIZE 4
#elif SYSBIT == 64
#define SPX_ALIGN_SIZE 8
#else
#define SPX_ALIGN_SIZE 8
#endif

#define SpxFree(ptr) do\
    {\
        if(NULL != ptr)\
        {\
            free(ptr);\
            ptr = NULL; \
        }\
    }while(false)

    void *spx_malloc(const size_t s,err_t *err);
    void *spx_alloc(const size_t numbs,const size_t s,err_t *err);
    void *spx_alloc_alone(const size_t s,err_t *err);
    void *spx_memalign_alloc(const size_t size,err_t *err);
    /* alloc the pointer array */
    void *spx_alloc_mptr(const size_t numbs,err_t *err);
    void *spx_realloc(void *p,const size_t size,err_t *err);

    spx_private spx_inline err_t spx_free(void **v){
        SpxFree(*v);
        return 0;
    }


#ifdef __cplusplus
}
#endif
#endif


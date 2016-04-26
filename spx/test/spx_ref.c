/*
 * =====================================================================================
 *
 *       Filename:  spx_ref.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2014/06/25 15时32分44秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#include <stdlib.h>


#include "spx_types.h"
#include "spx_alloc.h"
#include "spx_ref.h"
#include "spx_string.h"

void *spx_ref_new(void *v,err_t *err){
    if (NULL == v) {
        *err = EINVAL;
        return NULL;
    }
    struct spx_ref *ref = spx_alloc_alone(sizeof(*ref),err);
    if(NULL == ref){
        return NULL;
    }
    ref->p = v;
    ref->ref ++;
    return ref->p;
}

string_t spx_ref_string_new(string_t s,err_t *err){
    if(NULL == s){
        *err = EINVAL;
        return NULL;
    }
    struct spx_ref *ref = spx_alloc_alone(sizeof(*ref),err);
    if(NULL == ref){
        return NULL;
    }
    struct sds *sh = (void*)(s-sizeof *sh);
    ref->p = sh;
    ref->ref ++;
    return ((char *) ref->p) + sizeof(*sh);
}

void *spx_ref(void *v){
    if(NULL != v){
        struct spx_ref *ref =(struct spx_ref *) ((char *) v) - sizeof(u32_t);
        ref->ref ++;
        return ref->p;
    }
    return NULL;
}

string_t spx_ref_string(string_t s){
    if(NULL != s) {
        struct sds *sh = (void*)(s-sizeof *sh);
        struct spx_ref *ref =(struct spx_ref *) ((char *) sh) - sizeof(u32_t);
        ref->ref ++;
        return ((char *) ref->p) + sizeof(*sh);
    }
    return NULL;
}

bool_t spx_ref_free(void *v){
    if(NULL !=v){
        struct spx_ref *ref =(struct spx_ref *) ((char *) v) - sizeof(u32_t);
        ref->ref --;
        if(0 == ref){
            SpxFree(ref);
            return true;
        }
    }
    return false;
}

bool_t spx_ref_string_free(string_t s){
    if(NULL != s) {
        struct sds *sh = (void*)(s-sizeof *sh);
        struct spx_ref *ref =(struct spx_ref *) ((char *) sh) - sizeof(u32_t);
        ref->ref --;
        if(0 == ref){
            SpxFree(ref);
            return true;
        }
    }
    return false;
}



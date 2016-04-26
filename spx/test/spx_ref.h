/*
 * =====================================================================================
 *
 *       Filename:  spx_ref.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2014/06/25 15时32分50秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#ifndef _SPX_REF_H_
#define _SPX_REF_H_
#ifdef __cplusplus
extern "C" {
#endif

    /*
     * if you use the ref count please make sure you konw the memory status
     * aways.
     * and if you dont care of the memory a moment,it is very dangerous.
     * !!!!!!!MAKE SURE THE MEMORY
     *
     * if the  of pointer is the struct by pointer-compute,
     * then it can use the ref-count directly. and you must add the
     * wrapper function,just as string_t and struct sds
     */

#include "spx_types.h"

    struct spx_ref{
        u32_t ref;
        void *p;
    };

    void *spx_ref_new(void *v,err_t *err);
    void *spx_ref(void *v);
    bool_t spx_ref_free(void *p);
    string_t spx_ref_string_new(string_t s,err_t *err);
    string_t spx_ref_string(string_t s);
    bool_t spx_ref_string_free(string_t s);

#ifdef __cplusplus
}
#endif
#endif

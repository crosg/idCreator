/*
 * =====================================================================================
 *
 *       Filename:  spx_collection.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2014/06/09 10时25分48秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#ifndef _SPX_COLLECTION_H_
#define _SPX_COLLECTION_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <limits.h>

#include "spx_types.h"
#include "spx_string.h"

    typedef u64_t SpxCollectionKeyHashDelegate(void *k,size_t kl);
    typedef int SpxCollectionCmperDelegate(\
            void *k1,u32_t l1,void *k2,u32_t l2);
    typedef err_t SpxCollectionKeyFreeDelegate(void **k);
    typedef err_t SpxCollectionValueFreeDelegate(void **v);
    typedef string_t SpxCollectionKeyPrintfDelegate(\
            void *k,err_t *err);

spx_private spx_inline int spx_collection_i32_default_cmper(\
        void *k1,u32_t l1,void *k2,u32_t l2){
    if(NULL == k1 || NULL == k2) errno = EINVAL;
    i32_t *i1 = 0,*i2 = 0;
    i1 = (i32_t *) k1;
    i2 = (i32_t *) k2;
    if(*i1 < *i2) return -1;
    else if(*i1 > *i2) return 1;
    else return 0;
}
spx_private spx_inline int spx_collection_i64_default_cmper(\
        void *k1,u32_t l1,void *k2,u32_t l2){
    if(NULL == k1 || NULL == k2) errno = EINVAL;
    i64_t *i1 = 0,*i2 = 0;
    i1 = (i64_t *) k1;
    i2 = (i64_t *) k2;
    if(*i1 < *i2) return -1;
    else if(*i1 > *i2) return 1;
    else return 0;
}
spx_private spx_inline int spx_collection_u32_default_cmper(\
        void *k1,u32_t l1,void *k2,u32_t l2){
    if(NULL == k1 || NULL == k2) errno = EINVAL;
    u32_t *i1 = 0,*i2 = 0;
    i1 = (u32_t *) k1;
    i2 = (u32_t *) k2;
    if(*i1 < *i2) return -1;
    else if(*i1 > *i2) return 1;
    else return 0;
}
spx_private spx_inline int spx_collection_u64_default_cmper(\
        void *k1,u32_t l1,void *k2,u32_t l2){
    if(NULL == k1 || NULL == k2) errno = EINVAL;
    u64_t *i1 = 0,*i2 = 0;
    i1 = (u64_t *) k1;
    i2 = (u64_t *) k2;
    if(*i1 < *i2) return -1;
    else if(*i1 > *i2) return 1;
    else return 0;
}
spx_private spx_inline int spx_collection_string_default_cmper(\
        void *k1,u32_t l1,void *k2,u32_t l2){
    if(NULL == k1 || NULL == k2) errno = EINVAL;
    string_t s1 = (string_t) k1;
    string_t s2 = (string_t) k2;
    if(SpxStringIsEmpty(s1) \
            && !SpxStringIsNullOrEmpty(s2)) return -1;
    if(!SpxStringIsEmpty(s1) \
            && SpxStringIsEmpty(s2)) return 1;
    if(SpxStringIsEmpty(s1) \
            && SpxStringIsEmpty(s2)) return 0;
    return spx_string_cmp(s1,s2);
}
spx_private spx_inline int spx_collection_time_default_cmper(\
        void *k1,u32_t l1,void *k2,u32_t l2){
    if(NULL == k1 || NULL == k2) errno = EINVAL;
    time_t *i1 = 0,*i2 = 0;
    i1 = (time_t *) k1;
    i2 = (time_t *) k2;
    if(*i1 < *i2) return -1;
    else if(*i1 > *i2) return 1;
    else return 0;
}


spx_private spx_inline string_t spx_collection_i32_default_printf(\
        void *k,err_t *err){
    i32_t *n = (i32_t *)k;
    return spx_string_from_i64(*n,err);
}
spx_private spx_inline string_t spx_collection_u32_default_printf(\
        void *k,err_t *err){
    u32_t *n = (u32_t *)k;
    return spx_string_from_i64(*n,err);
}
spx_private spx_inline string_t spx_collection_i64_default_printf(\
        void *k,err_t *err){
    i64_t *n = (i64_t *)k;
    return spx_string_from_i64(*n,err);
}
spx_private spx_inline string_t spx_collection_u64_default_printf(\
        void *k,err_t *err){
    u64_t *n = (u64_t *)k;
    return spx_string_from_i64(*n,err);
}
spx_private spx_inline string_t spx_collection_time_default_printf(\
        void *k,err_t *err){
    u64_t *n = (u64_t *)k;
    return spx_string_from_i64(*n,err);
}

#define BIT_IN_INT     ( sizeof(int) * CHAR_BIT )
#define THREE_QUARTERS	((int) ((BIT_IN_INT * 3) / 4))
#define ONE_EIGHTH		((int) (BIT_IN_INT / 8))
#define HIGH_BITS		( ~((unsigned int)(~0) >> ONE_EIGHTH ))

spx_private spx_inline u64_t spx_pjw(void *buff,size_t len) {
	char *data = (char *) buff;
	u32_t val, i;
	for (val = 0; *data; ++data) {
		val = (val << ONE_EIGHTH ) + *data;
		if ((i = val & HIGH_BITS)!= 0 )val
			= (val ^ (i >> THREE_QUARTERS )) & ~HIGH_BITS;
	}
	return val;
}

spx_private spx_inline u64_t spx_elf(void *data,size_t len) {
    unsigned char *buff = (unsigned char *)data;
	u64_t h = 0, g;
	while (*buff) {
		h = (h << 4) + *buff++;
		if (0 != (g = (h & 0xF0000000)))
			h ^= g >> 24;
		h &= ~g;
	}
	return h;
}


#ifdef __cplusplus
}
#endif
#endif

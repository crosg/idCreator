/*************************************************************
 *                     _ooOoo_
 *                    o8888888o
 *                    88" . "88
 *                    (| -_- |)
 *                    O\  =  /O
 *                 ____/`---'\____
 *               .'  \\|     |//  `.
 *              /  \\|||  :  |||//  \
 *             /  _||||| -:- |||||-  \
 *             |   | \\\  -  /// |   |
 *             | \_|  ''\---/''  |   |
 *             \  .-\__  `-`  ___/-. /
 *           ___`. .'  /--.--\  `. . __
 *        ."" '<  `.___\_<|>_/___.'  >'"".
 *       | | :  `- \`.;`\ _ /`;.`/ - ` : | |
 *       \  \ `-.   \_ __\ /__ _/   .-` /  /
 *  ======`-.____`-.___\_____/___.-`____.-'======
 *                     `=---='
 *  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *           佛祖保佑       永无BUG
 *
 * ==========================================================================
 *
 * this software or lib may be copied only under the terms of the gnu general
 * public license v3, which may be found in the source kit.
 *
 *       Filename:  spx_object.c
 *        Created:  2014/10/11 13时15分17秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 ***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "spx_types.h"
#include "spx_defs.h"
#include "spx_alloc.h"
#include "spx_atomic.h"
#include "spx_object.h"


void *spx_object_new(const size_t s,err_t *err){
    if(0 == s){
        *err = EINVAL;
        return NULL;
    }
    size_t realsize = s + SpxObjectAlignSize;
    struct spx_object *o = (struct spx_object *)
        malloc(realsize);
    if(NULL == o){
        *err = 0 == errno ? ENOMEM : errno;
    }
    o->spx_object_refs = 1;
    o->spx_object_is_pooling = false;
    o->spx_object_size = s;
    return SpxMemIncr(o,SpxObjectAlignSize);
}

void *spx_object_news(const size_t numbs,const size_t s,err_t *err){
    if(0 == s || 0 == numbs){
        *err = EINVAL;
        return NULL;
    }
    size_t realsize = SpxObjectAlignSize + numbs * s;
    struct spx_object *o = calloc(realsize,sizeof(char));
    if(NULL == o){
        *err = 0 == errno ? ENOMEM : errno;
        return NULL;
    }
    o->spx_object_refs = 1;
    o->spx_object_is_pooling = false;
    o->spx_object_size = s * numbs;
    return SpxMemIncr(o,SpxObjectAlignSize);
}

void *spx_object_new_alone(const size_t s,err_t *err){
    if(0 == s){
        *err = EINVAL;
        return NULL;
    }
    return spx_alloc(1,s,err);
}

void *spx_object_new_algin(const size_t s,err_t *err) {
    struct spx_object *o = NULL;
    size_t realsize = SpxObjectAlignSize + SpxAlign(s,SpxAlignSize);
    if(0 != (*err = posix_memalign((void **) &o, SpxAlignSize, realsize))){
        return NULL;
    }
    o->spx_object_refs = 1;
    o->spx_object_is_pooling = false;
    o->spx_object_size = s;
    return SpxMemIncr(o,SpxObjectAlignSize);
}


void *spx_object_renew(void *p,const size_t s,err_t *err){
    struct spx_object *o = (struct spx_object *) ((char *) p - SpxObjectAlignSize);

    size_t realsize = SpxObjectAlignSize + s;
    struct spx_object *ptr = realloc(o,realsize);
    if(NULL == ptr){
        *err = 0 == errno ? ENOMEM : errno;
        return NULL;
    }
    return SpxMemIncr(ptr,SpxObjectAlignSize);
}

bool_t spx_object_free(void *p){
    struct spx_object *o = (struct spx_object *) SpxMemDecr(p,SpxObjectAlignSize);
    if(!o->spx_object_is_pooling){
        if(0 == SpxAtomicDecr(&(o->spx_object_refs))){
            SpxFree(o);
            return true;
        }
    }
    return false;
}

bool_t spx_object_free_force(void *p){
    struct spx_object *o = (struct spx_object *) SpxMemDecr(p,SpxObjectAlignSize);
    if(!o->spx_object_is_pooling){
        SpxFree(o);
        return true;
    }
    return false;
}

void *spx_object_ref(void *p){
    if(NULL == p){
        return NULL;
    }
    struct spx_object *o = (struct spx_object *) ((char *) p - SpxObjectAlignSize);
    if(0 == o->spx_object_refs){
        return NULL;
    }
    SpxAtomicVIncr(o->spx_object_refs);
    return p;
}

void *spx_object_unref(void *p){
    if(NULL == p){
        return NULL;
    }
    struct spx_object *o = (struct spx_object *) ((char *) p - SpxObjectAlignSize);
    if(0 == o->spx_object_refs){
        return NULL;
    }
    if(0 == SpxAtomicVDecr(o->spx_object_refs)){
        if(!o->spx_object_is_pooling){
            SpxFree(o);
            return NULL;
        }
    }
    return p;
}

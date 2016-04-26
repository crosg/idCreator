/*
 * =====================================================================================
 *
 *       Filename:  skiplist.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2014/05/19 11时23分26秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "spx_skiplist.h"
#include "spx_defs.h"
#include "spx_errno.h"
#include "spx_alloc.h"
#include "spx_types.h"
#include "spx_vector.h"
#include "spx_string.h"

/*
#define SkipListNodeSize(s) \
    (sizeof(struct spx_skiplist_n) \
     + sizeof(struct spx_skiplist_n *) * (s))

spx_private int get_skiplist_level(u32_t max_level);

spx_private int get_skiplist_level(u32_t max_level){
    u32_t seedVal = 0;
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv , &tz);
    seedVal=((((unsigned int)tv.tv_sec&0xFFFF)+
                (unsigned int)tv.tv_usec)^
            (unsigned int)tv.tv_usec);
    srand((unsigned int)seedVal +  rand());
    return rand() % max_level + 1;
}

struct spx_skiplist *spx_skiplist_new(SpxLogDelegate *log,\
            int type,u32_t maxlevel,\
            bool_t allow_conflict,
            SpxCollectionCmperDelegate cmper,\
            SpxSkipListUniqueInspectorDelegate *inspector,\
            SpxCollectionKeyPrintfDelegate *kprintf,\
            SpxCollectionKeyFreeDelegate *kfree,\
            SpxCollectionValueFreeDelegate *vfree,\
        err_t *err){
    if(0 > type || type == SPX_SKIPLIST_IDX_OBJECT){
        SpxLogFmt1(log,SpxLogDebug,\
                "the argument is fail.type is %d,but compers count is %d.",\
                type,SPX_SKIPLIST_IDX_OBJECT);
        *err = EINVAL;
        return NULL;
    }

    u32_t level = 0 == maxlevel ? SPX_SKIPLIST_LEVEL_DEFAULT : maxlevel;
    struct spx_skiplist *spl = NULL;
    spl = spx_alloc_alone(sizeof(*spl),err);
    if(NULL == spl) {
        SpxLog2(log,SpxLogError,*err,"alloc skiplist is fail.");
        goto r1;
    }
    spl->header = spx_alloc_alone(SkipListNodeSize(level),err);
    if(NULL == spl->header) {
        SpxLog2(log,SpxLogError,*err,"alloc skiplist header is fail.");
        goto r1;
    }
    spl->level = 0;
    spl->cmp = cmper;
    spl->inspector = inspector;
    spl->allow_conflict = allow_conflict;
    spl->maxlevel =level;
    spl->log = log;
    spl->type =type;
    spl->vfree = vfree;
    spl->kfree = kfree;
    spl->kprintf = kprintf;
    SpxLogFmt1(log,SpxLogDebug,\
            "create skiplist.max level:%d,idx:%d allow_conflict:%d,",\
            level,type,allow_conflict);
    return spl;
r1:
    if(NULL != spl->header){
        SpxFree(spl->header);
    }
    if(NULL != spl){
        SpxFree(spl);
    }
    return NULL;
}

//Spaghetti algorithm
//if you modify the code ,please make sure you understand this code.
err_t spx_skiplist_insert(struct spx_skiplist *spl,\
        void *k,u32_t kl,void *v,u64_t vl,
        int level){
    if(NULL == spl){
        return EINVAL;
    }
    if(NULL == k || NULL == v){
        SpxLog1(spl->log,SpxLogDebug, "the argument is fail.");
        return EINVAL;
    }

    int err = 0;
    struct spx_skiplist_n **up = NULL;
    struct spx_skiplist_v *value = NULL;
    struct spx_skiplist_n *node = NULL;

    up = spx_alloc_mptr(spl->maxlevel,&err);
    if(NULL == up){
        SpxLog2(spl->log,SpxLogError,err,"alloc skiplist node is fail.");
        return err;
    }
    //if you understand the code behand this line very hard,please donot modify it
    //警告：如果你理解以下的语句比较吃力，不要修改它。

    struct spx_skiplist_n *p = spl->header;
    struct spx_skiplist_n *q = NULL;
    i32_t l = 0;
    int r = 0;
    for(l = spl->level - 1; l >= 0; l--){
        while(NULL != (q = p->next[l])){
            r = spl->cmp(k,kl,q->k,q->kl);
            if( 0 < r){
                up[l] = q;
                p = q;
                continue;
            }
            break;
        }
        up[l] = p;
    }

    if(0 == r && 0 != spl->key_count) {
        //if have same key,the r must == 0 except first insert
        if(!spl->allow_conflict){
            if(NULL == spl->kprintf){
                SpxLogFmt1(spl->log,SpxLogError,\
                        "the key:%s is exist and skiplist is not allow conflict.",
                        SpxString2Char2(k));
            } else {
                string_t sk = spl->kprintf(k,&err);
                if(NULL != sk) {
                    SpxLogFmt1(spl->log,SpxLogError,\
                            "the key:%s is exist and skiplist is not allow conflict.",
                            sk);
                }
                spx_string_free(sk);
            }
            err = EEXIST;
            goto r2;
        }
        //add the same key,so not change the skiplist structure
        value = spx_alloc_alone(sizeof(*value),&err);
        if(NULL == value){
            SpxLog2(spl->log,SpxLogError,err,\
                    "alloc skiplist value is fail.");
            goto r2;
        }
        value->s = vl;
        value->v = v;
        if(0 != (err = spx_vector_push(q->v.list,value))){
            SpxLog2(spl->log,SpxLogError,err,\
                    "push the value to the vector of skiplist node is fail.")
                goto r3;
        }
        spl->val_count ++;
        goto r2;
    }

    //是不是准备要放弃看这个算法了？这才刚刚开始
    //is not giving up understanding this function? this is begin,just so so.
    l = 0 > level ? get_skiplist_level(spl->maxlevel) : level;
    int i = 0;
    if(l >(i32_t) spl->level){
        for(i = l -1; i >= (int) spl->level; i--){
            up[i] = spl->header;
        }
    }
    node = spx_alloc_alone(SkipListNodeSize(l),&err);
    if(NULL == node) {
        SpxLog2(spl->log,SpxLogError,err,\
                "alloc node for skiplist is fail.");
        goto r3;
    }
    node->k = k;
    node->kl = kl;
    node->level = l;
    //add the same key,so not change the skiplist structure
    value = spx_alloc_alone(sizeof(*value),&err);
    if(NULL == value) {
        SpxLog2(spl->log,SpxLogError,err,\
                "alloc skiplist value is fail.");
        goto r3;
    }
    value->s = vl;
    value->v = v;

    if(spl->allow_conflict){
        node->v.list = spx_vector_init(spl->log,spl->vfree,&err);
        if(NULL == node->v.list) {
            SpxLog2(spl->log,SpxLogError,err,\
                    "alloc vector for skiplist node is fail.");
            goto r3;
        }
        if(0 != (err = spx_vector_push(node->v.list,value))){
            SpxLog2(spl->log,SpxLogError,err,\
                    "push value to vector is fail.");
            goto r3;
        }
    }else {
        node->v.obj = value;
    }
    for(i = l - 1; i >= 0; i--){
        node->next[i] = up[i]->next[i];
        up[i]->next[i] =node;
    }
    if(l > (int) spl->level){
        spl->level = l;
    }
    spl->key_count ++;
    spl->val_count ++;
    goto r2;
r3:
    if(NULL != value){
        SpxFree(value);
    }
    if(NULL != node){
        SpxFree(node);
    }
r2:
    if(NULL != up){
        SpxFree(up);
    }
    return err;
}

err_t spx_skiplist_delete(struct spx_skiplist *spl,\
        void *k,u32_t kl){
    if(NULL == spl){
        return EINVAL;
    }
    if(NULL == k){
        SpxLog1(spl->log,SpxLogDebug,\
                "argument is fail.");
        return EINVAL;
    }
    if(0 == spl->key_count) return 0;
    int err = 0;
    struct spx_skiplist_n **up = NULL;

    up = spx_alloc_mptr(spl->maxlevel,&err);
    if(0 != err){
        SpxLog2(spl->log,SpxLogError,err,"alloc skiplist node is fail.");
        return err;
    }
    //if you understand the code behand this line very hard,please donot modify it
    //警告：如果你理解以下的语句比较吃力，不要修改它。

    struct spx_skiplist_n *p = spl->header;
    struct spx_skiplist_n *q = NULL;
    int l = 0;
    int i = 0;
    int r = 0;
    for(l = spl->level - 1; l >= 0; l--){
        while(NULL != (q = p->next[l])){
            r = spl->cmp(k,kl,q->k,q->kl);
            if( 0 < r){
                up[l] = q;
                p = q;
                continue;
            }
            break;
        }
        up[l] = p;
    }

    if(0 != r && 0 != spl->key_count){
        if(NULL == spl->kprintf){
            SpxLogFmt1(spl->log,SpxLogWarn,\
                    "the key:%s is not exist.",
                    SpxString2Char2(k));
        } else {
            string_t sk = spl->kprintf(k,&err);
            if(NULL != sk){
                SpxLogFmt1(spl->log,SpxLogWarn,\
                        "the key:%s is not exist.",
                        sk);
            }
            spx_string_free(sk);
        }
        goto r1;
    }
    if(spl->allow_conflict){
        struct spx_vector *vector = q->v.list;
        if(NULL == vector){
            if(NULL == spl->kprintf){
                SpxLogFmt1(spl->log,SpxLogWarn,\
                        "the value of vector for key %s is exist and value is null.",\
                        SpxString2Char2(k));
            } else {
                string_t sk = spl->kprintf(k,&err);
                if(NULL != sk) {
                    SpxLogFmt1(spl->log,SpxLogWarn,\
                            "the value of vector for key %s is exist and value is null.",\
                            sk);
                }
                spx_string_free(sk);
            }
            goto r1;
        }
        size_t numbs = vector->size;
        if(0 != (err = spx_vector_free(&vector))){
            if(NULL == spl->kprintf){
                SpxLogFmt1(spl->log,SpxLogError,\
                        "delete the key %s from skiplist is fail.",\
                        SpxString2Char2(k));
            } else {
                string_t sk = spl->kprintf(k,&err);
                if(NULL != sk){
                    SpxLogFmt1(spl->log,SpxLogError,\
                            "delete the key %s from skiplist is fail.",\
                            sk);
                }
                spx_string_free(sk);
            }
            goto r1;
        }

        for(i = q->level - 1; i >= 0; i--){
            up[i]->next[i] = q->next[i];
        }
        if(NULL != spl->kfree) {
            spl->kfree(&(q->k));
        }
        SpxFree(q);
        spl->val_count -= numbs;
        spl->key_count --;
    }else{
        for(i = q->level - 1; i >= 0; i--){
            up[i]->next[i] = q->next[i];
        }

        if(NULL != spl->vfree){
            spl->vfree((void **) &(q->v.obj));
        }
        if(NULL != spl->kfree){
            spl->kfree(&(q->k));
        }
        SpxFree(q);
        spl->val_count --;
        spl->key_count --;
    }
r1:
    if(NULL != up){
        SpxFree(up);
    }
    return err;
}

err_t spx_skiplist_out(struct spx_skiplist *spl,\
        void *k,u32_t kl,void **v,u64_t *vl,
        SpxSkipListRangeCmperDelegate *searcher){
    if(NULL == spl){
        return EINVAL;
    }
    if(NULL == k){
        SpxLog1(spl->log,SpxLogDebug,\
                "argument is fail.");
        return EINVAL;
    }
    if(0 == spl->key_count) return 0;
    int err = 0;
    struct spx_skiplist_n **up = NULL;
    up = spx_alloc_mptr(spl->maxlevel,&err);
    if(0 != err){
        SpxLog2(spl->log,SpxLogError,err,"alloc skiplist node is fail.");
        return err;
    }

    //if you understand the code behand this line very hard,please donot modify it
    //警告：如果你理解以下的语句比较吃力，不要修改它。

    struct spx_skiplist_n *p = spl->header;
    struct spx_skiplist_n *q = NULL;
    int l = 0;
    int r = 0;
    for(l = spl->level - 1; l >= 0; l--){
        while(NULL != (q = p->next[l])){
            if(NULL == searcher) {
                r = spl->cmp(k,kl,q->k,q->kl);
            } else {
                r = searcher(k,kl,q->k,q->kl);
            }
            if( 0 < r){
                up[l] = q;
                p = q;
                continue;
            }
            break;
        }
        up[l] = p;
    }

    if(0 != r && 0 != spl->key_count){
        if(NULL == spl->kprintf){
            SpxLogFmt1(spl->log,SpxLogError,\
                    "not found the key:%s in the skiplist.",\
                    SpxString2Char2(k));
        } else {
            string_t sk = spl->kprintf(k,&err);
            if(NULL != sk) {
                SpxLogFmt1(spl->log,SpxLogError,\
                        "not found the key:%s in the skiplist.",\
                        sk);
            }
            spx_string_free(sk);
        }
        goto r1;
    }

    int i = 0;
    struct spx_skiplist_v *nv = NULL;
    if(spl->allow_conflict){
        struct spx_vector *vector = q->v.list;
        if(0 < vector->size){
            nv = spx_vector_pop(vector,&err);
            if(NULL == nv) {
                if(NULL == spl->kprintf){
                    SpxLogFmt1(spl->log,SpxLogError,\
                            "pop the key:%s form node of skiplist is fai.",\
                            SpxString2Char2(k));
                } else {
                    string_t sk = spl->kprintf(k,&err);
                    if(NULL != sk) {
                        SpxLogFmt1(spl->log,SpxLogError,\
                                "pop the key:%s form node of skiplist is fai.",\
                                sk);
                    }
                    spx_string_free(sk);
                }
                goto r1;
            }
            *v = nv->v;
            *vl = nv->s;
        }
        if(0 == vector->size){
            if(0 != (err = spx_vector_free(&vector))){

                if(NULL == spl->kprintf){
                    SpxLogFmt1(spl->log,SpxLogError,\
                            "remove the key:%s from skiplist is fail.",\
                            SpxString2Char2(k));
                } else {
                    string_t sk = spl->kprintf(k,&err);
                    if(NULL != sk) {
                    SpxLogFmt1(spl->log,SpxLogError,\
                            "remove the key:%s from skiplist is fail.",\
                            sk);
                    }
                    spx_string_free(sk);
                }
                goto r1;
            }
            for(i = q->level - 1; i >= 0; i--){
                up[i]->next[i] = q->next[i];
            }
            if(NULL != spl->kfree) {
                spl->kfree(&(q->k));
            }
            SpxFree(q);
            spl->key_count --;
        }
        spl->val_count --;
        goto r1;
    }else{
        for(i = q->level - 1; i >= 0; i--){
            up[i]->next[i] = q->next[i];
        }
        *v = q->v.obj->v;
        *vl = q->v.obj->s;
        if(NULL != spl->kfree) {
            spl->kfree(&(q->k));
        }
        SpxFree(q);
        spl->key_count --;
        spl->val_count --;
    }
r1:
    if(NULL != up){
        SpxFree(up);
    }
    return err;
}

   err_t spx_skiplist_search(struct spx_skiplist *spl,\
   void *min,u32_t l1,void *max,u32_t l2,
   struct spx_vector **rc);

   err_t spx_skiplist_find(struct spx_skiplist *spl,\
   void *k,u32_t l,void **rc);
void spx_skiplist_free(struct spx_skiplist **spl){
    if(NULL == *spl){
        return;
    }
    struct spx_skiplist_n *node = (*spl)->header;
    struct spx_skiplist_n *q = NULL;
    if((*spl)->allow_conflict) {
        q = node->next[0];
        while(NULL != q){
            struct spx_vector *list =  q->v.list;
            spx_vector_free(&list);
            node = q->next[0];
            (*spl)->kfree(&(q->k));
            SpxFree(q);
        }
    } else {
        q = node->next[0];
        while(NULL != q){
            (*spl)->vfree((void **) &(q->v.obj));
            node = q->next[0];
            (*spl)->kfree(&(q->k));
            SpxFree(q);
        }
    }
    SpxFree((*spl)->header);
    SpxFree(*spl);
    return;
}

*/


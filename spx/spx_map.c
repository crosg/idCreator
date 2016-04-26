/*
 * =====================================================================================
 *
 *       Filename:  spx_map.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2014/06/06 10时45分54秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#include <stdlib.h>

#include "spx_errno.h"
#include "spx_map.h"
#include "spx_alloc.h"
#include "spx_defs.h"

#define SpxMapSlotSize 64
#define SpxMapFactor 1.25
#define SpxMapNodeSize 128

spx_private err_t spx_map_expand(struct spx_map *map);

spx_private err_t spx_map_expand(struct spx_map *map){
    err_t err = 0;
    size_t slots_count = (size_t) map->slots_count * SpxMapFactor;
    struct spx_map_slot *newslot = spx_alloc_alone(\
            sizeof(*newslot) * slots_count,&err);
    if(NULL == newslot){
        return err;
    }

    struct spx_map_key_node *knode = map->keys_header;
    while(NULL != knode){
        struct spx_map_node *n = knode->node;
        size_t idx = n->hash % slots_count;
        struct spx_map_slot *s = newslot + idx;
        if(NULL == s->header){
            s->header = n;
            s->tail = n;
        } else {
            n->prev = s->tail;
            s->tail->next = n;
            s->tail = n;
        }
        s->real_size ++;
        knode = knode->next;
    }

    SpxFree(map->slots);
    map->slots = newslot;
    map->slots_count = slots_count;
    return err;
}
struct spx_map *spx_map_new(SpxLogDelegate *log,\
            SpxCollectionKeyHashDelegate *hash,\
            SpxCollectionCmperDelegate *cmper,\
        SpxCollectionKeyPrintfDelegate *kprintf,\
        SpxCollectionKeyFreeDelegate *kfree,\
        SpxCollectionValueFreeDelegate *vfree,\
        err_t *err){
    struct spx_map *map = spx_alloc_alone(sizeof(*map),err);
    if(NULL == map){
        SpxLog2(log,SpxLogError,*err,"alloc map is fail.");
        return NULL;
    }

    map->slots = spx_alloc_alone(sizeof(*(map->slots)) * SpxMapSlotSize,err);
    if(NULL == map->slots){
        goto r1;
    }
    map->slots_count = SpxMapSlotSize;
    map->hash = hash;
    map->vfree = vfree;
    map->kprintf = kprintf;
    map->kfree = kfree;
    map->cmper = cmper;
    map->log = log;
    return map;
r1:
    SpxFree(map->slots);
    SpxFree(map);
    return NULL;
}

err_t spx_map_insert(struct spx_map *map,\
        void *k,size_t kl,void *v,size_t vl){
    err_t err = 0;
    u64_t hash = map->hash(k,kl);
    struct spx_map_node *n = NULL;
    while(true){
        size_t idx = hash % map->slots_count;
        struct spx_map_slot *slot = map->slots + idx;
        if(slot->real_size + 1 > SpxMapNodeSize){
            err = spx_map_expand(map);
            if(0 != err){
                goto r1;
            }
            continue;
        }
        n = spx_alloc_alone(sizeof(*n),&err);
        if(NULL == n){
            break;
        }
        struct spx_map_key_node *kn = spx_alloc_alone(sizeof(*kn),&err);
        if(NULL == kn){
            goto r1;
        }
        n->p = kn;
        n->k = k;
        n->kl = kl;
        n->v = v;
        n->vl = vl;
        n->hash = hash;
        kn->node = n;

        if(NULL == slot->header){
            slot->header = n;
            slot->tail = n;
        } else {
            n->prev = slot->tail;
            slot->tail->next = n;
            slot->tail = n;
        }

        if(NULL == map->keys_header){
            map->keys_header = kn;
            map->keys_tail = kn;
        }else{
            kn->prev = map->keys_tail;
            map->keys_tail->next = kn;
            map->keys_tail = kn;
        }
        slot->real_size ++;
        map->numbs ++;
        break;
    }
    return err;
r1:
    SpxFree(n);
    return err;
}

void *spx_map_get(struct spx_map *map,\
        void *k,size_t kl,size_t *vl){
    size_t hash = map->hash(k,kl);
    size_t idx = hash % map->slots_count;
    struct spx_map_slot *slot = map->slots + idx;
    struct spx_map_node *n = slot->header;
    void *v = NULL;
    while(NULL != n){
       if(0 == map->cmper(k,kl,n->k,n->kl)){
            v = n->v;
            if(NULL != vl) {
            *vl = n->vl;
            }
            break;
       }
       n = n->next;
    }
    return v;
}

bool_t spx_map_exist_key(struct spx_map *map,\
        void *k,size_t kl){
    size_t hash = map->hash(k,kl);
    size_t idx = hash % map->slots_count;
    struct spx_map_slot *slot = map->slots + idx;
    struct spx_map_node *n = slot->header;
    while(NULL != n){
       if(0 == map->cmper(k,kl,n->k,n->kl)){
            return true;
       }
       n = n->next;
    }
    return false;
}

void *spx_map_out(struct spx_map *map,\
        void *k,size_t kl,size_t *vl){
    size_t hash = map->hash(k,kl);
    size_t idx = hash % map->slots_count;
    struct spx_map_slot *slot = map->slots + idx;
    struct spx_map_node *n = slot->header;
    void *v = NULL;
    while(NULL != n){
        if(0 == map->cmper(k,kl,n->k,n->kl)){
            v = n->v;
            if(NULL != vl){
            *vl = n->vl;
            }
            struct spx_map_key_node *kn = n->p;
            if(NULL != kn->prev){
                kn->prev->next = kn->next;
            }else{
                map->keys_header = kn->next;
            }
            if(NULL != kn->next){
                kn->next->prev = kn->prev;
            }else {
                map->keys_tail = kn->prev;
            }
            n->prev->next = n->next;
            n->next->prev = n->prev;
            if(NULL != map->kfree){
                map->kfree(n->k);
            }
            SpxFree(n);
            break;
        }
        n = n->next;
    }
    return v;
}

err_t spx_map_delete(struct spx_map *map,\
        void *k,size_t kl){
    err_t err = 0;
    size_t hash = map->hash(k,kl);
    size_t idx = hash % map->slots_count;
    struct spx_map_slot *slot = map->slots + idx;
    struct spx_map_node *n = slot->header;
    while(NULL != n){
        if(0 == map->cmper(k,kl,n->k,n->kl)){
            struct spx_map_key_node *kn = n->p;
            if(NULL != kn->prev){
                kn->prev->next = kn->next;
            }else{
                map->keys_header = kn->next;
            }
            if(NULL != kn->next){
                kn->next->prev = kn->prev;
            }else {
                map->keys_tail = kn->prev;
            }
            n->prev->next = n->next;
            n->next->prev = n->prev;
            if(NULL != map->kfree){
                map->kfree(n->k);
            }
            SpxFree(n);
            break;
        }
        n = n->next;
    }
    return err;
}

err_t spx_map_free(struct spx_map **map){
    err_t err = 0;
    size_t i = 0;
    for(i = 0; i< (*map)->slots_count; i++){
        struct spx_map_slot *slot = (*map)->slots + i;
        struct spx_map_node *n = slot->header;
        while(NULL != n){
            struct spx_map_key_node *kn = n->p;
            if(NULL != kn->prev){
                kn->prev->next = kn->next;
            }else{
                (*map)->keys_header = kn->next;
            }
            if(NULL != kn->next){
                kn->next->prev = kn->prev;
            }else {
                (*map)->keys_tail = kn->prev;
            }
            n->prev->next = n->next;
            n->next->prev = n->prev;
            if(NULL != (*map)->kfree){
                (*map)->kfree(n->k);
            }
            if(NULL != (*map)->vfree){
                (*map)->vfree(n->v);
            }
            SpxFree(n);
            n = n->next;
        }
    }
    SpxFree(*map);
    return err;
}

struct spx_map_iter *spx_map_iter_new(struct spx_map *map,err_t *err){
    if(NULL == map){
        *err = EINVAL;
    }
    struct spx_map_iter *iter = spx_alloc_alone(sizeof(*iter),err);
    if(NULL == iter){
        return NULL;
    }
    iter->curr = NULL;
    iter->map = map;
    return iter;
}

struct spx_map_node *spx_map_iter_next(struct spx_map_iter *iter,err_t *err){
    if(NULL == iter){
        *err = EINVAL;
    }
    if(iter->curr == iter->map->keys_tail){
        return NULL;
    }
    struct spx_map_node *n= NULL;
    iter->curr = NULL == iter->curr \
                 ? iter->map->keys_header : iter->curr->next;
    n = iter->curr->node;
    return n;
}

void spx_map_iter_reset(struct spx_map_iter *iter){
    if(NULL == iter){
        return ;
    }
    iter->curr = NULL;
}

err_t spx_map_iter_free(struct spx_map_iter ** iter){
    if(NULL == *iter){
        return EINVAL;
    }
    SpxFree(*iter);
    return 0;
}


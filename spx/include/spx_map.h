/*
 * =====================================================================================
 *
 *       Filename:  spx_map.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2014/06/06 10时46分03秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#ifndef _SPX_MAP_H_
#define _SPX_MAP_H_
#ifdef __cplusplus
extern "C" {
#endif
#include "spx_types.h"
#include "spx_collection.h"

    struct spx_map_node;
    struct spx_map_key_node;

    struct spx_map_key_node{
        struct spx_map_key_node *prev;
        struct spx_map_key_node *next;
        struct spx_map_node  *node;
    };

    struct spx_map_node{
        u64_t hash;
        void *k;
        size_t kl;
        void *v;
        size_t vl;
        struct spx_map_key_node *p;
        struct spx_map_node *prev;
        struct spx_map_node *next;
    };

    struct spx_map_slot{
        struct spx_map_node *header;
        struct spx_map_node *tail;
        size_t real_size;
    };

    struct spx_map{
        size_t numbs;
        size_t slots_count;
        SpxLogDelegate *log;
        SpxCollectionCmperDelegate *cmper;
        SpxCollectionKeyPrintfDelegate *kprintf;
        SpxCollectionKeyFreeDelegate *kfree;
        SpxCollectionValueFreeDelegate *vfree;
        SpxCollectionKeyHashDelegate *hash;
        struct spx_map_slot *slots;
        struct spx_map_key_node *keys_header;
        struct spx_map_key_node *keys_tail;
    };

    struct spx_map_iter{
        struct spx_map *map;
        struct spx_map_key_node *curr;
    };

    struct spx_map *spx_map_new(SpxLogDelegate *log,\
            SpxCollectionKeyHashDelegate *hash,\
            SpxCollectionCmperDelegate *cmper,\
            SpxCollectionKeyPrintfDelegate *kprintf,\
            SpxCollectionKeyFreeDelegate *kfree,\
            SpxCollectionValueFreeDelegate *vfree,\
            err_t *err);

    err_t spx_map_insert(struct spx_map *map,\
            void *k,size_t kl,void *v,size_t vl);

    void *spx_map_get(struct spx_map *map,\
            void *k,size_t kl,size_t *vl);

    bool_t spx_map_exist_key(struct spx_map *map,\
            void *k,size_t kl);

    void *spx_map_out(struct spx_map *map,\
            void *k,size_t kl,size_t *vl);

    err_t spx_map_delete(struct spx_map *map,\
            void *k,size_t kl);

    err_t spx_map_free(struct spx_map **map);

    struct spx_map_iter *spx_map_iter_new(struct spx_map *map,err_t *err);
    struct spx_map_node *spx_map_iter_next(struct spx_map_iter *iter,err_t *err);
    err_t spx_map_iter_free(struct spx_map_iter ** iter);
    void spx_map_iter_reset(struct spx_map_iter *iter);

    spx_private spx_inline size_t spx_map_numbs(struct spx_map *map){
        return map->numbs;
    }

    spx_private spx_inline size_t spx_map_slots_count(struct spx_map *map){
        return map->slots_count;
    }

#ifdef __cplusplus
}
#endif
#endif

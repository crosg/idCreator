/*
 * =====================================================================================
 *
 *       Filename:  spx_map_test.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2014/06/09 10时18分27秒
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
#include "spx_log.h"
#include "spx_map.h"
#include "spx_collection.h"
#include "spx_errno.h"
#include "spx_string.h"

err_t spx_map_free(void **s){
    string_t sk = (string_t) *s;
    spx_string_free(sk);
    return 0;
}
int main(int argc,char **argv){
    err_t err = 0;
    SpxLogDelegate *log = spx_log;
    struct spx_map *map = spx_map_new(log,\
            spx_pjw,
            spx_collection_string_default_cmper,
            NULL,
            spx_map_free,
            spx_map_free,
            &err);
    if(NULL == map){
        return err;
    }

    string_t aaa = spx_string_new("aaa",&err);
    string_t bbb = spx_string_new("bbb",&err);
    string_t ccc = spx_string_new("ccc",&err);
    string_t ddd = spx_string_new("ddd",&err);
    err = spx_map_insert(map,aaa,spx_string_len(aaa),\
            aaa,spx_string_len(aaa));
    err = spx_map_insert(map,bbb,spx_string_len(bbb),\
            bbb,spx_string_len(bbb));
    err = spx_map_insert(map,ccc,spx_string_len(ccc),\
            ccc,spx_string_len(ccc));
    err = spx_map_insert(map,ddd,spx_string_len(ddd),\
            ddd,spx_string_len(ddd));

    string_t newaaa,newbbb,newccc,newddd;
    size_t newalen,newblen,newclen,newdlen;

    err = spx_map_get(map,aaa,spx_string_len(aaa),(void **) &newaaa,&newalen);
    err = spx_map_get(map,aaa,spx_string_len(bbb),(void **) &newbbb,&newblen);
    err = spx_map_get(map,aaa,spx_string_len(ccc),(void **) &newccc,&newclen);
    err = spx_map_get(map,aaa,spx_string_len(ddd),(void **) &newddd,&newdlen);

    err = spx_map_out(map,aaa,spx_string_len(aaa),(void **) &newaaa,&newalen);
    err = spx_map_out(map,aaa,spx_string_len(bbb),(void **) &newbbb,&newblen);

    err = spx_map_delete(map,ccc,spx_string_len(ccc));
    err = spx_map_delete(map,ddd,spx_string_len(ddd));

    err = spx_map_insert(map,aaa,spx_string_len(aaa),\
            aaa,spx_string_len(aaa));
    err = spx_map_insert(map,bbb,spx_string_len(bbb),\
            bbb,spx_string_len(bbb));
    err = spx_map_insert(map,ccc,spx_string_len(ccc),\
            ccc,spx_string_len(ccc));
    err = spx_map_insert(map,ddd,spx_string_len(ddd),\
            ddd,spx_string_len(ddd));

    struct spx_map_iter *iter = spx_map_iter_new(map,&err);
    struct spx_map_node *node = NULL;
    while(NULL != (node = spx_map_iter_next(iter,&err))){
        string_t ks = node->k;
        size_t kl = node->kl;
        string_t vs = node->v;
        size_t vl = node->vl;
        printf("key:%s,key size:%lld,value:%s,value size:%lld.",\
                ks,(long long)kl,vs,(long long)vl);
    }
    err = spx_map_iter_destory(&iter);

    err = spx_map_destory(&map);
    return 0;
}

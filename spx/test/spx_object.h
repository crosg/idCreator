/***********************************************************************
 *                              _ooOoo_
 *                             o8888888o
 *                             88" . "88
 *                             (| -_- |)
 *                              O\ = /O
 *                          ____/`---'\____
 *                        .   ' \\| |// `.
 *                         / \\||| : |||// \
 *                       / _||||| -:- |||||- \
 *                         | | \\\ - /// | |
 *                       | \_| ''\---/'' | |
 *                        \ .-\__ `-` ___/-. /
 *                     ___`. .' /--.--\ `. . __
 *                  ."" '< `.___\_<|>_/___.' >'"".
 *                 | | : `- \`.;`\ _ /`;.`/ - ` : | |
 *                   \ \ `-. \_ __\ /__ _/ .-` / /
 *           ======`-.____`-.___\_____/___.-`____.-'======
 *                              `=---='
 *           .............................................
 *                    佛祖镇楼                  BUG辟易
 *            佛曰:
 *                    写字楼里写字间，写字间里程序员；
 *                    程序人员写程序，又拿程序换酒钱。
 *                    酒醒只在网上坐，酒醉还来网下眠；
 *                    酒醉酒醒日复日，网上网下年复年。
 *                    但愿老死电脑间，不愿鞠躬老板前；
 *                    奔驰宝马贵者趣，公交自行程序员。
 *                    别人笑我忒疯癫，我笑自己命太贱；
 *                    不见满街漂亮妹，哪个归得程序员？
 * ==========================================================================
 *
 * this software or lib may be copied only under the terms of the gnu general
 * public license v3, which may be found in the source kit.
 *
 *       Filename:  spx_object.h
 *        Created:  2014/10/11 13时15分19秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 ****************************************************************************/
#ifndef _SPX_OBJECT_H_
#define _SPX_OBJECT_H_
#ifdef __cplusplus
extern "C" {
#endif


#include <stdlib.h>
#include <stdio.h>

#include "spx_types.h"
#include "spx_defs.h"

    void *spx_object_new(const size_t s,err_t *err);
    void *spx_object_news(const size_t numbs,const size_t s,err_t *err);
    void *spx_object_new_alone(const size_t s,err_t *err);
    void *spx_object_new_algin(const size_t s,err_t *err) ;
    void *spx_object_renew(void *p,const size_t s,err_t *err);
    bool_t spx_object_free(void *p);
    bool_t spx_object_free_force(void *p);
    void *spx_object_ref(void *p);
    void *spx_object_unref(void *p);

    spx_private u32_t spx_object_refcount(void *p){
        if(NULL == p){
            return 0;
        }
        struct spx_object *o = (struct spx_object *) ((char *) p - SpxObjectAlignSize);
        if(0 == o->spx_object_refs){
            return 0;
        }
        return o->spx_object_refs;
    }

#define SpxObjectFree(p) \
    do { \
        if(NULL != p && spx_object_free(p)) { \
            p = NULL; \
        } \
    }while(false)

#define SpxObjectFreeForce(p) \
    do { \
        if(NULL != p) {\
            spx_object_free_force(p);\
            p = NULL; \
        } \
    }while(false)
#ifdef __cplusplus
}
#endif
#endif

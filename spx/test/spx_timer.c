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
 *       Filename:  spx_timer.c
 *        Created:  2014/11/08 09时32分58秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 ***********************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include "SpxTypes.h"
#include "SpxObject.h"

struct SpxTimer *SpxTimerNew(SpxLogDelegate *log,
        u32_t slotsCount,err_t *err){
    struct SpxTimer *t = (struct SpxTimer *) spx_alloc_alone(sizeof(*t),err);
    if(NULL == t){
        SpxLog2(log,SpxLogError,*err,
                "alloc timer is fail.");
        return NULL;
    }
    t->slotsCount = slotsCount;
    t->log = log;
    t->basetime = spx_now();
    t->header = spx_alloc(slotsCount,sizeof(struct SpxTimerSlot),err);
    if(NULL == t->header){
        SpxLog2(log,SpxLogError,*err,
                "alloc slots for timer is fail.");
        goto r1;
    }
    t->current = t->header;
    t->running = true;
    u32_t i = 0;
    for( ; i < slotsCount; i++){
        struct SpxTimerSlot *s = t->header + i;
        s->header = NULL;
        s->idx = i;
        s->count = 0;
    }
    return t;
r1:
    SpxFree(t);
    return NULL;

}

struct SpxTimerElement *SpxTimerAdd(struct SpxTimer *timer,
        u32_t expired,
        SpxExpiredDelegate *expiredHandler,
        void *arg,
        err_t *err){
    if(NULL == timer){
        *err = EINVAL;
        return NULL;
    }

    if(!timer->running){
        *err = ENOSYS;
        return NULL;
    }

    u64_t ts = spx_now();
    u32_t idx = 0;
    idx = 0 == expired ? timer->current->idx
            :(ts + expired - timer->basetime) % timer->slotsCount;
    struct SpxTimerSlot *s = timer->header + idx;
    struct SpxTimerElement *e = (struct SpxTimerElement *)
        spx_alloc_alone(sizeof(*e),err);
    if(NULL == e){
        SpxLog2(timer->log,SpxLogError,*err,
                "alloc timer element is fail.");
        return NULL;
    }
    e->expiredHander = expiredHandler;
    e->basetime = ts;
    e->expired = expired;
    e->id = SpxAtomicLazyVIncr(timer->idx);
    e->arg = arg;
    if(NULL == s->header){
        s->header = e;
        s->tail = e;
    } else {
        e->prev = s->tail;
        s->tail->next = e;
        s->tail= e;
    }
    SpxAtomicVIncr(s->count);
    return e;
}

err_t SpxTimerRemove(struct SpxTimer *timer,
        struct SpxTimerElement *e){
    if(NULL == timer){
        return EINVAL;
    }

    if(!timer->running){
        return ENOSYS;
    }
    u32_t idx = (e->basetime + e->expired - timer->basetime)
        % timer->slotsCount;
    struct SpxTimerSlot *s = timer->header + idx;
    if(NULL == e->prev){
        s->header = e->next;
    } else {
        e->prev->next = e->next;
    }
    if(NULL == e->next){
        s->tail = e->prev;
    } else {
        e->next->prev = e->prev;
    }
    SpxAtomicVDecr(s->count);
    SpxFree(e);
    return 0;
}

err_t SpxTimerElement *SpxTimerModify(struct SpxTimer *timer,
        struct SpxTimerElement *e,u32_t expired){
    if(NULL == timer){
        return EINVAL;
    }
    if(!timer->running){
        return ENOSYS;
    }

    u64_t ts = spx_now();

    u32_t oidx = (e->basetime + e->expired - timer->basetime)
        % timer->slotsCount;
    u32_t idx = (ts + expired - timer->basetime) % timer->slotsCount;
    if(oidx == idx){
        e->basetime = ts;
        e->expired = expired;
    } else {
        struct SpxTimerSlot *s = timer->header + oidx;
        if(NULL == e->prev){
            s->header = e->next;
        } else {
            e->prev->next = e->next;
        }
        if(NULL == e->next){
            s->tail = e->prev;
        } else {
            e->next->prev = e->prev;
        }
        SpxAtomicVDecr(s->count);

        e->basetime = ts;
        e->expired = expired;
        s = timer->header + idx;
        if(NULL == s->header){
            s->header = e;
            s->tail = e;
        } else {
            e->prev = s->tail;
            s->tail->next = e;
            s->tail= e;
        }
        SpxAtomicVIncr(s->count);
    }
    return 0;
}

struct SpxTimerElement *SpxTimerRunning(struct SpxTimer *timer,
        u32_t *count){
    if(!timer->running){
        return NULL;
    }
    u64_t basetime = spx_now();
    struct SpxTimerElement *l = NULL;
    struct SpxTimerElement *lt = NULL;
    struct SpxTimerElement *e = NULL;
    struct SpxTimerSlot *s = timer->current;
    struct SpxTimerElement *p = s->header;
    while(NULL != (e = p)){
        if(basetime >= e->basetime + e->expired){
            if(NULL == e->prev){
                s->header = e->next;
            } else {
                e->prev->next = e->next;
            }
            if(NULL == e->next){
                s->tail = e->prev;
            } else {
                e->next->prev = e->prev;
            }
            e->prev = NULL;
            e->next = NULL;
            if(NULL == l){
                l = e;
                lt = e;
            } else {
                e->prev = lt;
                lt->next = e;
                lt = e;
            }
            SpxAtomicVDecr(s->count);
        }
        p = p->next;
    }

    timer->curent = timer->slots + (timer->current->idx + 1)
        % timer->slotsCount;
    return l;
}

err_t SpxTimerFree(struct SpxTimer **timer){
    if(NULL == timer || NULL == *timer){
        return 0;
    }

    (*timer)->running = false;
    u32_t idx = 0;
    for( ; i < (*timer)->slotCount; i++) {
        struct SpxTimerSlot *curr = (*timer)->header + idx;
        struct SpxTimerElement *e = NULL;
        while((NULL != (e = curr->header))) {
            curr->header = e->next;
            SpxFree(e);
        }
    }
    SpxFree((*timer)->header);
    SpxFree(*timer);
    return 0;
}

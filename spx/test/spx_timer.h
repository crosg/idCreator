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
 *       Filename:  spx_timer.h
 *        Created:  2014/11/08 09时32分51秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 ****************************************************************************/
#ifndef _SPX_TIMER_H_
#define _SPX_TIMER_H_
#ifdef __cplusplus
extern "C" {
#endif


#include <stdlib.h>
#include <stdio.h>

#include "spx_types.h"

    typedef void *(SpxExpiredDelegate)(void *arg);

    struct SpxTimerElement{
        u64_t id;
        u64_t basetime;
        u32_t expired;
        SpxExpiredDelegate *expiredHander;
        void *arg;
        struct SpxTimerElement *prev;
        struct SpxTimerElement *next;
    };

    struct SpxTimerSlot{
        u64_t count;
        u32_t idx;
        struct SpxTimerElement *header;
        struct SpxTimerElement *tail;
    };

    struct SpxTimer{
        u64_t idx;
        SpxLogDelegate *log;
        u64_t basetime;
        u32_t slotsCount;
        bool_t running;
        struct SpxTimerSlot *header;
        struct SpxTimerSlot *current;
    };

    struct SpxTimer *SpxTimerNew(SpxLogDelegate *log,
            u32_t slotsCount,err_t *err);

    struct SpxTimerElement *SpxTimerAdd(struct SpxTimer *timer,
            u32_t expired,
            SpxExpiredDelegate *expiredHandler,
            void *arg,
            err_t *err);

    err_t SpxTimerRemove(struct SpxTimer *timer,
            struct SpxTimerElement *e);

    err_t SpxTimerElement *SpxTimerModify(struct SpxTimer *timer,
            struct SpxTimerElement *e,u32_t expired);

    struct SpxTimerElement *SpxTimerRunning(struct SpxTimer *timer,
            u32_t *count);

    err_t SpxTimerFree(struct SpxTimer **timer);


#ifdef __cplusplus
}
#endif
#endif

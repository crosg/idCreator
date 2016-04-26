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
 *       Filename:  spx_thread.h
 *        Created:  2014/09/01 17时27分29秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 ****************************************************************************/
#ifndef _SPX_THREAD_H_
#define _SPX_THREAD_H_
#ifdef __cplusplus
extern "C" {
#endif


#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "spx_types.h"

    typedef void (SpxThreadCleanupDelegate)(void *arg);

    /**
     * this function alloc mutex-locker must use
     * spx_thread_mutex_free function to free the mutex locker
     */
    pthread_mutex_t *spx_thread_mutex_new(SpxLogDelegate *log,
            err_t *err);
    void spx_thread_mutex_free(pthread_mutex_t **mlock);

    pthread_cond_t *spx_thread_cond_new(SpxLogDelegate *log,
            err_t *err);
    void spx_thread_cond_free(pthread_cond_t **clock);

    pthread_t spx_thread_new(SpxLogDelegate *log,
            size_t stacksize,
            void *(*start_routine)(void*),
            void *arg,
            err_t *err);

pthread_t spx_detach_thread_new(SpxLogDelegate *log,
        size_t stacksize,
        void *(*start_routine)(void*),
        void *arg,
        err_t *err);

    pthread_t spx_thread_new_cancelability(SpxLogDelegate *log,
        size_t stacksize,
        void *(*start_routine)(void *),
        void *arg,
        err_t *err);
#ifdef __cplusplus
}
#endif
#endif

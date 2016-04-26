/*
 * =====================================================================================
 *
 * this software or lib may be copied only under the terms of the gnu general
 * public license v3, which may be found in the source kit.
 *
 *       Filename:  spx_sio_thread.h
 *        Created:  2014/07/23 15时31分02秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 * =====================================================================================
 */
#ifndef _SPX_NOTIFIER_MODULE_H_
#define _SPX_NOTIFIER_MODULE_H_
#ifdef __cplusplus
extern "C" {
#endif


#include <stdlib.h>
#include <stdio.h>
#include <ev.h>

#include "spx_module.h"

    extern struct spx_module_context *g_spx_notifier_module;

    void spx_notifier_module_receive_handler(struct ev_loop *loop,ev_io *w,int revents);
//    void spx_notifier_module_wakeup_handler(struct ev_loop *loop,ev_io *w,int revents);
    void spx_notifier_module_wakeup_handler(int revents,void *arg);

#ifdef __cplusplus
}
#endif
#endif

/*
 * =====================================================================================
 *
 * this software or lib may be copied only under the terms of the gnu general
 * public license v3, which may be found in the source kit.
 *
 *       Filename:  spx_dio_thread.h
 *        Created:  2014/07/24 10时23分15秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 * =====================================================================================
 */
#ifndef _SPX_TASK_MODULE_H_
#define _SPX_TASK_MODULE_H_
#ifdef __cplusplus
extern "C" {
#endif


#include <stdlib.h>
#include <stdio.h>
#include <ev.h>

#include "spx_types.h"
#include "spx_defs.h"
#include "spx_task.h"
#include "spx_module.h"

    extern struct spx_module_context *g_spx_task_module;
    void spx_task_module_receive_handler(struct ev_loop *loop,ev_io *w,int revents);
//    void spx_task_module_wakeup_handler(struct ev_loop *loop,ev_io *w,int revents);
    void spx_task_module_wakeup_handler(int revents,void *arg);

    spx_private spx_inline int spx_task_module_wakeup_idx(int idx){
        return 0 > idx ? g_spx_task_module->threadpool->size - 1 : idx % (g_spx_task_module->threadpool->size -2);
    }

#ifdef __cplusplus
}
#endif
#endif

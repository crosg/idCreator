/*
 * =====================================================================================
 *
 * this software or lib may be copied only under the terms of the gnu general
 * public license v3, which may be found in the source kit.
 *
 *       Filename:  spx_nio_thread.h
 *        Created:  2014/07/24 10时23分03秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 * =====================================================================================
 */
#ifndef _SPX_NETWORK_MODULE_H_
#define _SPX_NETWORK_MODULE_H_
#ifdef __cplusplus
extern "C" {
#endif


#include <stdlib.h>
#include <stdio.h>
#include <ev.h>

#include "spx_module.h"
#include "spx_job.h"

    extern struct spx_module_context *g_spx_network_module;

    void spx_network_module_receive_handler(struct ev_loop *loop,ev_io *w,int revents);
//    void spx_network_module_wakeup_handler(struct ev_loop *loop,ev_io *w,int revents);
    void spx_network_module_wakeup_handler(int revents,void *arg);

    spx_private int spx_network_module_wakeup_idx(struct spx_job_context *jc){
        return jc->idx % g_spx_network_module->threadpool->size;
    }

#ifdef __cplusplus
}
#endif
#endif

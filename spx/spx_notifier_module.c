/*
 * =====================================================================================
 *
 * this software or lib may be copied only under the terms of the gnu general
 * public license v3, which may be found in the source kit.
 *
 *       Filename:  spx_sio_thread.c
 *        Created:  2014/07/23 15时31分32秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <ev.h>

#include "spx_types.h"
#include "spx_io.h"
#include "spx_defs.h"
#include "spx_module.h"
#include "spx_notifier_module.h"
#include "spx_network_module.h"
#include "spx_socket.h"
#include "spx_job.h"
#include "spx_time.h"

struct spx_module_context *g_spx_notifier_module = NULL;

void spx_notifier_module_receive_handler(struct ev_loop *loop,ev_io *w,int revents){
    struct spx_job_context *jc = NULL;

    size_t len = 0;
    struct spx_receive_context *rc = (struct spx_receive_context *) w;//magic,yeah
    err_t err= 0;
    err = spx_read_nb(w->fd,(byte_t *) &jc,sizeof(jc),&len);
    if(NULL == jc){
        SpxLog2(rc->log,SpxLogError,err,
                "recv job context is fail.");
        return;
    }

    jc->client_ip = spx_ip_get(jc->fd,&err);
    spx_set_nb(jc->fd);
    spx_tcp_nodelay(jc->fd,true);
    jc->moore = SpxNioMooreRequest;
    size_t idx = spx_network_module_wakeup_idx(jc);
    SpxLogFmt1(rc->log,SpxLogDebug,\
            "recv the client:%s connection.sock:%d."\
            "and send to thread:%d to deal.",
            jc->client_ip,jc->fd,idx);
    struct spx_thread_context *tc = spx_get_thread(g_spx_network_module,idx);
    jc->tc = tc;
    SpxModuleDispatch(spx_network_module_wakeup_handler,jc);
    return ;

}

//void spx_notifier_module_wakeup_handler(struct ev_loop *loop,ev_io *w,int revents){
void spx_notifier_module_wakeup_handler(int revents,void *arg){
    err_t err = 0;
    struct spx_job_context *jc = (struct spx_job_context *) arg;
    if((revents & EV_TIMEOUT) || (revents & EV_ERROR)){
        SpxLogFmt1(jc->log,SpxLogError,
                "wake up notify module is fail,no:%d.",revents);
        spx_job_pool_push(g_spx_job_pool,jc);
    }
    if(revents & EV_WRITE){
        SpxLogFmt1(jc->log,SpxLogDebug,"wakeup thread:%d.",jc->tc->idx );

        size_t len = 0;
        err = spx_write_nb(jc->tc->pipe[1],(byte_t *) &jc,sizeof(jc),&len);
        if (0 != err || sizeof(jc) != len) {
            spx_job_pool_push(g_spx_job_pool,jc);
            SpxLog1(jc->log,SpxLogError,\
                    "wake up notify module is fail.");
        }
    }
}

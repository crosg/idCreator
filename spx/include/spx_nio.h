/*
 * =====================================================================================
 *
 *       Filename:  spx_nio.h
 *
 *    Description:  ,e
 *
 *        Version:  1.0
 *        Created:  2014/06/09 17时43分29秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#ifndef _SPX_NIO_H_
#define _SPX_NIO_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <ev.h>

#include "spx_types.h"
#include "spx_job.h"
#include "spx_task.h"

    typedef void (SpxAsyncDelegate)(struct ev_loop *loop, ev_async *w, int revents);
    typedef void (SpxSateDelegate)(struct ev_loop *loop, ev_stat *w, int revents);

err_t  spx_dio_regedit_async(ev_async *w,
        SpxAsyncDelegate *reader,void *data);

void spx_dio_async_launch(struct ev_loop *loop,ev_async *w,
        SpxAsyncDelegate *hander,void *data);

void spx_dio_regedit_stat(ev_stat *w,string_t path,double ts,SpxSateDelegate *cb,void *arg);



void spx_nio_reader_with_timeout(int revents,void *arg);
void spx_nio_writer_with_timeout(int revents,void *arg);
void spx_nio_reader_with_timeout_fast(int revents,void *arg);
void spx_nio_writer_with_timeout_fast(int revents,void *arg);



#ifdef __cplusplus
}
#endif
#endif

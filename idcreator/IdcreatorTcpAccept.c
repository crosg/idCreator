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
 *       Filename:  IdcreatorTcpAccept.c
 *        Created:  2015年03月03日 11时27分35秒
 *         Author:  Seapeak.Xu (www.94geek.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 ***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ev.h>

#include "spx_types.h"
#include "spx_string.h"
#include "spx_defs.h"
#include "spx_socket.h"
#include "spx_module.h"
#include "spx_job.h"
#include "spx_time.h"
#include "spx_alloc.h"
#include "spx_io.h"
#include "spx_nio.h"

#include "IdcreatorTcpAccept.h"
#include "IdcreatorConfig.h"
#include "IdcreatorServerContext.h"
#include "IdcreatorCore.h"
#include "IdcreatorProtocol.h"

struct mainsocket_thread_arg{
    SpxLogDelegate *log;
    struct IdcreatorConfig *c;
};

spx_private struct ev_loop *main_socket_loop = NULL;
spx_private ev_io main_watcher;

spx_private void *idcreatorTcpCreate(void *arg);
spx_private void idcreatorTcpAcceptHandler(struct ev_loop *loop,
        ev_io *watcher,int revents);
spx_private void idcreatorTcpAccept(struct IdcreatorConfig *c,struct ev_loop *loop,int fd);
spx_private void idcreatorDispatcherHandler(struct ev_loop *loop,ev_async *w,int revents);

spx_private void idcreatorTcpAccept(struct IdcreatorConfig *c,struct ev_loop *loop,int fd){/*{{{*/
    SpxZero(main_watcher);
    ev_io_init(&main_watcher,idcreatorTcpAcceptHandler,fd,EV_READ);
    main_watcher.data = c;
     ev_io_start(loop,&(main_watcher));
    ev_run(loop,0);
}/*}}}*/

spx_private void idcreatorTcpAcceptHandler(struct ev_loop *loop,ev_io *watcher,int revents){/*{{{*/
    ev_io_stop(loop,watcher);
    struct IdcreatorConfig *c = (struct IdcreatorConfig *) watcher->data;
//    SpxLogDelegate *log = (SpxLogDelegate *) watcher->data;
    err_t err = 0;
    while(true){
        struct sockaddr_in client_addr;
        unsigned int socket_len = 0;
        int client_sock = 0;
        socket_len = sizeof(struct sockaddr_in);
        client_sock = accept(watcher->fd, (struct sockaddr *) &client_addr,
                &socket_len);
        if (0 > client_sock) {
            if (EWOULDBLOCK == errno || EAGAIN == errno) {
                break;
            }
            break;
        }

        if (0 == client_sock) {
            break;
        }

        struct IdcreatorServerContext *isc = idcreatorServerContextPoolPop(gIdcreatorServerContextPool,&err);
        if(!isc) {
            SpxClose(client_sock);
            SpxLog1(c->log,SpxLogError,\
                    "pop isc context is fail.");
            break;
        }

        isc->fd = client_sock;
        isc->loop = loop;

        ev_once(loop,isc->fd,EV_READ,(ev_tstamp) c->waitting,isc->inHandler,(void *) isc);
    }
    ev_io_start(loop,watcher);
}/*}}}*/

spx_private void *idcreatorTcpCreate(void *arg){/*{{{*/
    struct mainsocket_thread_arg *mainsocket_arg = (struct mainsocket_thread_arg *) arg;
    SpxLogDelegate *log = mainsocket_arg->log;
    struct IdcreatorConfig *c= mainsocket_arg->c;
    SpxFree(mainsocket_arg);
    err_t err = 0;
    main_socket_loop = ev_loop_new(0);
    if(NULL == main_socket_loop){
        SpxLog2(log,SpxLogError,err,"create main socket loop is fail.");
        return NULL;
    }
    int mainsocket =  spx_socket_new(&err);
    if(0 == mainsocket){
        SpxLog2(log,SpxLogError,err,"create main socket is fail.");
        return NULL;
    }

    if(0!= (err = spx_set_nb(mainsocket))){
        SpxLog2(log,SpxLogError,err,"set main socket nonblock is fail.");
        goto r1;
    }

    if(0 != (err =  spx_socket_start(mainsocket,c->ip,c->port,\
                    true,c->timeout,\
                    3,c->timeout,\
                    false,0,\
                    true,\
                    true,c->timeout,
                    1024))){
        SpxLog2(log,SpxLogError,err,"start main socket is fail.");
        goto r1;
    }

    SpxLogFmt1(log,SpxLogMark,
            "main socket fd:%d."
            "and accepting...",
            mainsocket);

    idcreatorTcpAccept(c,main_socket_loop,mainsocket);
r1:
    SpxClose(mainsocket);
    return NULL;
}/*}}}*/


pthread_t idcreatorMainTcpThreadNew(SpxLogDelegate *log,struct IdcreatorConfig *c,err_t *err) {/*{{{*/
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    size_t ostack_size = 0;
    pthread_attr_getstacksize(&attr, &ostack_size);
    if (ostack_size != c->stackSize
            && (0 != (*err = pthread_attr_setstacksize(&attr,c->stackSize)))){
        return 0;
    }
    struct mainsocket_thread_arg *arg =(struct mainsocket_thread_arg *) spx_alloc_alone(sizeof(*arg),err);
    if(NULL == arg){
        pthread_attr_destroy(&attr);
        return 0;
    }
    arg->log = log;
    arg->c = c;

    pthread_t tid = 0;
    if (0 !=(*err =  pthread_create(&tid, &attr, idcreatorTcpCreate,
                    arg))){
        pthread_attr_destroy(&attr);
        SpxFree(arg);
        return 0;
    }
    pthread_attr_destroy(&attr);
    return tid;
}/*}}}*/

void idcreatorNetworkReceiverHandler(int revents,void *arg) {/*{{{*/
    SpxTypeConvert2(struct IdcreatorServerContext,isc,arg);
    if(NULL == isc){
        return;
    }
    SpxTypeConvert2(struct IdcreatorConfig,c,isc->c);
    if(EV_TIMEOUT & revents){
        SpxLog1(isc->log,SpxLogError,\
                "waitting read from client is timeout."
                "and then push the server context to pool.");
        idcreatorServerContextPoolPush(gIdcreatorServerContextPool,isc);
        return;
    }
    err_t err = 0;

    if(EV_READ & revents){
        size_t len = 0;
        err = spx_read_ack(isc->fd,
                (byte_t *) isc->inbuf + isc->offset, isc->inlen,&len);
        if(err) {
            if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err){
                isc->offset += len;
                ev_once(isc->loop,isc->fd,EV_READ,c->waitting,isc->inHandler,(void *) isc);
                return;
            } else {
                SpxLogFmt2(isc->log,SpxLogError,err,
                        "read body is fail.bodylen:%d,recvlen:%d."
                        "and then push the server context to pool.",
                        isc->inlen,isc->offset);
                idcreatorServerContextPoolPush(gIdcreatorServerContextPool,isc);
                return;
            }
        }

        spx_dio_async_launch(isc->loop,&(isc->async),idcreatorDispatcherHandler,isc);
        return;
    }
    return;
}/*}}}*/

void idcreatorNetworkSenderHandler(int revents,void *arg) {/*{{{*/
    SpxTypeConvert2(struct IdcreatorServerContext,isc,arg);
    if(NULL == isc){
        return;
    }
    SpxTypeConvert2(struct IdcreatorConfig,c,isc->c);
    if(EV_TIMEOUT & revents){
        SpxLogFmt1(isc->log,SpxLogError,
                "wait for writing to client is fail."
                "bodylen:%d,writed len:%d."
                "and then push the server context to pool.",
                isc->outlen,isc->offset);
        idcreatorServerContextPoolPush(gIdcreatorServerContextPool,isc);
        return;
    }
    err_t err = 0;

    if(EV_WRITE & revents){
        size_t len = 0;
        err = spx_write_ack(isc->fd,
                (byte_t *) isc->outbuf + isc->offset, isc->outlen,&len);
        if(err) {
            if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err){
                isc->offset += len;
                ev_once(isc->loop,isc->fd,EV_WRITE,c->waitting,isc->outHandler,(void *) isc);
                return;
            } else {
                SpxLogFmt2(isc->log,SpxLogError,err,
                        "wait for writing to client is fail."
                        "bodylen:%d,writed len:%d."
                        "and then push the server context to pool.",
                        isc->outlen,isc->offset);
                idcreatorServerContextPoolPush(gIdcreatorServerContextPool,isc);
                return;
            }
        }
        idcreatorServerContextPoolPush(gIdcreatorServerContextPool,isc);
    }
    return;
}/*}}}*/

spx_private void idcreatorDispatcherHandler(struct ev_loop *loop,ev_async *w,int revents){/*{{{*/
    ev_async_stop(loop,w);
    SpxTypeConvert2(struct IdcreatorServerContext,isc,w->data);
    err_t err = 0;
    u64_t id = 0l;
    int type = 0;

    type = spx_msg_b2i((uchar_t *) isc->inbuf + SpxMsgHeaderSize);

    SpxTypeConvert2(struct IdcreatorConfig,c,isc->c);
    if((0x4ff != type) && (0 > type || 0x3ff < type)){
        type = 0x3ff;
    }

    int mid = c->mid;

    id = IdGenerator(type, mid, &err);

    isc->outheader.version = IdcreatorVersion;
    isc->outheader.protocol = IdcreatorMakeId;
    isc->outheader.bodylen = sizeof(u64_t);

    spx_header_pack(isc->outbuf,&(isc->outheader));
    spx_msg_ul2b((uchar_t *) isc->outbuf + SpxMsgHeaderSize,id);

    idcreatorNetworkSenderHandler(EV_WRITE,isc);
    return;
}/*}}}*/

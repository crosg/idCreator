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
 *       Filename:  IdcreatorServerContext.c
 *        Created:  2015年03月03日 09时59分41秒
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
#include "spx_io.h"
#include "spx_alloc.h"

#include "IdcreatorServerContext.h"
#include "IdcreatorConfig.h"

struct IdcreatorServerContextPool *gIdcreatorServerContextPool = NULL;

spx_private void *idcreatorServerContextNew(size_t idx,void *arg,err_t *err);
spx_private err_t idcreatorServerContextFree(void **arg);
spx_private void idcreatorServerContextClear(struct IdcreatorServerContext *isc);

spx_private void *idcreatorServerContextNew(size_t idx,void *arg,err_t *err){/*{{{*/
    SpxTypeConvert2(struct IdcreatorServerContextTransport,isct,arg);
    struct IdcreatorServerContext *isc = NULL;
    isc = spx_alloc_alone(sizeof(*isc),err);
    if(NULL == isc){
        return NULL;
    }
    isc->log = isct->log;
    isc->idx = idx;
    isc->c = isct->c;
    isc->inHandler = isct->inHandler;
    isc->outHandler = isct->outHandler;
    isc->inlen = SpxMsgHeaderSize + sizeof(i32_t);
    isc->outlen = SpxMsgHeaderSize + sizeof(u64_t);
    return isc;

}/*}}}*/

spx_private err_t idcreatorServerContextFree(void **arg){/*{{{*/
    struct IdcreatorServerContext **isc = (struct IdcreatorServerContext **) arg;
    idcreatorServerContextClear(*isc);
    SpxFree(*isc);
    return 0;
}/*}}}*/

spx_private void idcreatorServerContextClear(struct IdcreatorServerContext *isc){/*{{{*/
    if(NULL != isc->client_ip){
        SpxStringFree(isc->client_ip);
    }

    isc->err = 0;
    isc->moore = SpxNioMooreNormal;
    isc->loop = NULL;
    memset(isc->inbuf,0,SpxMsgHeaderSize + sizeof(i32_t));
    memset(isc->outbuf,0,SpxMsgHeaderSize + sizeof(u64_t));
    memset(&(isc->inheader),0,sizeof(isc->inheader));
    memset(&(isc->outheader),0,sizeof(isc->outheader));
    isc->offset = 0;
    if(0 < isc->fd){
        SpxClose(isc->fd);
    }
}/*}}}*/


void idcreatorServerContextReset(struct IdcreatorServerContext *isc){
}


struct IdcreatorServerContextPool *idcreatorServerContextPoolNew(SpxLogDelegate *log,\
        void *c,
        size_t size,
        IdcreatorWatcherDelegate *inHandler,
        IdcreatorWatcherDelegate *outHandler,
        err_t *err){/*{{{*/
    if(0 == size){
        *err = EINVAL;
    }
    struct IdcreatorServerContextPool *pool = NULL;
    pool = spx_alloc_alone(sizeof(*pool),err);
    if(NULL == pool){
        return NULL;
    }

    struct IdcreatorServerContextTransport arg;
    SpxZero(arg);
    arg.log = log;
    arg.c = c;
    arg.inHandler = inHandler;
    arg.outHandler = outHandler;

    pool->pool = spx_fixed_vector_new(log,size,\
            idcreatorServerContextNew,\
            &arg,\
            idcreatorServerContextFree,\
            err);

    if(NULL == pool->pool){
        SpxFree(pool);
        return NULL;
    }
    return pool;
}/*}}}*/

struct IdcreatorServerContext *idcreatorServerContextPoolPop(struct IdcreatorServerContextPool *pool,err_t *err){/*{{{*/
    struct IdcreatorServerContext *isc = spx_fixed_vector_pop(pool->pool,err);
    if(NULL == isc){
        *err = 0 == *err ? ENOENT : *err;
        return NULL;
    }
    return isc;
}/*}}}*/

err_t idcreatorServerContextPoolPush(struct IdcreatorServerContextPool *pool,struct IdcreatorServerContext *isc){/*{{{*/
    idcreatorServerContextClear(isc);
    return spx_fixed_vector_push(pool->pool,isc);
}/*}}}*/

err_t idcreatorServerContextPoolFree(struct IdcreatorServerContextPool **pool){/*{{{*/
    err_t err = 0;
    err = spx_fixed_vector_free(&((*pool)->pool));
    SpxFree(*pool);
    return err;
}/*}}}*/




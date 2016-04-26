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
 *       Filename:  IdcreatorServerContext.h
 *        Created:  2015年03月03日 09时59分39秒
 *         Author:  Seapeak.Xu (www.94geek.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 ****************************************************************************/
#ifndef _idcreatorSERVERCONTEXT_H_
#define _idcreatorSERVERCONTEXT_H_
#ifdef __cplusplus
extern "C" {
#endif


#include <stdlib.h>
#include <stdio.h>
#include <ev.h>

#include "spx_types.h"
#include "spx_defs.h"
#include "spx_message.h"
#include "spx_properties.h"
#include "spx_module.h"

#define IdcreatorMooreNormal 0
#define IdcreatorMooreIn 1
#define IdcreatorMooreOut 2

     struct IdcreatorServerContext;

    typedef void IdcreatorWatcherDelegate(int revents,void *arg);
    typedef void IdcreatorDispatcherDelegate(struct ev_loop *loop,
            int idx,struct IdcreatorServerContext *tsc);

    struct IdcreatorServerContextTransport{
        SpxLogDelegate *log;
        void *c;
        IdcreatorWatcherDelegate *inHandler;
        IdcreatorWatcherDelegate *outHandler;
    };

    struct IdcreatorServerContext{
        SpxLogDelegate *log;
        struct ev_async async;
        int fd;
        int use;
        size_t idx;
        err_t err;
        IdcreatorWatcherDelegate *inHandler;
        IdcreatorWatcherDelegate *outHandler;
//        IdcreatorDispatcherDelegate *dispatcher;
        u32_t moore;
        struct ev_loop *loop;
        string_t client_ip;
        void *c;

        struct spx_msg_header inheader;
        struct spx_msg_header outheader;

        char inbuf[SpxMsgHeaderSize + sizeof(i32_t)];
        char outbuf[SpxMsgHeaderSize + sizeof(u64_t)];
        int inlen;
        int outlen;
        u32_t offset;
    };

    struct IdcreatorServerContextPool {
        SpxLogDelegate *log;
        struct spx_fixed_vector *pool;
    };


    extern struct IdcreatorServerContextPool *gIdcreatorServerContextPool;

struct IdcreatorServerContextPool *idcreatorServerContextPoolNew(SpxLogDelegate *log,\
        void *c,
        size_t size,
        IdcreatorWatcherDelegate *inHandler,
        IdcreatorWatcherDelegate *outHandler,
        err_t *err);
struct IdcreatorServerContext *idcreatorServerContextPoolPop(struct IdcreatorServerContextPool *pool,err_t *err);
err_t idcreatorServerContextPoolPush(struct IdcreatorServerContextPool *pool,struct IdcreatorServerContext *tsc);
err_t idcreatorServerContextPoolFree(struct IdcreatorServerContextPool **pool);

#ifdef __cplusplus
}
#endif
#endif

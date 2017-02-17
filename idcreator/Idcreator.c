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
 *       Filename:  Idcreator.c
 *        Created:  2015年03月03日 15时57分52秒
 *         Author:  Seapeak.Xu (www.94geek.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 ***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include <ev.h>


#include "spx_types.h"
#include "spx_string.h"
#include "spx_defs.h"
#include "spx_log.h"
#include "spx_socket.h"
#include "spx_env.h"
#include "spx_nio.h"
#include "spx_configurtion.h"
#include "spx_module.h"

#include "IdcreatorConfig.h"
#include "IdcreatorServerContext.h"
#include "IdcreatorTcpAccept.h"
#include "IdcreatorHttpAccept.h"
#include "IdcreatorStateKeep.h"


spx_private void idcreatorRegeditSigle();
spx_private void idcreatorSigactionMark(int sig);
spx_private void idcreatorSigactionExit(int sig);



int main(int argc, char **argv) {
    umask(0);
    SpxLogDelegate *log = spx_log;
    if(2 != argc){
        SpxLog1(log,SpxLogError,"no the configurtion file in the argument.");
        return ENOENT;
    }
    err_t err = 0;
    string_t confname = spx_string_new(argv[1],&err);
    if(NULL == confname){
        SpxLog2(log,SpxLogError,err,
                "new the confname is fail."
                "and will exit...");
        exit(err);
    }

    struct IdcreatorConfig *c = (struct IdcreatorConfig *) \
                               spx_configurtion_parser(log,\
                                       idcreatorConfigInit,\
                                       NULL,\
                                       confname,\
                                       idcreatorConfigParser,\
                                       &err);
    if(NULL == c){
        SpxLogFmt2(log,SpxLogError,err,
                "parser the configurtion is fail.file name:%s."
                "and will exit...",
                confname);
        exit(err);
    }

    if(c->daemon){
        spx_env_daemon();
    }

    if(0 != ( err = spx_log_new(\
                    log,\
                    c->logpath,\
                    c->logprefix,\
                    c->logsize,\
                    c->loglevel))){
        SpxLog2(log,SpxLogError,err,
                "init the logger is fail."
                "and will exit...");
        exit(err);
    }

    idcreatorRegeditSigle();

    gIdcreatorServerContextPool = idcreatorServerContextPoolNew(log,
            c,c->serverContextSize,
            idcreatorNetworkReceiverHandler,
            idcreatorNetworkSenderHandler,
            &err);
    if(NULL == gIdcreatorServerContextPool){
        SpxLog2(log,SpxLogError,err,
                "alloc job pool is fail."
                "and will exit...");
        exit(err);
    }

    err = idcreator_statef_open (c);
    if(0 != err){
        SpxLog2(log,SpxLogError,err,
                "create or re-read id state file is fail"
                "and will exit...");
        exit(err);
    }

    pthread_t tid = idcreatorMainTcpThreadNew(log,c,&err);
    if(0 != err){
        SpxLog2(log,SpxLogError,err,
                "create main socket thread is fail."
                "and will exit...");
        exit(err);
    }

    SpxLogFmt1(log,SpxLogMark,
            "tracker start over."
            "ip:%s,port:%d",
            c->ip,c->port);

    pthread_t tid_http = NewIdCreatorHttp(log,c,&err);
    if(0 != err){
        SpxLog2(log,SpxLogError,err,
                "create main socket thread is fail."
                "and will exit...");
        exit(err);
    }

    pthread_join(tid,NULL);
    pthread_join(tid_http, NULL);

    return 0;
}

spx_private void idcreatorRegeditSigle(){
    spx_env_sigaction(SIGUSR1,NULL);
    spx_env_sigaction(SIGHUP,idcreatorSigactionMark);
    spx_env_sigaction(SIGPIPE,idcreatorSigactionMark);
    spx_env_sigaction(SIGINT,idcreatorSigactionExit);
}

spx_private void idcreatorSigactionMark(int sig){
    SpxLogDelegate *log = spx_log;
    SpxLogFmt1(log,SpxLogMark,
            "emerge sig:%d,info:%s.",
            sig,strsignal(sig));
}

spx_private void idcreatorSigactionExit(int sig){
    SpxLogDelegate *log = spx_log;
    SpxLogFmt1(log,SpxLogMark,
            "emerge sig:%d,info:%s.",
            sig,strsignal(sig));
    exit(0);
}


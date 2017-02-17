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
 *       Filename:  IdcreatorConfig.c
 *        Created:  2015年03月03日 11时25分12秒
 *         Author:  Seapeak.Xu (www.94geek.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 ***********************************************************************/

#include <stdlib.h>
#include <stdio.h>

#include "spx_types.h"
#include "spx_string.h"
#include "spx_defs.h"
#include "spx_log.h"
#include "spx_socket.h"
#include "spx_env.h"
#include "spx_nio.h"
#include "spx_configurtion.h"
#include "spx_module.h"
#include "spx_alloc.h"

#include "IdcreatorConfig.h"


spx_private u64_t idcreatorConfigIoSizeConvert(
                SpxLogDelegate *log,string_t v,u32_t default_unitsize,
                char *errinfo,err_t *err){/* {{{*/
    u64_t size = strtoul(v,NULL,10);
    if(ERANGE == size) {
        SpxLog1(log,SpxLogError,\
                errinfo);
        *err = size;
        return 0;
    }
    string_t unit = spx_string_range_new(v,\
            -2,spx_string_len(v),err);
    if(NULL == unit){
        size *= default_unitsize;
    }
    if(0 == spx_string_casecmp(unit,"GB")){
        size *= SpxGB;
    }
    else if( 0 == spx_string_casecmp(unit,"MB")){
        size *= SpxMB;
    }else if(0 == spx_string_casecmp(unit,"KB")){
        size *= SpxKB;
    }else {
        size *= default_unitsize;
    }
    spx_string_free(unit);
    return size;
}/* }}}*/

spx_private u32_t idcreatorConfigTimespanConvert(
                SpxLogDelegate *log,string_t v,u32_t default_unitsize,
                char *errinfo,err_t *err){/* {{{*/
    u64_t size = strtoul(v,NULL,10);
    if(ERANGE == size) {
        SpxLog1(log,SpxLogError,\
                errinfo);
        *err = size;
        return 0;
    }
    string_t unit = spx_string_range_new(v,\
            -2,spx_string_len(v),err);
    if(NULL == unit){
        size *= default_unitsize;
    }
    if(0 == spx_string_casecmp(unit,"D")){
        size *= SpxDayTick;
    }
    else if( 0 == spx_string_casecmp(unit,"H")){
        size *= SpxHourTick;
    }else if(0 == spx_string_casecmp(unit,"M")){
        size *= SpxMinuteTick;
    }else if(0 == spx_string_casecmp(unit,"S")){
        size *= SpxSecondTick;
    }else {
        size *= default_unitsize;
    }
    spx_string_free(unit);
    return size;
}/* }}}*/


void *idcreatorConfigInit(SpxLogDelegate *log,err_t *err){/*{{{*/

    struct IdcreatorConfig *c = (struct IdcreatorConfig *)
        spx_alloc_alone(sizeof(*c),err);
    if(NULL == c){
        SpxLog2(log,SpxLogError,*err,\
                "new the config is fail.");
        return NULL;
    }
    c->log = log;
    c->port = 9048;
    c->connectTimeout = 3;
    c->logsize = 10 * SpxMB;
    c->loglevel = SpxLogInfo;
    c->daemon = true;
    c->http_port= 8081;
    c->stackSize = 128 * SpxKB;
    c->waitting = 10;
    c->serverContextSize = 1000;
    c->timeout = 30;
    c->mid = 0;
    return c;
}/*}}}*/


void idcreatorConfigParser(string_t line,void *config,err_t *err){/*{{{*/
    SpxTypeConvert2(struct IdcreatorConfig,c,config);
    int count = 0;
    string_t *kv = spx_string_splitlen(line,\
            spx_string_len(line),"=",strlen("="),&count,err);
    if(NULL == kv){
        return;
    }

    spx_string_trim(*kv," ");
    if(2 == count){
        spx_string_trim(*(kv + 1)," ");
    }

    //ip
    if(0 == spx_string_casecmp(*kv,"ip")){
        if(2 == count){
            if(spx_socket_is_ip(*(kv + 1))){
                c->ip = spx_string_dup(*(kv + 1),err);
                if(NULL == c->ip){
                    SpxLog2(c->log,SpxLogError,*err,\
                            "dup the ip from config operator is fail.");
                }
            } else {
                string_t ip = spx_socket_getipbyname(*(kv + 1),err);
                if(NULL == ip){
                    SpxLogFmt2(c->log,SpxLogError,*err,\
                            "get local ip by hostname:%s is fail.",*(kv + 1));
                    goto r1;
                }
                c->ip = ip;
            }
        } else{
            SpxLog1(c->log,SpxLogWarn,"use the default ip.");
            string_t ip = spx_socket_getipbyname(NULL,err);
            if(NULL == ip){
                SpxLog2(c->log,SpxLogError,*err,\
                        "get local ip by default hostname is fail.");
                goto r1;
            }
            c->ip = ip;
        }
        goto r1;
    }

    //port
    if(0 == spx_string_casecmp(*kv,"port")){
        if(1 == count){
            SpxLogFmt1(c->log,SpxLogWarn,"the port is use default:%d.",c->port);
            goto r1;
        }
        i32_t port = strtol(*(kv + 1),NULL,10);
        if(ERANGE == port) {
            SpxLog1(c->log,SpxLogError,"bad the configurtion item of port.");
            goto r1;
        }
        c->port = port;
        goto r1;
    }

    //http_port
    if(0 == spx_string_casecmp(*kv,"http_port")){
        if(1 == count){
            SpxLogFmt1(c->log,SpxLogWarn,"the http_port is use default:%d.",c->http_port);
            goto r1;
        }
        i32_t http_port = strtol(*(kv + 1),NULL,10);
        if(ERANGE == http_port) {
            SpxLog1(c->log,SpxLogError,"bad the configurtion item of http_port.");
            goto r1;
        }
        c->http_port = http_port;
        goto r1;
    }


    //connectTimeout
    if(0 == spx_string_casecmp(*kv,"connectTimeout")){
        if(1 == count){
            SpxLogFmt1(c->log,SpxLogWarn,"use default connectTimeout:%d.",c->connectTimeout);
        } else {
            u32_t connectTimeout = idcreatorConfigTimespanConvert(c->log,*(kv + 1),
                    SpxSecondTick,
                    "bad the configurtion item of connectTimeout.",err);
            if(0 != *err) {
                goto r1;
            }
            c->connectTimeout = connectTimeout;
        }
        goto r1;
    }

    //timeout
    if(0 == spx_string_casecmp(*kv,"timeout")){
        if(1 == count){
            SpxLogFmt1(c->log,SpxLogWarn,"use default timeout:%d.",c->timeout);
        } else {
            u32_t timeout = idcreatorConfigTimespanConvert(c->log,*(kv + 1),
                    SpxSecondTick,
                    "bad the configurtion item of timeout.",err);
            if(0 != *err) {
                goto r1;
            }
            c->timeout = timeout;
        }
        goto r1;
    }



    //daemon
    if(0 == spx_string_casecmp(*kv,"daemon")){
        if(1 == count){
            SpxLogFmt1(c->log,SpxLogWarn,"instance use default daemon:%d.",c->daemon);
        } else {
            string_t s = *(kv + 1);
            if(0 == spx_string_casecmp(s,spx_bool_desc[false])){
                c->daemon = false;
            } else if(0 == spx_string_casecmp(s,spx_bool_desc[true])){
                c->daemon = true;
            } else {
                c->daemon = true;
            }
        }
        goto r1;
    }


    //stackSize
    if(0 == spx_string_casecmp(*kv,"stackSize")){
        if(1 == count){
            SpxLogFmt1(c->log,SpxLogWarn,"stackSize use default:%lld.",c->stackSize);
        } else {
            u64_t size = idcreatorConfigIoSizeConvert(
                    c->log,*(kv + 1),SpxKB,
                    "convert stackSize is fail.",err);
            if(0 != *err){
                goto r1;
            }
            c->stackSize = size;
        }
        goto r1;
    }


    //logpath
    if(0 == spx_string_casecmp(*kv,"logpath")){
        if(1 == count){
            c->logpath = spx_string_new("/opt/idcreator/log",err);
            if(NULL == c->logpath){
                SpxLog2(c->log,SpxLogError,*err,\
                        "alloc default logpath is fail.");
                goto r1;
            }
            SpxLogFmt1(c->log,SpxLogWarn,\
                    "logpath use default:%s.",c->logpath);
        }else {
            c->logpath = spx_string_dup(*(kv + 1),err);
            if(NULL == c->logpath){
                SpxLog2(c->log,SpxLogError,*err,\
                        "dup the string for logpath is fail.");
            }
        }
        goto r1;
    }


    //id_state_file
    if(0 == spx_string_casecmp(*kv,"id-state-file")){
        if(1 == count){
            c->id_state_file = spx_string_new("/opt/idcreator/id.sf",err);
            if(NULL == c->id_state_file){
                SpxLog2(c->log,SpxLogError,*err,\
                        "alloc default id_state_file is fail.");
                goto r1;
            }
            SpxLogFmt1(c->log,SpxLogWarn,\
                    "id_state_file use default:%s.",c->id_state_file);
        }else {
            c->id_state_file = spx_string_dup(*(kv + 1),err);
            if(NULL == c->id_state_file){
                SpxLog2(c->log,SpxLogError,*err,\
                        "dup the string for id_state_file is fail.");
            }
        }
        goto r1;
    }

    //logprefix
    if(0 == spx_string_casecmp(*kv,"logprefix")){
        if( 1 == count){
            c->logprefix = spx_string_new("idcreator",err);
            if(NULL == c->logprefix){
                SpxLog2(c->log,SpxLogError,*err,\
                        "alloc default logprefix is fail.");
                goto r1;
            }
            SpxLogFmt1(c->log,SpxLogWarn,\
                    "logprefix use default:%s.",c->logprefix);
        } else {
            c->logprefix = spx_string_dup(*(kv + 1),err);
            if(NULL == c->logprefix){
                SpxLog2(c->log,SpxLogError,*err,\
                        "dup the string for logprefix is fail.");
            }
        }
        goto r1;
    }

    //logsize
    if(0 == spx_string_casecmp(*kv,"logsize")){
        if(1 == count){
            SpxLogFmt1(c->log,SpxLogWarn,\
                    "logsize use default:%lld.",c->logsize);
        } else {
            u64_t size = idcreatorConfigIoSizeConvert(
                    c->log,*(kv + 1),SpxMB,
                    "convert logsize is fail.",err);
            if(0 != *err){
                goto r1;
            }
            c->logsize = size;
        }
        goto r1;
    }

    //loglevel
    if(0 == spx_string_casecmp(*kv,"loglevel")){
        if(1 == count){
            SpxLogFmt1(c->log,SpxLogWarn,\
                    "loglevel use default:%s",SpxLogDesc[c->loglevel]);
        } else {
            string_t s = *(kv + 1);
            if(0 == spx_string_casecmp(s,"debug")){
                c->loglevel = SpxLogDebug;
            } else if(0 == spx_string_casecmp(s,"info")){
                c->loglevel = SpxLogInfo;
            }else if(0 == spx_string_casecmp(s,"warn")){
                c->loglevel = SpxLogWarn;
            }else if(0 == spx_string_casecmp(s,"error")){
                c->loglevel = SpxLogError;
            } else {
                c->loglevel = SpxLogInfo;
            }
        }
        goto r1;
    }

    //serverContextSize
    if(0 == spx_string_casecmp(*kv,"serverContextSize")){
        if(1 == count){
            SpxLogFmt1(c->log,SpxLogWarn,\
                    "serverContextSize use default:%d.",\
                    c->serverContextSize);
        } else {
            u32_t serverContextSize = strtol(*(kv + 1),NULL,10);
            if(ERANGE == serverContextSize) {
                SpxLog1(c->log,SpxLogError,"bad the configurtion item of serverContextSize.");
                goto r1;
            }
            c->serverContextSize = serverContextSize;
        }
        goto r1;
    }

    //waitting
    if(0 == spx_string_casecmp(*kv,"waitting")){
        if(1 == count){
            SpxLogFmt1(c->log,SpxLogWarn,"use default waitting:%d.",c->waitting);
        } else {
            u32_t waitting = idcreatorConfigTimespanConvert(c->log,*(kv + 1),
                    SpxSecondTick,
                    "bad the configurtion item of waitting.",err);
            if(0 != *err) {
                goto r1;
            }
            c->waitting = waitting;
        }
        goto r1;
    }

    //mid
    if(0 == spx_string_casecmp(*kv,"mid")){
        if(1 == count){
            SpxLogFmt1(c->log,SpxLogWarn,"use default mid:%d.",c->mid);
        } else {
            u32_t mid = idcreatorConfigTimespanConvert(c->log,*(kv + 1),
                    SpxSecondTick,
                    "bad the configurtion item of mid.",err);
            if(0 != *err) {
                goto r1;
            }
            c->mid = mid;
        }
        goto r1;
    }


r1:
    spx_string_free_splitres(kv,count);
}/*}}}*/

/*
 * =====================================================================================
 *
 *       Filename:  spx_log_test.c
 *
 *    Description:  the spx_log test
 *
 *        Version:  1.0
 *        Created:  2014/05/21 09时09分21秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "spx_defs.h"
#include "spx_types.h"
#include "spx_string.h"
#include "spx_log.h"

int main(int argc,char **argv){
    umask(0);
    string_t path = (string_t) "./spxlib_test/log";
    string_t name =(string_t)  "logtest";
    u64_t size = 1 * SpxMB;
    u8_t level = SpxLogDebug;
    err_t err = 0;
    SpxLogDelegate *log = spx_log;
    SpxLog1(log,SpxLogMark,"test np the og file.");
    if( 0 != (err = spx_log_new(spx_log,path,name,size,level))){
        SpxLog2(log,SpxLogInfo,err,"create log is fail.");
        return 0;
    }

    spx_log(SpxLogDebug,(string_t) "test log deug,errno:%d.",ENOMEM);
    spx_log(SpxLogMark,(string_t) "mark tet");
    spx_log(SpxLogInfo,(string_t )("test"));
    SpxLog1(log,SpxLogDebug,"printf info by SpxLog1.");
    SpxLog2(log,SpxLogInfo,EEXIST,"printf log info by SpxLog2");

    SpxLogFmt1(log,SpxLogMark,"printf log info by SpxLogFmt1.%s","success") \
    SpxLogFmt2(log,SpxLogError,\
            EACCES,"printf log info by SpxLogFmt2.%s","success") \
    spx_log_free();
    return 0;
}

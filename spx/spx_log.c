#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>


#include "spx_alloc.h"
#include "spx_errno.h"
#include "spx_string.h"
#include "spx_types.h"
#include "spx_path.h"
#include "spx_defs.h"
#include "spx_time.h"
#include "spx_thread.h"
#include "spx_log.h"


spx_private struct spx_log *g_spx_log = NULL;

spx_private spx_inline string_t get_log_line(err_t *err,
        u8_t level,string_t fmt,va_list ap);

spx_private FILE *logf_create(SpxLogDelegate log,
        const string_t path,
        const string_t name,u64_t max_size,
        err_t *err);

spx_private spx_inline void logf_close();


err_t spx_log_new(SpxLogDelegate log,
        const string_t path,
        const string_t name,const u64_t max_size,
        const u8_t level){/*{{{*/
    if(SpxStringIsNullOrEmpty(path)
            || SpxStringIsNullOrEmpty(name)){
        return EINVAL;
    }
    err_t err = 0;
    u64_t size = 0 == max_size ? 10 * SpxMB : max_size;
    g_spx_log = spx_alloc_alone(sizeof(struct spx_log),&err);
    if(NULL == g_spx_log){
        return err;
    }
    g_spx_log->size =size;
    g_spx_log->level = level;
    g_spx_log->log = log;
    g_spx_log->path = spx_string_dup(path,&err);
    if(NULL == g_spx_log->path){
        goto r1;
    }
    g_spx_log->name = spx_string_dup(name,&err);
    if(NULL == g_spx_log->name){
        goto r1;
    }
    g_spx_log->mlock = spx_thread_mutex_new(log,&err);
    if(NULL == g_spx_log->mlock){
        goto r1;
    }

    g_spx_log->fp = logf_create(log,g_spx_log->path,g_spx_log->name,
            g_spx_log->size,&err);
    if(NULL == g_spx_log->fp){
        goto r1;
    }

    return err;
r1:
    if(NULL != g_spx_log->path) {
        SpxStringFree(g_spx_log->path);
    }
    if(NULL != g_spx_log->name) {
        SpxStringFree(g_spx_log->name);
    }
    if(NULL != g_spx_log->mlock){
        pthread_mutex_destroy(g_spx_log->mlock);
        SpxFree(g_spx_log->mlock);
    }
    if(NULL != g_spx_log->fp){
        fclose(g_spx_log->fp);
    }
    SpxFree(g_spx_log);
    return err;
}/*}}}*/

void spx_log(int level,char *fmt,...){/*{{{*/
    if(NULL == g_spx_log
            ||   level < 0
            || level < g_spx_log->level
            || NULL == fmt
      ) {
        return;
    }

    err_t err = 0;
    va_list ap;
    va_start(ap,fmt);
    string_t line = get_log_line(&err,level,fmt,ap);
    va_end(ap);
    if(NULL == line){
        return;
    }

    if(NULL == g_spx_log || 0 == g_spx_log->fp ){
        fprintf(stdout,"%s",SpxString2Char2(line));
        SpxStringFree(line);
        return;
    }

    if(level < g_spx_log->level){
        SpxStringFree(line);
        return;
    }

    size_t s = spx_string_len(line);
    if(0 == pthread_mutex_lock(g_spx_log->mlock)){
        do{
            if(g_spx_log->offset + s > g_spx_log->size){
                logf_close(g_spx_log);
                g_spx_log->fp = logf_create(g_spx_log->log,
                        g_spx_log->path,g_spx_log->name,
                        g_spx_log->size,&err);
                if(NULL == g_spx_log->fp){
                    break;
                }
            }
            fwrite(line,s,sizeof(char),g_spx_log->fp);
            g_spx_log->offset += s;
        }while(false);
        pthread_mutex_unlock(g_spx_log->mlock);
    }
    SpxStringFree(line);
    return;
}/*}}}*/

void spx_log_free(){
    logf_close();
    if(NULL != g_spx_log->path)
        SpxStringFree(g_spx_log->path);
    if(NULL != g_spx_log->name)
        SpxStringFree(g_spx_log->name);
    if(NULL != g_spx_log->mlock){
        pthread_mutex_destroy(g_spx_log->mlock);
        SpxFree(g_spx_log->mlock);
    }
    SpxFree(g_spx_log);
}

spx_private FILE *logf_create(SpxLogDelegate log,\
        const string_t path,\
        const string_t name,u64_t max_size,\
        err_t *err){/*{{{*/
    if(SpxStringIsNullOrEmpty(path)
            || SpxStringIsNullOrEmpty(name)){
        *err =  EINVAL;
        return NULL;
    }
    string_t fp = NULL;
    string_t newfp = NULL;
    bool_t is_dir = false;
    FILE *f = NULL;
    is_dir = spx_is_dir(path,err);
    if(0 != *err){
        return NULL;
    }
    if(!is_dir){
        if(0 != (*err = spx_mkdir(log,path,SpxPathMode))){
            return NULL;
        }
    }
    fp = spx_string_empty(err);
    if(NULL == fp){
        return NULL;
    }
    struct spx_datetime dt;
    SpxZero(dt);
    spx_get_curr_datetime(&dt);
    newfp = spx_string_cat_printf(err,fp,\
            "%s%s%s%-4d%02d%02d-%02d%02d%02d.log",\
            SpxString2Char1(path),\
            SpxStringEndWith(path,SpxPathDlmt) ? "" : SpxPathDlmtString,\
            name,\
            SpxYear(&dt),SpxMonth(&dt),SpxDay(&dt),\
            SpxHour(&dt),SpxMinute(&dt),SpxSecond(&dt));
    if(NULL == newfp){
        goto r1;
    }
    fp = newfp;
    f = fopen(SpxString2Char2(fp),"a+");
    if(NULL == f){
        *err = 0 == errno ? EACCES : errno;
        goto r1;
    }
    setlinebuf(f);
    if(NULL != newfp){
        SpxStringFree(newfp);
    } else {
        if(NULL != fp){
            SpxStringFree(fp);
        }
    }
    return f;

r1:
    if(NULL != newfp){
        SpxStringFree(newfp);
    } else {
        if(NULL != fp){
            SpxStringFree(fp);
        }
    }
    if(NULL != f){
        fclose(f);
    }
    return NULL;
}/*}}}*/

spx_private spx_inline string_t get_log_line(err_t *err,\
        u8_t level,char *fmt,va_list ap){/*{{{*/
    string_t line = spx_string_newlen(SpxLogDesc[level],SpxLogDescSize[level],err);
    if(NULL == line){
        return NULL;
    }

    struct spx_datetime dt;
    SpxZero(dt);
    spx_get_curr_datetime(&dt);

    string_t newline = spx_string_cat_printf(err,line,"%04d-%02d-%02d %02d:%02d:%02d.",
            dt.d.year,dt.d.month,dt.d.day,dt.t.hour,dt.t.min,dt.t.sec);
    if(NULL == newline){
        SpxStringFree(line);
        return NULL;
    }
    line = newline;
    newline = spx_string_cat_vprintf(err,line,fmt,ap);
    if(NULL == newline){
        SpxStringFree(line);
        return NULL;
    }
    line = newline;
    newline = spx_string_cat(line,SpxLineEndDlmtString,err);
    if(NULL == newline){
        SpxStringFree(line);
        return NULL;
    }
    line = newline;
    return line;
}/*}}}*/

spx_private spx_inline void logf_close(){
    if(NULL != g_spx_log->fp){
        fflush(g_spx_log->fp);
        fclose(g_spx_log->fp);
    }
    g_spx_log->offset = 0;
}

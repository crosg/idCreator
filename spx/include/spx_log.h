#ifndef SPX_LOG_H
#define SPX_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "spx_types.h"
#include "spx_string.h"
#include "spx_defs.h"

    //the macro to string,it will use to debug
#define spx_m2s_d(m) #m
#define spx_m2s(m) spx_m2s_d(m)

    struct spx_log{
        off_t offset;
        FILE *fp;
        size_t size;
        u8_t level;
        string_t path;
        string_t name;
        SpxLogDelegate *log;
        pthread_mutex_t *mlock;
    };

    err_t spx_log_new(SpxLogDelegate log,\
            const string_t path,\
            const string_t name,const u64_t max_size,\
            const u8_t level);
    void spx_log(int level,char *fmt,...);
    void spx_log_free();

#ifdef __cplusplus
}
#endif
#endif

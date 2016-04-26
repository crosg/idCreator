/*
 * =====================================================================================
 *
 *       Filename:  spx_time.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2014/05/20 10时17分06秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <utime.h>

#include "spx_types.h"
#include "spx_defs.h"
#include "spx_time.h"
#include "spx_string.h"
#include "spx_alloc.h"

void spx_get_curr_datetime(struct spx_datetime *dt){
    time_t timep;
    struct tm *p;
    time(&timep);
    p=localtime(&timep);
    dt->d.year = 1900 + p->tm_year;
    dt->d.month = 1 + p->tm_mon;
    dt->d.day = p->tm_mday;
    dt->t.hour = p->tm_hour;
    dt->t.min = p->tm_min;
    dt->t.sec = p->tm_sec;
}

struct spx_date *spx_get_today(struct spx_date *d){
    time_t timep;
    struct tm *p;
    time(&timep);
    p=localtime(&timep);
    d->year = 1900 + p->tm_year;
    d->month = 1 + p->tm_mon;
    d->day = p->tm_mday;
    return d;
}

time_t spx_now() {/*{{{*/
    time_t timep;
    struct tm *p;
    time(&timep);
    p = localtime(&timep);
    timep = mktime(p);
    return timep;
} /*}}}*/

u64_t spx_now_usec(){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_usec;
}


time_t spx_zero(struct spx_date *d){
    time_t timep;
    struct tm p;
    SpxZero(p);
    p.tm_year = d->year - 1900;
    p.tm_mon = d->month - 1;
    p.tm_mday = d->day;
    timep = mktime(&p);
    return timep;
}


time_t spx_mktime(struct spx_datetime *dt){
    time_t timep;
    struct tm p;
    SpxZero(p);
    p.tm_year = SpxYear(dt) - 1900;
    p.tm_mon = SpxMonth(dt) - 1;
    p.tm_mday = SpxDay(dt);
    p.tm_hour = SpxHour(dt);
    p.tm_min = SpxMinute(dt);
    p.tm_sec = SpxSecond(dt);
    timep = mktime(&p);
    return timep;
}

time_t spx_mkzero(struct spx_date *dt){
    time_t timep;
    struct tm p;
    SpxZero(p);
    p.tm_year = dt->year - 1900;
    p.tm_mon = dt->month - 1;
    p.tm_mday = dt->day;
    timep = mktime(&p);
    return timep;
}

struct spx_datetime *spx_datetime_dup(struct spx_datetime *dt,err_t *err){
    if(NULL == dt){
        *err = EINVAL;
        return NULL;
    }
    struct spx_datetime *new = spx_alloc_alone(sizeof(*new),err);
    if(NULL == new){
        return NULL;
    }
    memcpy(new,dt,sizeof(*new));
    return new;
}

struct spx_datetime *spx_get_datetime(time_t *t,struct spx_datetime *dt){
    struct tm *p;
    p=localtime(t);
    dt->d.year = 1900 + p->tm_year;
    dt->d.month = 1 + p->tm_mon;
    dt->d.day = p->tm_mday;
    dt->t.hour = p->tm_hour;
    dt->t.min = p->tm_min;
    dt->t.sec = p->tm_sec;
    return dt;
}

struct spx_date *spx_get_date(time_t *t,struct spx_date *d){
    struct tm *p;
    p=localtime(t);
    d->year = 1900 + p->tm_year;
    d->month = 1 + p->tm_mon;
    d->day = p->tm_mday;
    return d;
}

struct spx_datetime *spx_datetime_add_days(struct spx_datetime *dt,int days){
    time_t secs = spx_mktime(dt);
    secs += days * 24 * 3600;
    dt = spx_get_datetime(&secs,dt);
    return dt;
}

struct spx_date *spx_date_add(struct spx_date *d,int days){
    time_t secs = spx_zero(d);
    secs += days * SpxSecondsOfDay;
    d = spx_get_date(&secs,d);
    return d;

}

bool_t spx_date_is_before(struct spx_date *d){
    struct spx_date today;
    SpxZero(today);
    spx_get_today(&today);
    time_t zero = spx_mkzero(&today);
    time_t dzero = spx_mkzero(d);
    return zero > dzero;
}

bool_t spx_date_is_after(struct spx_date *d){
    struct spx_date today;
    SpxZero(today);
    spx_get_today(&today);
    time_t zero = spx_mkzero(&today);
    time_t dzero = spx_mkzero(d);
    return zero < dzero;
}

bool_t spx_date_is_today(struct spx_date *d){
    struct spx_date today;
    SpxZero(today);
    spx_get_today(&today);
    time_t zero = spx_mkzero(&today);
    time_t dzero = spx_mkzero(d);
    return zero = dzero;
}

int spx_date_cmp(struct spx_date *d1,struct spx_date *d2){
    time_t dz1 = spx_mkzero(d1);
    time_t dz2 = spx_mkzero(d2);
    if(dz1 < dz2) return -1;
    if(dz1 == dz2) return 0;
    else return 1;
}

int spx_datetime_cmp(struct spx_datetime *dt1,struct spx_datetime *dt2){
    time_t dz1 = spx_mktime(dt1);
    time_t dz2 = spx_mktime(dt2);
    if(dz1 < dz2) return -1;
    if(dz1 == dz2) return 0;
    else return 1;
}

struct spx_datetime_context{
    int start;
    int count;
    char op;
};

/*
 * fmt:yyyy-MM-dd
 */
struct spx_date *spx_date_convert(SpxLogDelegate *log,
        struct spx_date *d,
        string_t s,char *fmt,err_t *err){
    struct spx_datetime dt;
    SpxZero(dt);
    struct spx_datetime *dtnew = spx_datetime_convert(log,&dt,s,fmt,err);
    if(NULL == dtnew){
        SpxLogFmt2(log,SpxLogError,*err,
                "parset date from %s is fail.",
                s);
        return NULL;
    }
    memcpy(d,&(dt.d),sizeof(*d));
    return d;
}

/*
 * fmt:hh:mm:ss
 */
struct spx_time *spx_time_convert(SpxLogDelegate *log,
        struct spx_time *t,
        string_t s,char *fmt,err_t *err){
    struct spx_datetime dt;
    SpxZero(dt);
    struct spx_datetime *dtnew = spx_datetime_convert(log,&dt,s,fmt,err);
    if(NULL == dtnew){
        SpxLogFmt2(log,SpxLogError,*err,
                "parset time from %s is fail.",
                s);
        return NULL;
    }
    memcpy(t,&(dt.t),sizeof(*t));
    return t;
}

struct spx_datetime *spx_datetime_convert(SpxLogDelegate *log,
        struct spx_datetime *dt,
        string_t s,char *fmt,err_t *err
        ){/*{{{*/
    struct spx_datetime_context sdtc[6];
    char op = 0;
    int idx = -1;
    char lastop = 0;
    int start = 0 ;
    while('\0' != (op == *fmt)){
        switch(op){
            case 'y':
            case 'Y':
                {
                    if(op != lastop) {
                        idx++;
                        sdtc[idx].op = 'y';
                        sdtc[idx].start = start;
                    }
                    sdtc[idx].count++;
                    start++;
                    lastop = 'y';
                    break;
                }
            case 'M':
                {
                    if(op != lastop) {
                        idx++;
                        sdtc[idx].op = op;
                        sdtc[idx].start = start;
                    }
                    sdtc[idx].count++;
                    start++;
                    lastop = op;
                    break;
                }
            case 'd':
            case 'D':
                {
                    if(op != lastop) {
                        idx++;
                        sdtc[idx].op = 'd';
                        sdtc[idx].start = start;
                    }
                    sdtc[idx].count++;
                    start++;
                    lastop = 'd';
                    break;
                }
            case 'h':
            case 'H':
                {
                    if(op != lastop) {
                        idx++;
                        sdtc[idx].op = 'h';
                        sdtc[idx].start = start;
                    }
                    sdtc[idx].count++;
                    start++;
                    lastop = 'h';
                    break;
                }
            case 'm':
                {
                    if(op != lastop) {
                        idx++;
                        sdtc[idx].op = op;
                        sdtc[idx].start = start;
                    }
                    sdtc[idx].count++;
                    start++;
                    lastop = op;
                    break;
                }
            case 's':
            case 'S':
                {
                    if(op != lastop) {
                        idx++;
                        sdtc[idx].op = 's';
                        sdtc[idx].start = start;
                    }
                    sdtc[idx].count++;
                    start++;
                    lastop = 's';
                    break;
                }
            default:{
                        lastop = op;
                        start++;
                    }
        }
    }

    SpxZero(sdtc);
    int i = 0;
    for( ; i <= idx; i++){
        switch(sdtc[i].op) {
            case 'y':
                {
                    int end = sdtc[i].start + sdtc[i].count;
                    string_t year = spx_string_range_new(s,sdtc[i].start,end,err);
                    if(NULL == year){
                        SpxLogFmt2(log,SpxLogError,*err,
                                "parser year from string:%s begin:%d end %d is fail.",
                                s,sdtc[i].start,end);
                        return NULL;
                    }
                    dt->d.year = atoi(year);
                    SpxStringFree(year);
                    return NULL;
                }
            case 'M':
                {
                    int end = sdtc[i].start + sdtc[i].count;
                    string_t month = spx_string_range_new(s,sdtc[i].start,end,err);
                    if(NULL == month){
                        SpxLogFmt2(log,SpxLogError,*err,
                                "parser month from string:%s begin:%d end %d is fail.",
                                s,sdtc[i].start,end);
                        return NULL;
                    }
                    dt->d.month = atoi(month);
                    SpxStringFree(month);
                    return NULL;
                }
            case 'd':
                {
                    int end = sdtc[i].start + sdtc[i].count;
                    string_t day = spx_string_range_new(s,sdtc[i].start,end,err);
                    if(NULL == day){
                        SpxLogFmt2(log,SpxLogError,*err,
                                "parser day from string:%s begin:%d end %d is fail.",
                                s,sdtc[i].start,end);
                        return NULL;
                    }
                    dt->d.day = atoi(day);
                    SpxStringFree(day);
                    break;
                }
            case 'h':
                {
                    int end = sdtc[i].start + sdtc[i].count;
                    string_t hour = spx_string_range_new(s,sdtc[i].start,end,err);
                    if(NULL == hour){
                        SpxLogFmt2(log,SpxLogError,*err,
                                "parser hour from string:%s begin:%d end %d is fail.",
                                s,sdtc[i].start,end);
                        return NULL;
                    }
                    dt->t.hour = atoi(hour);
                    SpxStringFree(hour);
                    break;
                }
            case 'm':
                {
                    int end = sdtc[i].start + sdtc[i].count;
                    string_t min = spx_string_range_new(s,sdtc[i].start,end,err);
                    if(NULL == min){
                        SpxLogFmt2(log,SpxLogError,*err,
                                "parser minute from string:%s begin:%d end %d is fail.",
                                s,sdtc[i].start,end);
                        return NULL;
                    }
                    dt->t.min = atoi(min);
                    SpxStringFree(min);
                    break;
                }
            case 's':
                {
                    int end = sdtc[i].start + sdtc[i].count;
                    string_t sec = spx_string_range_new(s,sdtc[i].start,end,err);
                    if(NULL == sec){
                        SpxLogFmt2(log,SpxLogError,*err,
                                "parser second from string:%s begin:%d end %d is fail.",
                                s,sdtc[i].start,end);
                        return NULL;
                    }
                    dt->t.sec = atoi(sec);
                    SpxStringFree(sec);
                    break;
                }
        }
    }
    return dt;
}/*}}}*/


err_t spx_modify_filetime(
        const string_t fname,
        u64_t secs
        ){
    struct timeval val[2];
    memset(&val,0,2 * sizeof(struct timeval));
    val[0].tv_sec = secs;
    val[1].tv_sec = secs;
    if(0 != utimes(fname,val)){
        return errno;
    }
    return 0;
}

time_t spx_get_token(){
    time_t now =  spx_now();
    struct spx_datetime dt;
    memset(&dt,0,sizeof(dt));
    SpxYear(&dt) = 2015;
    SpxMonth(&dt) = 1;
    SpxDay(&dt) = 1;
    SpxHour(&dt)  = 0;
    SpxMinute(&dt)  = 0;
    SpxSecond(&dt) = 0;
    time_t base =  spx_mktime(&dt);
    return now -base;
}

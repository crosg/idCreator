/*
 * =====================================================================================
 *
 *       Filename:  spx_time.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2014/05/20 10时20分41秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */

#ifndef SPX_TIME_H
#define SPX_TIME_H
#if __cplusplus
extern "C" {
#endif

#include "spx_types.h"

#define SpxYear(dt) ((dt)->d.year)
#define SpxMonth(dt) ((dt)->d.month)
#define SpxDay(dt) ((dt)->d.day)
#define SpxHour(dt) ((dt)->t.hour)
#define SpxMinute(dt) ((dt)->t.min)
#define SpxSecond(dt) ((dt)->t.sec)



    void spx_get_curr_datetime(struct spx_datetime *dt);
    struct spx_date *spx_get_today(struct spx_date *d);
    time_t spx_now();
    u64_t spx_now_usec();
    time_t spx_mktime(struct spx_datetime *dt);
    struct spx_datetime *spx_datetime_dup(struct spx_datetime *dt,err_t *err);
    struct spx_datetime *spx_datetime_add_days(struct spx_datetime *dt,int days);
    struct spx_datetime *spx_get_datetime(time_t *t,struct spx_datetime *dt);
    struct spx_date *spx_get_date(time_t *t,struct spx_date *d);
    struct spx_date *spx_get_today(struct spx_date *d);
    time_t spx_zero(struct spx_date *d);
    struct spx_date *spx_date_add(struct spx_date *d,int days);
    bool_t spx_date_is_before(struct spx_date *d);
    bool_t spx_date_is_after(struct spx_date *d);
    bool_t spx_date_is_today(struct spx_date *d);
    /* *
     * cmp d1 and d2
     * if d1 < d2   return -1
     * if d1 == d2 return 0
     * if d1 > d2 return 1
     * notes:base of cmper is 00:00:00
     * */
    int spx_date_cmp(struct spx_date *d1,struct spx_date *d2);

    /* *
     * cmp dt1 and dt2
     * if d1 < d2   return -1
     * if d1 == d2 return 0
     * if d1 > d2 return 1
     * */
    int spx_datetime_cmp(struct spx_datetime *dt1,struct spx_datetime *dt2);

    struct spx_date *spx_date_convert(SpxLogDelegate *log,
            struct spx_date *d,
            string_t s,char *fmt,err_t *err);

    struct spx_time *spx_time_convert(SpxLogDelegate *log,
            struct spx_time *t,
            string_t s,char *fmt,err_t *err);

    struct spx_datetime *spx_datetime_convert(SpxLogDelegate *log,
            struct spx_datetime *dt,
            string_t s,char *fmt,err_t *err);

    err_t spx_modify_filetime(
            const string_t fname,
            u64_t secs
            );
    time_t spx_get_token();

#if __cplusplus
}
#endif
#endif

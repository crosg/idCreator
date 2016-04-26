#ifndef SPX_TYPES_H
#define SPX_TYPES_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <stddef.h>
#include <wchar.h>

#ifndef bool_t
    typedef enum {
        false = 0,
        true = 1
    }bool_t;
#endif


#ifndef byte_t
    typedef signed char byte_t;
#endif

#ifndef ubyte_t
    typedef unsigned char ubyte_t;
#endif

#ifndef uchar_t
    typedef unsigned char uchar_t;
#endif

#ifndef bstring_t
    typedef struct {
        size_t s;
        uchar_t v[0];
    }bstring_t;
#endif

    //i think we do not need to define the struct for string
    //and string not must need length in the struct
    //but i think we must have the sign('\0') of the string end
#ifndef string_t
    typedef char * string_t;
#endif

#ifndef wstring_t
    typedef wchar_t * wstring_t;
#endif

#ifndef u64_t
    typedef u_int64_t u64_t;
#endif

#ifndef u32_t
    typedef u_int32_t u32_t;
#endif

#ifndef u16_t
    typedef u_int16_t u16_t;
#endif

#ifndef u8_t
    typedef u_int8_t u8_t;
#endif

#ifndef i64_t
    typedef int64_t i64_t;
#endif

#ifndef i32_t
    typedef int32_t i32_t;
#endif

#ifndef i16_t
    typedef int16_t i16_t;
#endif

#ifndef i8_t
    typedef int8_t i8_t;
#endif

#ifndef SpxLogDelegate
    typedef void (SpxLogDelegate)(int level,string_t fmt,...);
#endif

#ifndef ptr_t
    typedef char * ptr_t;
#endif

#ifndef spx_atomic_t
    typedef long spx_atomic_t;
#endif

#ifndef spx_uatomit_t
    typedef unsigned long spx_uatomic_t;
#endif

#ifndef err_t
#define err_t int
#endif

#ifndef spx_inline
#define spx_inline inline
#endif

#ifndef spx_private
#define spx_private static
#endif

#ifndef spx_public
#define spx_public
#endif

    struct spx_host{
        string_t ip;
        int port;
    };

    struct spx_date{
        int year;
        int month;
        int day;
    };

    struct spx_time{
        int hour;
        int min;
        int sec;
    };

    struct spx_datetime{
        struct spx_date d;
        struct spx_time t;
    };

    spx_private char *spx_bool_desc[] = {
        "false",
        "true"
    };




#ifndef spx_res_t
    typedef struct spx_res{
        int t;
        union{
            int fd;
            struct{
                int fd;
                char *ptr;
            }mmap;
            FILE *fp;
        }r;
    }spx_res_t;
#define SpxFd(r) (r->r.file.fd)
#define SpxMapFd(r) (r->r.mmap.fd)
#define SpxMap(r) (r->r.mmap.ptr)
#define SpxFILE(r) (r->r.fp);
#endif


#ifdef __cplusplus
}
#endif
#endif


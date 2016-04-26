
#ifndef SPX_DEFS_H
#define SPX_DEFS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <unistd.h>

#include "spx_types.h"
#include "spx_errno.h"

#define SpxLogDebug 0
#define SpxLogInfo 1
#define SpxLogWarn 2
#define SpxLogError 3
#define SpxLogMark 4


spx_private char *SpxLogDesc[] = {
    "Debug.",
    "Info.",
    "Warn.",
    "Error.",
    "Mark."
};

 spx_private int SpxLogDescSize[] = {
    6,
    5,
    5,
    6,
    5
};


#define SpxLogStack(log,level) \
    if(NULL != (log)) {\
        (log)(level,((string_t) "File:%s,Line:%d,Func:%s.function be called."), \
                __FILE__,__LINE__,__FUNCTION__); \
    }


#define SpxLog1(log,level,info) \
    if(NULL != (log)) {\
        (log)(level,((string_t) "File:%s,Line:%d,Func:%s.%s."), \
                __FILE__,__LINE__,__FUNCTION__,info); \
    }

#define SpxLog2(log,level,err,info) \
    if(NULL != (log)) {\
        (log)(level,((string_t) "File:%s,Line:%d,Func:%s.errno:%d,info:%s.%s."),\
                __FILE__,__LINE__,__FUNCTION__,err,err >= SpxSuccess ?  spx_strerror(err) : strerror(err),info);\
    }

#define SpxLogFmt1(log,level,fmt,...) \
    if(NULL != (log)) {\
        (log)(level,((string_t) "File:%s,Line:%d,Func:%s."fmt),\
                __FILE__,__LINE__,__FUNCTION__,__VA_ARGS__);\
    }

#define SpxLogFmt2(log,level,err,fmt,...) \
    if(NULL != (log)) {\
        (log)(level,((string_t) "File:%s,Line:%d,Func:%s.errno:%d,info:%s."fmt),\
                __FILE__,__LINE__,__FUNCTION__,err,err >= SpxSuccess ? spx_strerror(err) : strerror(err),__VA_ARGS__);\
    }

#define SpxErrReset errno = 0
#define SpxErr1 (0 != errno)
#define SpxErr2(rc) (0 != (rc = errno))

#define SpxPathMode 0777
#define SpxFileMode 0777
#define SpxPathSize 1023
#define SpxFileNameSize 127
#define SpxPathDlmt '/'
#define SpxSuffixDlmt '.'
#define SpxSuffixDlmtString "."
#define SpxSuffixDlmtLen 1
#define SpxPathDlmtLen 1
#define SpxPathDlmtString "/"
#define SpxLineSize 2047
#define SpxLineEndDlmtString "\n"
#define SpxLineEndDlmt '\n'
#define SpxLineEndDlmtLen 1
#define SpxKeyStringSize 255
#define SpxHostNameSize 255

#define SpxGB (1024 * 1024 * 1024)
#define SpxMB (1024 * 1024)
#define SpxKB (1024)

#define SpxSecondTick 1
#define SpxMinuteTick 60
#define SpxHourTick (60 * 60)
#define SpxDayTick (24 * 60 * 60)

#define SpxDiskUnitPB 0
#define SpxDiskUnitTB 1
#define SpxDiskUnitGB 2
#define SpxDiskUnitMB 3
#define SpxDiskUnitKB 4
#define SpxDiskUnitB 5
#define SpxSecondsOfDay (24 * 60 * 60)
#define SpxBoolTransportSize (sizeof(char))

 spx_private char *spx_diskunit_desc[]={
    "PB",
    "TB",
    "GB",
    "MB",
    "KB",
    "B"
};

#define SpxIpv4Size 15

#define SpxMin(a,b) ((a) < (b) ? (a) : (b))
#define SpxMax(a,b) ((a) > (b) ? (a) : (b))
#define SpxAbs(a) ((a) < 0 ? -(a) : a)

#ifdef Spx64
#define SpxPtrSize 8
#elif Spx32
#define SpxPtrSize 4
#else
#define SpxPtrSize 4
#endif

//#define SpxI32Size   (sizeof("-2147483648") - 1)
//#define SpxI64Size   (sizeof("-9223372036854775808") - 1)

#define LF     (u_char) 10
#define CR     (u_char) 13
#define CRLF   "\x0d\x0a"


//#if Spx32
//#define SpxIntSize   SpxI32Size
//#else
//#define SpxIntSize   SpxI64Size
//#endif

//#if (Spx64)
//#define SpxAtomicSize            (sizeof("-9223372036854775808") - 1)
//#else
//#define SpxAtomicSize            (sizeof("-2147483648") - 1)
//#endif

#define SpxTypeConvert(tp,name,old) tp name = (tp) (old)
#define SpxTypeConvert2(t,newp,old) t *newp = (t *) (old)


#define SpxClose(fd)  \
    do { \
        if(0 != fd) { \
            close(fd);\
            fd = 0;\
        } \
    }while(false)


#define SpxAlign(d, a)     (((d) + (a - 1)) & ~(a - 1))

#if Spx32
#define SpxAlignSize 4
#elif Spx64
#define SpxAlignSize 8
#else
#define SpxAlignSize 8
#endif

#define SpxMemIncr(p,s) (((char *) p) + (s))
#define SpxMemDecr(p,s) (((char *) p) - (s))
#define SpxPtrDecr(p1,p2) ((size_t ) (((char *) p1) - ((char *) p2)))
#define SpxRightMovePointer(p,s) (((char *) p) + (s))
#define SpxLeftMovePointer(p,s) (((char *) p) - (s))

#define SpxSSet(o,p,v) o->p = (v)

#ifdef __cplusplus
}
#endif
#endif

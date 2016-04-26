
#ifndef SPX_STDF_H
#define SPX_STDF_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "spx_types.h"

    struct spx_stdf{
        FILE *p;
        bool_t isbry;
        bool_t is_stp;
        uchar_t stp;
        size_t max;
        off_t offset;
        enum spx_fcs{
            Line,
            Buf,
            Size
        } cs;
    };
#ifdef __cplusplus
}
#endif
#endif

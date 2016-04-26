
#ifndef SPX_ERRNO_H
#define SPX_ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <string.h>

#include "spx_types.h"

#define SpxSuccess 512
#define ENODLMT 513
#define EBADHEADER 514

char *spx_strerror(err_t err);


#ifdef __cplusplus
}
#endif
#endif

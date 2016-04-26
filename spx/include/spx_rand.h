/*
 * =====================================================================================
 *
 * this software or lib may be copied only under the terms of the gnu general
 * public license v3, which may be found in the source kit.
 *
 *       Filename:  spx_rand.h
 *        Created:  2014/07/15 11时49分05秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 * =====================================================================================
 */
#ifndef _SPX_RAND_H_
#define _SPX_RAND_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>

#include "spx_types.h"

u32_t spx_random(u32_t idx);
u32_t spx_srandom();


#ifdef __cplusplus
}
#endif
#endif

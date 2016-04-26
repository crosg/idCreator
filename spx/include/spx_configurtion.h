/*
 * =====================================================================================
 *
 * this software or lib may be copied only under the terms of the gnu general
 * public license v3, which may be found in the source kit.
 *
 *       Filename:  spx_configurtion.h
 *        Created:  2014/07/16 16时38分03秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 * =====================================================================================
 */
#ifndef _SPX_CONFIGURTION_H_
#define _SPX_CONFIGURTION_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>

#include "spx_types.h"

    typedef void SpxConfigurtionLineParserDelegate(string_t line,\
            void *config,err_t *err);
    typedef void *SpxConfigurtionParserBeforeDelegate(SpxLogDelegate *log, err_t *err);
    typedef err_t SpxConfigurtionParserAfterDelegate(void *config);

    void *spx_configurtion_parser(SpxLogDelegate *log,\
            SpxConfigurtionParserBeforeDelegate *before,\
            SpxConfigurtionParserAfterDelegate *after,\
            string_t filename,\
            SpxConfigurtionLineParserDelegate *lineparser,\
            err_t *err);


#ifdef __cplusplus
}
#endif
#endif

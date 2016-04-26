#ifndef SPX_PATH_H
#define SPX_PATH_H
#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>

#include "spx_types.h"

#define SpxFileExist(filename) ( 0 == access(filename,F_OK))

bool_t spx_is_dir(const string_t path,err_t *err);
err_t spx_mkdir(SpxLogDelegate *log,const string_t path,const mode_t mode);
string_t spx_fullname(const string_t path,const string_t filename,\
        err_t *err);
string_t spx_basepath(const string_t path,err_t *err);
string_t spx_file_suffix(string_t fname,err_t *err);

u64_t spx_mountpoint_availsize(string_t path,err_t *err);
u64_t spx_mountpoint_totalsize(string_t path,err_t *err);

#ifdef __splusplus
}
#endif
#endif

#ifndef _SPX_SOCKET_ACCEPT_H_
#define _SPX_SOCKET_ACCEPT_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <ev.h>

#include "spx_types.h"

//void spx_socket_accept_nb(SpxLogDelegate *log,int fd);
void spx_socket_accept_nb(SpxLogDelegate *log,
        struct ev_loop *loop,int fd);

#ifdef __cplusplus
}
#endif
#endif

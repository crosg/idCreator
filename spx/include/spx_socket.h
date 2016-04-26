/*
 * =====================================================================================
 *
 *       Filename:  spx_socket.h
 *
 *    Description:
 *
 *
 *        Version:  1.0
 *        Created:  2014/06/11 14时33分39秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#ifndef _SPX_SOCKET_H_
#define _SPX_SOCKET_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "spx_types.h"



#define SpxKeepAlive true
#define SpxAliveTimeout 30
#define SpxDetectTimes 3
#define SpxDetectTimeout 30
#define SpxLinger false
#define SpxLingerTimeout 0
#define SpxNodelay true

int spx_socket_new(err_t *err);
//void spx_socket_accept_nb(int fd);
err_t spx_socket_start(const int fd,\
        string_t ip,const int port,\
        bool_t is_keepalive,size_t alive_timeout,\
        size_t detect_times,size_t detect_timeout,\
        bool_t is_linger,size_t linger_timeout,\
        bool_t is_nodelay,\
        bool_t is_timeout,size_t timeout,\
        size_t listens);
string_t spx_ip_get(int sock,err_t *err);

string_t spx_host_tostring(struct spx_host *host,err_t *err);

err_t spx_socket_set(const int fd,\
        bool_t is_keepalive,size_t alive_timeout,\
        size_t detect_times,size_t detect_timeout,\
        bool_t is_linger,size_t linger_timeout,\
        bool_t is_nodelay,\
        bool_t is_timeout,size_t timeout);
err_t spx_socket_connect(int fd,string_t ip,int port);
err_t spx_socket_connect_nb(int fd,string_t ip,int port,u32_t timeout);
string_t spx_socket_getipbyname(string_t name,err_t *err);
bool_t spx_socket_is_ip(string_t ip);
err_t spx_tcp_nodelay(int fd,bool_t enable);
bool_t spx_socket_ready_read(int fd,u32_t timeout);
bool_t spx_socket_read_timeout(int fd,u32_t timeout);
bool_t spx_socket_test(int fd);

#ifdef __cplusplus
}
#endif
#endif

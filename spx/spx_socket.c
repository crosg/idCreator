/*
 * =====================================================================================
 *
 *       Filename:  spx_socket.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2014/06/11 14时33分30秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "spx_types.h"
#include "spx_string.h"
#include "spx_defs.h"
#include "spx_socket.h"
#include "spx_notifier_module.h"
#include "spx_module.h"

spx_private err_t spx_socket_reuseaddr(int sock) ;
spx_private err_t spx_socket_keepalive(int fd,bool_t enable,\
        size_t alive_timeout,size_t detect_times,size_t detect_timeout);
spx_private err_t spx_socket_linger(const int fd,bool_t enable, size_t timeout);
spx_private err_t spx_socket_timout(int fd, size_t timeout);


spx_private err_t spx_socket_reuseaddr(int sock) {
	int optval = 1;
    if(0 != setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&optval,(socklen_t) sizeof(int))){
        return errno;
    }
    return 0;
}

spx_private err_t spx_socket_keepalive(int fd,bool_t enable,\
        size_t alive_timeout,size_t detect_times,size_t detect_timeout){

    err_t err = 0;
    if(!enable) return 0;
#ifdef SpxLinux
    //open the keepalive
    int optval = 1;
    if (-1 == setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval,
                (socklen_t) sizeof(int))) {
        err = errno;
        return err;
    }
    if (-1 == setsockopt(fd, SOL_TCP, TCP_KEEPIDLE, &alive_timeout,
                sizeof(size_t))) {
        err = errno;
        return err;
    }
    if (-1 == setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &detect_times,\
                sizeof(size_t))) {
        err = errno;
        return err;
    }
    if (-1 == setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &detect_timeout,\
                sizeof(size_t))) {
        err = errno;
        return err;
    }
#endif

#ifdef SpxMac
    //open the keepalive
    int optval = 1;
    if (-1 == setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval,
                (socklen_t) sizeof(int))) {
        err = errno;
        return err;
    }
#endif
    return err;
}


spx_private err_t spx_socket_linger(const int fd,bool_t enable, size_t timeout) {/*{{{*/
	struct linger optval;
	optval.l_onoff = enable;
	optval.l_linger = timeout;
	if (-1 == setsockopt(fd, SOL_SOCKET, SO_LINGER, &optval,\
					(socklen_t) sizeof(struct linger))) {
		return errno;
	}
	return 0;
}/*}}}*/


err_t spx_tcp_nodelay(int fd,bool_t enable) {/*{{{*/
#ifdef SpxLinux
	if (-1 == setsockopt(fd, SOL_TCP, TCP_NODELAY, &enable, sizeof(enable))) {
	    return errno;
	}
#endif
	return 0;
}/*}}}*/

spx_private err_t spx_socket_timout(int fd, size_t timeout) {/*{{{*/
	struct timeval waittime;
	waittime.tv_sec = timeout;
	waittime.tv_usec = 0;
	if (-1 == setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &waittime,
					sizeof(struct timeval))) {
		return errno;
	}
	if (-1 == setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &waittime,
					sizeof(struct timeval))) {
		return errno;
	}
	return 0;
}/*}}}*/


int spx_socket_new(err_t *err){
    int fd = 0;
    fd = socket(AF_INET, SOCK_STREAM,0);
    if(-1 == fd){
        *err = errno;
        return -1;
    }
    return fd;
}
err_t spx_socket_start(const int fd,\
        string_t ip,const int port,\
        bool_t is_keepalive,size_t alive_timeout,\
        size_t detect_times,size_t detect_timeout,\
        bool_t is_linger,size_t linger_timeout,\
        bool_t is_nodelay,\
        bool_t is_timeout,size_t timeout,\
        size_t listens){

    err_t err = 0;
    if(0 != (err = spx_socket_reuseaddr(fd))){
        return err;
    }

    if(0 != (err = spx_socket_keepalive(fd,is_keepalive,
                    alive_timeout,detect_times,detect_timeout))){
        return err;
    }
    if(0 != (err = spx_socket_linger(fd,is_linger,linger_timeout))){
        return err;
    }
    if(0 != (err = spx_tcp_nodelay(fd,is_nodelay))){
        return err;
    }
    if(is_timeout && (0 != (err = spx_socket_timout(fd,timeout)))){
        return err;
    }

    struct sockaddr_in addr;
    SpxZero(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if(SpxStringIsNullOrEmpty(ip)){
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    } else{
        inet_aton(ip,&(addr.sin_addr));
    }

    if(-1 == bind(fd, (struct sockaddr *) &addr,
                sizeof(struct sockaddr_in))) {
        err = errno;
        return err;
    }

	if (-1 == listen(fd, listens)) {
        return errno;
	}
    return 0;
}

string_t spx_ip_get(int sock,err_t *err) {

	string_t ip = spx_string_emptylen(SpxIpv4Size,err);
	if(NULL == ip){
        return NULL;
    }

	socklen_t len;
	struct sockaddr_in addr;
	len = sizeof(struct sockaddr_in);
	memset(&addr, 0, len);
	getpeername(sock, (struct sockaddr *) &addr, &len);

	char *tmp;
	tmp = inet_ntoa(addr.sin_addr);
	spx_string_cat(ip,tmp,err);
	if(0 != *err){
	    spx_string_free(ip);
        return NULL;
    }
	return ip;
}


string_t spx_host_tostring(struct spx_host *host,err_t *err){
    string_t shost = spx_string_empty(err);
    if(NULL == shost){
        return NULL;
    }

    string_t new = spx_string_cat_printf(err,shost,"%s:%d",host->ip,host->port);
    if(NULL == new){
        goto r1;
    }
    return new;

r1:
    spx_string_free(shost);
    return NULL;
}

err_t spx_socket_connect(int fd,string_t ip,int port){
    struct sockaddr_in addr;
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port=htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    if(0 > connect(fd,(struct sockaddr *) &addr,sizeof(addr))){
        return errno;
    }
    return 0;
}

err_t spx_socket_connect_nb(int fd,string_t ip,int port,u32_t timeout){
    struct sockaddr_in addr;
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port=htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    err_t err = 0;
    err_t rc = 0;
    if(0 > connect(fd,(struct sockaddr *) &addr,sizeof(addr))){
        //filter this errno,socket is not connect to server and return at once
        if (EINPROGRESS == errno) {
            struct timeval tv;
            SpxZero(tv);
            tv.tv_sec = timeout;
            tv.tv_usec = 0;
            fd_set frd;
            FD_ZERO(&frd);
            FD_SET(fd,&frd);
            socklen_t len = sizeof(err);
            if (0 < (rc = select (fd+1 , NULL,&frd,NULL,&tv))) {
                if(0 > getsockopt(fd,SOL_SOCKET,SO_ERROR,(void*)(&err),&len)) {
                    err = errno;
                    return err;
                }
                if (0 != err) {
                    return err;
                }
            } else if(0 == rc) {
                err = ETIMEDOUT;
                return err;
            } else {
                err = EXDEV;
                return err;
            }
            SpxErrReset;
            return 0;
        } else {
            return errno;
        }
    }
    return 0;
}

err_t spx_socket_set(const int fd,\
        bool_t is_keepalive,size_t alive_timeout,\
        size_t detect_times,size_t detect_timeout,\
        bool_t is_linger,size_t linger_timeout,\
        bool_t is_nodelay,\
        bool_t is_timeout,size_t timeout){
    err_t err = 0;
    if(0 != (err = spx_socket_reuseaddr(fd))){
        return err;
    }

    if(0 != (err = spx_socket_keepalive(fd,is_keepalive,
                    alive_timeout,detect_times,detect_timeout))){
        return err;
    }
    if(0 != (err = spx_socket_linger(fd,is_linger,linger_timeout))){
        return err;
    }
    if(0 != (err = spx_tcp_nodelay(fd,is_nodelay))){
        return err;
    }
    if(is_timeout && (0 != (err = spx_socket_timout(fd,timeout)))){
        return err;
    }

    return 0;
}

string_t spx_socket_getipbyname(string_t name,err_t *err){
    string_t hostname = NULL;
    string_t ip = NULL;
    if(NULL == name){
        hostname = spx_string_newlen(NULL,SpxHostNameSize,err);
        if(NULL == hostname){
            return NULL;
        }
        *err = gethostname(hostname,SpxHostNameSize);
        if(0 != *err){
            goto r1;
        }
    }else {
        hostname = name;
    }
    struct hostent *hosts = gethostbyname(hostname);
    if(NULL == hosts){
        *err = h_errno;
        goto r1;
    }
    struct in_addr addr;
    memcpy(&addr, hosts->h_addr_list[0], sizeof(struct in_addr));
    ip = spx_string_newlen(NULL,SpxIpv4Size,err);
    if(NULL == ip){
        goto r1;
    }
    string_t newip = spx_string_cat(ip,inet_ntoa(addr),err);
    if(SpxStringIsNullOrEmpty(newip)){
        spx_string_free(ip);
        goto r1;
    }
r1:
    if(NULL == name){
        spx_string_free(hostname);
    }
    return ip;
}

bool_t spx_socket_is_ip(string_t ip){
    struct sockaddr_in sa;
    int rc = inet_pton(AF_INET, ip, &(sa.sin_addr));
    return 1 == rc ? true : false;
}

bool_t spx_socket_read_timeout(int fd,u32_t timeout){
    struct timeval tv;
    SpxZero(tv);
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    fd_set frd;
    FD_ZERO(&frd);
    FD_SET(fd,&frd);
    int n = select (fd+1 , &frd,NULL,NULL,&tv);
    if(0 < n && FD_ISSET(fd,&frd)){
        return true;
    }
    return false;
}

bool_t spx_socket_ready_read(int fd,u32_t timeout){
    return spx_socket_read_timeout(fd,timeout);
}

bool_t spx_socket_test(int fd){
    struct tcp_info info;
    int len=sizeof(info);
    getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len);
    if((info.tcpi_state==TCP_ESTABLISHED))
        return true;
    return false;
}

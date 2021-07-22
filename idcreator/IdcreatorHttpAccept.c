/*************************************************************************
    > File Name: ydb_tracker_monitor.c
    > Author: shuaixiang
    > Mail:
    
    > Created Time: Mon 23 Nov 2015 09:31:53 AM UTC
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <string.h>
#include <strings.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <ev.h>
#include <ctype.h>

#include "spx_types.h"
#include "spx_properties.h"
#include "spx_defs.h"
#include "spx_string.h"
#include "spx_socket.h"
#include "spx_io.h"
#include "spx_alloc.h"
#include "spx_module.h"
#include "spx_network_module.h"
#include "spx_job.h"
#include "spx_time.h"
#include "spx_log.h"
#include "spx_collection.h"


#include "IdcreatorCore.h"
#include "IdcreatorHttpAccept.h"
#include "IdcreatorConfig.h"

#define ISspace(x) isspace((int)(x))
#define SERVER_STRING "Server: DFSHTTPD/0.1.0\r\n"
#define JSON_SIZE 1024
#define POOL_SIZE 1024
#define REQUEST_SIZE 1024
#define RETRY_TIMES 3
//#define PORT 8988
#define ELEMENT_SIZE 100
#define SPLIT_SIZE 1024

struct mainsocket_thread_arg{
    SpxLogDelegate *log;
    struct IdcreatorConfig *c;
};

struct storage_list{
    struct ydb_remote_storage *storage;
    struct storage_list *next_storage;
};

spx_private struct ev_loop *main_socket_loop = NULL;
spx_private ev_io monitor_watcher;
spx_private SpxLogDelegate *mtr_log = NULL;

spx_private struct monitor_http_context * mtr_ctx_top = NULL;

spx_private struct param_node* InsertParam(struct monitor_http_context* mtr_ctx, char *key, int key_len, char *val, int val_len);
spx_private struct param_node* GetParam(struct monitor_http_context *mtr_ctx, char *key);
spx_private err_t FreeParam(struct monitor_http_context *mtr_ctx);
spx_private void * CreatMainSocket(void *arg);
spx_private err_t NewCTXPool(int pool_size, struct IdcreatorConfig *c);
spx_private void FreeMonitorCTXPool();
spx_private void SendResponse(EV_P_ ev_async *watcher, int revents);
spx_private void RegisterAsyncWatcher(ev_async *watcher, void(*cb)(struct ev_loop *loop, ev_async *watcher, int revents), void *data);
spx_private void Reciver(struct ev_loop *loop, ev_io *watcher, int revents);
spx_private void Request(struct ev_loop *loop, ev_async *watcher, int revents);
spx_private int Request_GetRequest_ReadRequest_IsEnd(byte_t* buf, size_t len);
spx_private err_t Request_GetRequest_ReadRequest(int fd, byte_t *buf, size_t *len);
spx_private void Request_GetRequest(int revents, void *arg);
spx_private void ParserRequest(EV_P_ ev_async *watcher, int revents);
spx_private void bad_request(int client);
spx_private void headers(int client, int content_length);
spx_private void unimplemented(int client);
spx_private void ydb_tracker_monitor_socket_response(EV_P_ ev_async *watcher, int revents);
spx_private void RequestFinish(struct monitor_http_context *mtr_ctx);
spx_private void RequestException(struct monitor_http_context * mtr_ctx, void (*handle)(int client));
spx_private void CloseCTX(struct monitor_http_context *mtr_ctx);

spx_private struct param_node* InsertParam(struct monitor_http_context* mtr_ctx, char *key, int key_len, char *val, int val_len){/*{{{*/
    if(NULL == mtr_ctx){
        SpxLog1(mtr_log, SpxLogError, "InsertParam mtr_ctx is NULL");
        return NULL;
    }

    struct param_node * param = (struct param_node *) calloc(1, sizeof(struct param_node));
    if(NULL == param){
        SpxLog1(mtr_log, SpxLogError, "calloc param failed");
        return NULL;
    }

    strncpy(param->key, key, key_len);
    strncpy(param->value, val, val_len);
    param->next_param = NULL;

    if(NULL == mtr_ctx->param_tail){
        mtr_ctx->param_head = param;
        mtr_ctx->param_tail = param;
    }else{
        mtr_ctx->param_tail->next_param = param;
        mtr_ctx->param_tail = param;
    }

    return param;
}/*}}}*/

spx_private struct param_node* GetParam(struct monitor_http_context *mtr_ctx, char *key){/*{{{*/
    if(NULL == mtr_ctx){
        SpxLog1(mtr_log, SpxLogError, "GetParam mtr_ctx is NULL");
        return NULL;
    }

    struct param_node * p_node = mtr_ctx->param_head;

    while(NULL != p_node){
        if(0 == strncmp(p_node->key, key, strlen(key))){
            return p_node;
        }else{
            p_node = p_node->next_param;
        }
    }

    return NULL;
}/*}}}*/

spx_private err_t FreeParam(struct monitor_http_context *mtr_ctx){/*{{{*/
    err_t err = 0;

    if(NULL == mtr_ctx){
        SpxLog1(mtr_log, SpxLogError, "GetParam mtr_ctx is NULL");
        return errno;
    }

    struct param_node * p_node = mtr_ctx->param_head;

    while(NULL != p_node){
            struct param_node *tmp_node = p_node;
            p_node = p_node->next_param;
            free(tmp_node);
    }

    return err;
}/*}}}*/

spx_private err_t CTXInit(struct monitor_http_context * mtr_ctx, struct IdcreatorConfig *c){/*{{{*/
    err_t err = 0;
    if(NULL == mtr_ctx){
        SpxLog1(mtr_log, SpxLogError, "CTXInit mtr_ctx is NULL");
        return errno;
    }

    mtr_ctx->p_nextCTX = NULL;
    mtr_ctx->p_preCTX = NULL;
    mtr_ctx->fd = 0;
    mtr_ctx->timeout = 3.0;
    mtr_ctx->c = c;

    mtr_ctx->req_retry = 0;
    mtr_ctx->req_len = 0;
    mtr_ctx->req_size = 0;
    mtr_ctx->param_head = NULL;
    mtr_ctx->param_tail = NULL;
    mtr_ctx->request = (byte_t*)malloc(sizeof(char) * REQUEST_SIZE);

    mtr_ctx->resp_retry = 0;
    mtr_ctx->resp_len = 0;
    mtr_ctx->split_size = 0;
    mtr_ctx->response = NULL;

    if(NULL == mtr_ctx->request){
        SpxLog1(mtr_log, SpxLogError, "mtr_ctx->request is NULL");
        err = errno;
    }

    return err;
}/*}}}*/

spx_private struct monitor_http_context* CTXPop(){/*{{{*/
    struct monitor_http_context *mtr_ctx = mtr_ctx_top;
    if(NULL != mtr_ctx){
        mtr_ctx_top = mtr_ctx->p_preCTX;
        if (mtr_ctx_top != NULL){
            mtr_ctx_top->p_nextCTX = NULL;
        }
        mtr_ctx->p_preCTX = NULL;
    }

    return mtr_ctx;
}/*}}}*/

spx_private err_t CTXPush(struct monitor_http_context * mtr_ctx){/*{{{*/
    err_t err = 0;

    if(mtr_ctx == NULL){
        perror("CTXPush NULL");
        err = errno;
    }else{
        FreeParam(mtr_ctx);
        mtr_ctx->req_len = 0;
        mtr_ctx->resp_len = 0;
        mtr_ctx->req_retry = 0;
        mtr_ctx->resp_retry = 0;
        mtr_ctx->req_size = 0;
        mtr_ctx->resp_size = 0;
        mtr_ctx->split_size = 0;

        memset(mtr_ctx->request, 0, sizeof(char)*REQUEST_SIZE);
        free(mtr_ctx->response);
        mtr_ctx->response = NULL;
        mtr_ctx->param_head = NULL;
        mtr_ctx->param_tail = NULL;

        if(NULL == mtr_ctx_top){
            mtr_ctx_top = mtr_ctx;
        }else{
            mtr_ctx_top->p_nextCTX = mtr_ctx;
            mtr_ctx->p_preCTX = mtr_ctx_top;
            mtr_ctx_top = mtr_ctx;
        }
    }

    return err;
}/*}}}*/

spx_private int GetCTXCount(){
    int count = 0;
    struct monitor_http_context *mtr_ctx = mtr_ctx_top;
    while(mtr_ctx != NULL){
        count++;
        mtr_ctx = mtr_ctx->p_preCTX;
    }

    return count;
}

spx_private struct monitor_http_context * GetCTXTop(){
    struct monitor_http_context *mtr_ctx = mtr_ctx_top;
    return mtr_ctx;
}

spx_private err_t NewCTXPool(int pool_size, struct IdcreatorConfig *c){/*{{{*/
   err_t err = 0;
   int i = 0;

   for(i = 0; i < pool_size; i++){
        struct monitor_http_context *mtr_ctx = (struct monitor_http_context *) malloc(sizeof(struct monitor_http_context));
        if(0 != CTXInit(mtr_ctx, c)){
           perror("NewCTXPool Failed");
           FreeMonitorCTXPool();
           err = errno;
           break;
        }else{
            if(NULL == mtr_ctx_top){
                mtr_ctx_top = mtr_ctx;
            }else{
                mtr_ctx_top->p_nextCTX = mtr_ctx;
                mtr_ctx->p_preCTX = mtr_ctx_top;
                mtr_ctx_top = mtr_ctx;
            }
        }
   }

  return err;
}/*}}}*/

spx_private void FreeMonitorCTXPool(){/*{{{*/
    while(NULL != mtr_ctx_top){
        struct monitor_http_context * mtr_ctx = mtr_ctx_top;
        mtr_ctx_top = mtr_ctx->p_preCTX;
        free(mtr_ctx->request);
        free(mtr_ctx);
    }
}/*}}}*/

spx_private void ydb_tracker_monitor_io_watcher_register(ev_io *watcher ,int fd, void (*cb)(struct ev_loop *loop, ev_io *watcher, int revents), int revents, void *data){
    SpxZero(*watcher);
    ev_io_init(watcher, cb, fd, revents);
    watcher->data = data;
}

spx_private void RegisterAsyncWatcher(ev_async *watcher, void(*cb)(struct ev_loop *loop, ev_async *watcher, int revents), void *data){
    SpxZero(*watcher);
    ev_async_init(watcher, cb);
    watcher->data = data;
}

/**********************************************************************/
/* Inform the client that a request it has made has a problem.
 * Parameters: client socket */
/**********************************************************************/
spx_private void bad_request(int client)/*{{{*/
{
 char head[1024]={0};
 char tmp[1024]={0};
 //char body[1024] = {"<HTML><BODY><P>Your browser sent a bad request, such as a POST without a Content-Length.</HTML></BODY>\r\n"};
 char body[1024] = {"{\"error\":412, \"errorMessage\":\"Bad Request\"}\r\n"};
 int content_length = strlen(body);

 snprintf(tmp, sizeof(tmp),"HTTP/1.1 400 BAD REQUEST\r\n");
 strncpy(head, tmp, strlen(tmp));
 snprintf(tmp, sizeof(tmp),"Content-type: text/html\r\n");
 strncat(head, tmp, strlen(tmp));
 snprintf(tmp, sizeof(tmp), "Content-Length: %d\r\n", content_length);
 strncat(head, tmp, strlen(tmp));
 snprintf(tmp, sizeof(tmp), "\r\n");
 strncat(head, tmp, strlen(tmp));

 send(client, head, strlen(head), 0);
 send(client, body, strlen(body), 0);
}/*}}}*/

/**********************************************************************/
/* Return the informational HTTP headers about a file. */
/* Parameters: the socket to print the headers on
 *             the name of the file */
/**********************************************************************/
spx_private void headers(int client, int content_length)/*{{{*/
{
 char head[1024]={0};
 char tmp[1024]={0};

 sprintf(tmp, "HTTP/1.1 200 OK\r\n");
 strncpy(head, tmp,  strlen(tmp));
 sprintf(tmp, SERVER_STRING);
 strncat(head, tmp, strlen(tmp));
 sprintf(tmp, "Content-Type: text/html;charset=UTF-8\r\n");
 strncat(head, tmp, strlen(tmp));
 sprintf(tmp, "Content-Length: %d\r\n", content_length);
 strncat(head, tmp, strlen(tmp));
 sprintf(tmp, "\r\n");
 strncat(head, tmp, strlen(tmp));

 send(client, head, strlen(head), 0);
}/*}}}*/

/**********************************************************************/
/* Inform the client that the requested web method has not been
 * implemented.
 * Parameter: the client socket */
/**********************************************************************/
spx_private void unimplemented(int client)/*{{{*/
{
 char head[1024]={0};
 //char body[1024]={0};
 char buf[4096]={0};
 char tmp[1024]={0};
 int content_length = 0;

 //snprintf(tmp, sizeof(tmp), "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
 //strncpy(body, tmp, strlen(tmp));
 //snprintf(tmp, sizeof(tmp), "</TITLE></HEAD>\r\n");
 //strncat(body, tmp, strlen(tmp));
 //snprintf(tmp, sizeof(tmp), "<BODY><P>HTTP request method not supported.\r\n");
 //strncat(body, tmp, strlen(tmp));
 //snprintf(tmp, sizeof(tmp), "</BODY></HTML>\r\n");
 //strncat(body, tmp, strlen(tmp));
 char body[1024] = {"{\"error\":404, \"errorMessage\":\"Service Not Found\"}\r\n"};

 strncpy(buf, body, strlen(body));

 content_length = strlen(body);
 headers(client, content_length);
 send(client, body, strlen(body), 0);
}/*}}}*/

spx_private int Request_GetRequest_ReadRequest_IsEnd(byte_t* buf, size_t len){/*{{{*/
    if((len > 4)&&(*(buf + len -1) == '\n')&&(*(buf + len -2 ) == '\r')&&(*(buf + len - 3) == '\n')&&(*(buf + len -4) == '\r'))
        return 1;
    else
        return 0;
}/*}}}*/

spx_private err_t Request_GetRequest_ReadRequest(int fd, byte_t *buf, size_t *len){/*{{{*/
    SpxErrReset;
    i64_t rc = 0;
    err_t err = 0;

    while(!Request_GetRequest_ReadRequest_IsEnd(buf, *len)&&(*len <= REQUEST_SIZE)){
        if (*len >= REQUEST_SIZE){
            err = -1;
            SpxLogFmt1(mtr_log, SpxLogError, "request buffer is out of range %d\n", REQUEST_SIZE);
            break;
        }

        rc = read(fd, ((byte_t *) buf) + *len, REQUEST_SIZE);
        if(0 > rc){
            err = errno;
            break;
        }else if(0 == rc){
            if(0 == strlen((char *)buf)){
                err = -1;
                SpxLog1(mtr_log, SpxLogError, "client is closed\n");
            }
            break;
        }else{
            *len += rc;
        }

    }

    return err;
}/*}}}*/

spx_private i64_t SendResponse_WriteResponse(int fd, byte_t *buf, size_t *len, size_t *size){/*{{{*/
    SpxErrReset;
    i64_t rc = 0;
    err_t err = 0;

    rc = write(fd, ((byte_t *) buf) + *len, *size);

    if(0 > rc){
        err = errno;
    }else{
        *len += rc;
        *size -= rc;
    }

    return err;
}/*}}}*/

spx_private void SendResponse_ReWriteResponse(int revents, void *arg){/*{{{*/
    struct monitor_http_context *mtr_ctx = (struct monitor_http_context *) arg;
    if(NULL == mtr_ctx){
        SpxLog1(mtr_log, SpxLogError, "SendResponse mtr_ctx is NULL");
        return;
    }

    if(EV_ERROR & revents){
        SpxLog1(mtr_log, SpxLogError, "EV_ERROR");
        return;
    }

    if(EV_TIMEOUT & revents){
      if((mtr_ctx->resp_retry++) >= RETRY_TIMES){
        RequestException(mtr_ctx, bad_request);
        SpxLog1(mtr_log, SpxLogError, "EV_TIMEOUT");
        return;
      }else{
        ev_once(main_socket_loop, mtr_ctx->fd, EV_WRITE, mtr_ctx->timeout, SendResponse_ReWriteResponse, mtr_ctx);
        return;
      }
   }

   if(EV_WRITE & revents){
     err_t err = SendResponse_WriteResponse(mtr_ctx->fd, mtr_ctx->response, &mtr_ctx->resp_len, &mtr_ctx->split_size);
     if((0 == err)&&(0 == mtr_ctx->split_size)){
          if(mtr_ctx->resp_size == mtr_ctx->resp_len){
                RequestFinish(mtr_ctx);
          }else{
            int remain_size = mtr_ctx->resp_size - mtr_ctx->resp_len;
            if(remain_size >= SPLIT_SIZE){
                mtr_ctx->split_size = SPLIT_SIZE;
            }else{
                mtr_ctx->split_size = remain_size;
            }

            RegisterAsyncWatcher(&mtr_ctx->async_watcher, SendResponse, mtr_ctx);
            ev_async_start(main_socket_loop, &mtr_ctx->async_watcher);
            ev_async_send(main_socket_loop, &mtr_ctx->async_watcher);
            return;
          }
     }else{
          if((EAGAIN == err || EWOULDBLOCK == err || EINTR == err)||(mtr_ctx->resp_size > 0)) {
              ev_once(main_socket_loop, mtr_ctx->fd, EV_READ, mtr_ctx->timeout, SendResponse_ReWriteResponse, mtr_ctx);
              return;
          }else{
             SpxLog2(mtr_log, SpxLogError, err,"SendResponse Failed");
             RequestException(mtr_ctx, bad_request);
          }
     }
   }
}/*}}}*/

spx_private void SendResponse(EV_P_ ev_async *watcher, int revents){/*{{{*/
    ev_async_stop(loop, watcher);
    struct monitor_http_context * mtr_ctx = (struct monitor_http_context *) watcher->data;
    if(NULL == mtr_ctx){
        SpxLog1(mtr_log, SpxLogError, "SendResponse mtr_ctx is NULL");
        return;
    }

    err_t err = SendResponse_WriteResponse(mtr_ctx->fd, mtr_ctx->response, &mtr_ctx->resp_len, &mtr_ctx->split_size);
    if((0 == err)&&(0 == mtr_ctx->split_size)){
         if(mtr_ctx->resp_size == mtr_ctx->resp_len){
            RequestFinish(mtr_ctx);
         }else{
            int remain_size = mtr_ctx->resp_size - mtr_ctx->resp_len;
            if(remain_size >= SPLIT_SIZE){
                mtr_ctx->split_size = SPLIT_SIZE;
            }else{
                mtr_ctx->split_size = remain_size;
            }

            RegisterAsyncWatcher(&mtr_ctx->async_watcher, SendResponse, mtr_ctx);
            ev_async_start(loop, &mtr_ctx->async_watcher);
            ev_async_send(loop, &mtr_ctx->async_watcher);
            return;
         }
    }else{
         if((EAGAIN == err || EWOULDBLOCK == err || EINTR == err)||(mtr_ctx->resp_size > 0)) {
             ev_once(main_socket_loop, mtr_ctx->fd, EV_READ, mtr_ctx->timeout, SendResponse_ReWriteResponse, mtr_ctx);
             return;
         }else{
           SpxLog2(mtr_log, SpxLogError, err,"SendResponse Failed");
           RequestException(mtr_ctx, bad_request);
         }
    }
}/*}}}*/

pthread_t NewIdCreatorHttp(SpxLogDelegate *log, struct IdcreatorConfig *c, err_t *err){/*{{{*/
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    mtr_log = log;

    size_t ostack_size = 0;
    pthread_attr_getstacksize(&attr, &ostack_size);
    if (ostack_size != c->stackSize
            && (0 != (*err = pthread_attr_setstacksize(&attr,c->stackSize)))){
        return 0;

    }

    struct mainsocket_thread_arg *arg = (struct mainsocket_thread_arg *) spx_alloc_alone(sizeof(*arg), err);
    if(NULL == arg){
        pthread_attr_destroy(&attr);
        return 0;
    }

    arg->log = log;
    arg->c = c;

    if(0 != NewCTXPool(POOL_SIZE, c)){
        SpxLog1(log, SpxLogError, "NewCTXPool failed.");
        pthread_attr_destroy(&attr);
        SpxFree(arg);
        return 0;
    }

    pthread_t tid = 0;
    if( 0 != (*err = pthread_create(&tid, &attr, CreatMainSocket, arg))){
        pthread_attr_destroy(&attr);
        SpxFree(arg);
        return 0;
    }

    pthread_attr_destroy(&attr);
    return tid;
}/*}}}*/

spx_private void * CreatMainSocket(void *arg){/*{{{*/
    struct mainsocket_thread_arg *mainsocket_arg = (struct mainsocket_thread_arg *) arg;
    SpxLogDelegate *log = mainsocket_arg->log;
    struct IdcreatorConfig *c = mainsocket_arg->c;
    SpxFree(mainsocket_arg);
    err_t err = 0;

    main_socket_loop = ev_loop_new(0);
    if(NULL == main_socket_loop){
        SpxLog2(log, SpxLogError, err, "create main socket loop is fail.");
        goto r1;
    }

    int mainsocket = spx_socket_new(&err);
    if(0 == mainsocket){
        SpxLog2(log, SpxLogError, err, "create main socket is fail.");
        goto r1;
    }

    if( 0 != (err = spx_set_nb(mainsocket))){
        SpxLog2(log, SpxLogError, err, "set main socket nonblock is fail.");
    }

    if( 0 != (err = spx_socket_start(mainsocket, c->ip, c->http_port,\
                                     true, c->timeout,\
                                     3, c->timeout,\
                                     false, 0,\
                                     true,\
                                     true, c->timeout,
                                     1024))){
        SpxLog2(log, SpxLogError, err, "start main socket is fail.");
        goto r1;
    }

    SpxLogFmt1(log, SpxLogMark,
               "main socket fd: %d"
               "and accepting...",
               mainsocket);

    ydb_tracker_monitor_io_watcher_register(&monitor_watcher, mainsocket, Reciver, EV_READ, log);
    ev_io_start(main_socket_loop, &monitor_watcher);

    ev_run(main_socket_loop, 0);
r1:
    SpxClose(mainsocket);
    return NULL;
}/*}}}*/

spx_private void Reciver(struct ev_loop *loop, ev_io *watcher, int revents){/*{{{*/
    ev_io_stop(loop, watcher);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    SpxLogDelegate *log = (SpxLogDelegate *) watcher->data;
    err_t err = 0;

    while(true){
        struct sockaddr_in client_addr;
        pthread_t tid = 0;
        unsigned int socket_len = 0;
        int client_sock = 0;
        socket_len = sizeof(struct sockaddr_in);
        client_sock = accept(watcher->fd, (struct sockaddr *) &client_addr, &socket_len);

        if (0 > client_sock){
            if( EWOULDBLOCK == errno || EAGAIN == errno){
                break;
            }
            break;
        }

        if( 0  == client_sock){
            break;
        }

        if( 0 != (err = spx_set_nb(client_sock))){
            SpxLog2(log, SpxLogError, err, "set main client_socket nonblock is fail.");
        }

        printf("\n----------------CLIENT:%d START CTX:%d-----------------------\n", client_sock, GetCTXCount());

        struct monitor_http_context * mtr_ctx = CTXPop();

        printf("\n----------------CLIENT:%d POP CTX:%d-----------------------\n", client_sock, GetCTXCount());

        if(NULL != mtr_ctx){
            mtr_ctx->fd = client_sock;
            RegisterAsyncWatcher(&mtr_ctx->async_watcher, Request, mtr_ctx);
            ev_async_start(loop, &mtr_ctx->async_watcher);
            ev_async_send(loop, &mtr_ctx->async_watcher);
        }

    }

    ev_io_start(loop, watcher);
}/*}}}*/

spx_private void Request(EV_P_ ev_async *watcher, int revents){/*{{{*/
    ev_async_stop(loop, watcher);
    struct monitor_http_context * mtr_ctx = (struct monitor_http_context *) watcher->data;
    if(NULL == mtr_ctx){
        SpxLog1(mtr_log, SpxLogError, "Request mtr_ctx is NULL");
        return;
    }
    err_t err = Request_GetRequest_ReadRequest(mtr_ctx->fd, mtr_ctx->request, &mtr_ctx->req_len);
    if(0 == err){
        RegisterAsyncWatcher(&mtr_ctx->async_watcher, ParserRequest, mtr_ctx);
        ev_async_start(loop, &mtr_ctx->async_watcher);
        ev_async_send(loop, &mtr_ctx->async_watcher);
        return;
    }else{
        if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err) {
                ev_once(loop, mtr_ctx->fd, EV_READ, mtr_ctx->timeout, Request_GetRequest, mtr_ctx);
                return;
        }else{
            SpxLog2(mtr_log, SpxLogError, err,"Request_GetRequest_ReadRequest Failed");
            if( -1 == err )
                CloseCTX(mtr_ctx);
            else
                RequestException(mtr_ctx, bad_request);
            return;
        }
    }
}/*}}}*/

spx_private void Request_GetRequest(int revents, void *arg){/*{{{*/
    struct monitor_http_context *mtr_ctx = (struct monitor_http_context *) arg;
    if(NULL == mtr_ctx){
        SpxLog1(mtr_log, SpxLogError, "Request_GetRequest mtr_ctx is NULL");
        return;
    }

    if(EV_ERROR & revents){
        SpxLog1(mtr_log, SpxLogError, "EV_ERROR");
        return;
    }

    if(EV_TIMEOUT & revents){
      if((mtr_ctx->req_retry++) >= RETRY_TIMES){
        RequestException(mtr_ctx, bad_request);
        SpxLog1(mtr_log, SpxLogError, "EV_TIMEOUT");
        return;
      }else{
        ev_once(main_socket_loop, mtr_ctx->fd, EV_READ, mtr_ctx->timeout, Request_GetRequest, mtr_ctx);
        return;
      }
   }

   if(EV_READ & revents){
     err_t err = Request_GetRequest_ReadRequest(mtr_ctx->fd, mtr_ctx->request, &mtr_ctx->req_len);
     if(0 == err){
         RegisterAsyncWatcher(&mtr_ctx->async_watcher, ParserRequest, mtr_ctx);
         ev_async_start(main_socket_loop, &mtr_ctx->async_watcher);
         ev_async_send(main_socket_loop, &mtr_ctx->async_watcher);
     }else{
          if((EAGAIN == err || EWOULDBLOCK == err || EINTR == err)) {
              ev_once(main_socket_loop, mtr_ctx->fd, EV_READ, mtr_ctx->timeout, Request_GetRequest, mtr_ctx);
              return;
          }else{
            if( -1 == err )
                CloseCTX(mtr_ctx);
            else
                RequestException(mtr_ctx, bad_request);

             return;
          }
     }
   }

}/*}}}*/

spx_private void ParserRequest(EV_P_ ev_async *watcher, int revents){/*{{{*/
    ev_async_stop(loop, watcher);
    err_t err = 0;
    struct monitor_http_context * mtr_ctx = (struct monitor_http_context *) watcher->data;
    if(NULL == mtr_ctx){
        SpxLog1(mtr_log, SpxLogError, "ParserRequest mtr_ctx is NULL");
        return;
    }

   int client_sock = mtr_ctx->fd;
   size_t i = 0, j = 0;
   char *buf = (char *)mtr_ctx->request;
   char method[255] = {0};
   char url[1024]= {0};

   if(mtr_ctx->req_len == 0){
        CloseCTX(mtr_ctx);
        SpxLog1(mtr_log, SpxLogError, "Client Socket is closed");
        return;
   }

   while(!ISspace(buf[j]) && (i < sizeof(method) -1)){
       method[i] = buf[j];
       i++; j++;
   }

   method[i] = '\0';

   if(0 == strcasecmp(method, "POST")){
       RequestException(mtr_ctx, unimplemented);
       return ;
   }

   i = 0;
   while(ISspace(buf[j]) && (j < strlen(buf)))
       j++;
   while (!ISspace(buf[j]) && (i < sizeof(url) -1) && (j < strlen(buf))){
       url[i] = buf[j];
       i++;
       j++;
   }

   url[i] = '\0';

   size_t url_idx = 0;
   while((url[url_idx] != '?') && (url[url_idx] != '\0')){
//       path[url_idx] = url[url_idx];
       url_idx++;
   }

   if(url_idx < strlen(url) && url[url_idx] == '?')
       url_idx++;
   while(url_idx < strlen(url) && url[url_idx] != '\0'){
       char tmp[100];
       size_t tmp_len = 0;
       size_t tmp_total = strlen(url) - url_idx;

       while(url[url_idx] != '&' && tmp_len < tmp_total){
            tmp[tmp_len++] = url[url_idx++];
       }

       size_t part_cursor = 0, global_cursor = 0, key_len = 0, val_len = 0;
       char tmpKey[100] = {0};
       char tmpVal[100] = {0};

       while(tmp[part_cursor] != '=' && global_cursor < tmp_len){
           tmpKey[part_cursor++] = tmp[global_cursor++];
       }
       if(global_cursor == tmp_len){
           RequestException(mtr_ctx, bad_request);
           return;
       }

       key_len = part_cursor;

       global_cursor++;
       part_cursor = 0;

       while(global_cursor < tmp_len){
           tmpVal[part_cursor++] = tmp[global_cursor++];
       }
       val_len = part_cursor;

       InsertParam(mtr_ctx, tmpKey, key_len, tmpVal, val_len);
       url_idx++;
   }

   if(strcasecmp(method, "GET") == 0){
           RegisterAsyncWatcher(&mtr_ctx->async_watcher,ydb_tracker_monitor_socket_response, mtr_ctx);
           ev_async_start(loop, &mtr_ctx->async_watcher);
           ev_async_send(loop, &mtr_ctx->async_watcher);
           return;
   }

   if(0 == strcasecmp(method, "POST")){
       RequestException(mtr_ctx, unimplemented);
       return;
   }

   RequestException(mtr_ctx, bad_request);
}/*}}}*/


spx_private int IsNumber(char * str){
    int len = strlen(str);
    if(0 == len)
        return 0;
    while(*str != '\0'){
       if(*str < '0' || *str > '9')
            return 0;
        str++;
    }

    return 1;
}

spx_private void ydb_tracker_monitor_socket_response(EV_P_ ev_async *watcher, int revents){/*{{{*/
    ev_async_stop(loop, watcher);
    err_t err = 0;
    struct monitor_http_context * mtr_ctx = (struct monitor_http_context *) watcher->data;
    if(NULL == mtr_ctx){
        SpxLog1(mtr_log, SpxLogError, "ydb_tracker_monitor_socket_response mtr_ctx is NULL");
        return;
    }

    char key[20] = "type";
    struct param_node * param = GetParam(mtr_ctx, key);
    if(NULL == param){
        RequestException(mtr_ctx, bad_request);
        SpxLog1(mtr_log, SpxLogError, "param is null");
        return;
    }

    char * type = param->value;
    int client = mtr_ctx->fd;
    int mid = mtr_ctx->c->mid;

    if(!IsNumber(type)){
        RequestException(mtr_ctx, bad_request);
        SpxLog1(mtr_log, SpxLogError, "type is not a nunber");
        return;
    }

    u64_t id = IdGenerator(atoi(type), mid, &err);
    char msg[20] = "success";
    if(0 != err){
        strcpy(msg, "failed");
    }
    char *buf = calloc(1, JSON_SIZE*sizeof(char));

    char ret_json[JSON_SIZE]={0};
    char errorNo[ELEMENT_SIZE];
    char errorMessage[ELEMENT_SIZE];
    char result[ELEMENT_SIZE];

    strncpy(ret_json, "{", 1);
    snprintf(errorNo, ELEMENT_SIZE, "\"error\":%d,", err);
    strncat(ret_json, errorNo, strlen(errorNo));

    snprintf(errorNo, ELEMENT_SIZE, "\"errorMessage\":\"%s\",", msg);
    strncat(ret_json, errorNo, strlen(errorNo));

    snprintf(result, ELEMENT_SIZE, "\"result\":%llu", (unsigned long long)id);
    strncat(ret_json, result, strlen(result));
    strncat(ret_json, "}\r\n", 1);

    strncpy(buf, ret_json, strlen(ret_json));

    mtr_ctx->response = (byte_t *)buf;
    mtr_ctx->resp_size = strlen(buf);
    headers(client, strlen(buf));

    int remain_size = mtr_ctx->resp_size - mtr_ctx->resp_len;
    if(remain_size >= SPLIT_SIZE){
       mtr_ctx->split_size = SPLIT_SIZE;
   }else{
       mtr_ctx->split_size = remain_size;
   }

    RegisterAsyncWatcher(&mtr_ctx->async_watcher, SendResponse, mtr_ctx);
    ev_async_start(loop, &mtr_ctx->async_watcher);
    ev_async_send(loop, &mtr_ctx->async_watcher);
}/*}}}*/

spx_private void RequestFinish(struct monitor_http_context *mtr_ctx){
        CloseCTX(mtr_ctx);
        printf("----------------CLIENT:%d SUCCESS CTX:%d-----------------------\n\n\n", mtr_ctx->fd, GetCTXCount());
}

spx_private void RequestException(struct monitor_http_context * mtr_ctx, void (*handle)(int client)){
        handle(mtr_ctx->fd);
        CloseCTX(mtr_ctx);
        printf("----------------CLIENT:%d FAILED CTX:%d-----------------------\n\n\n", mtr_ctx->fd, GetCTXCount());
}

spx_private void CloseCTX(struct monitor_http_context *mtr_ctx){
        SpxClose(mtr_ctx->fd);
        CTXPush(mtr_ctx);
}

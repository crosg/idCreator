/*************************************************************************
    > File Name: spx_web_server.c
    > Author: shuaixiang
    > Mail: shuaixiang@yuewen.com
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
#include "spx_web_server.h"

#define SERVER_INFO "Server: Sparrow/1.0.0\r\n"
#define RETRY_TIMES 3
#define HTTP_MAX_HEADER_SIZE 10240
#define SPLIT_SIZE 1024 

#define IsSpace(x) isspace((int)(x))

spx_private struct ev_loop *main_socket_loop = NULL;
spx_private ev_io monitor_watcher;
spx_private SpxLogDelegate *g_log = NULL;

spx_private struct server_context * ctx_top = NULL;
//Response Handler
spx_private void spx_web_server_build_bad_request(int client);
spx_private void spx_web_server_build_header(int client, int content_length);
spx_private void spx_web_server_build_unimplement(int client);

//Server Service
pthread_t spx_web_server_thread_new(SpxLogDelegate *log, struct server_config *conf, err_t *err);
spx_private void * spx_web_server_main_socket_create(void *arg);
spx_private err_t spx_web_server_ctx_new_pool(struct server_config *conf, char *(*ServerHandler)(struct request *req));
spx_private void spx_web_server_ctx_free_pool();
spx_private int spx_web_server_http_header_read_is_end(char* buf, size_t len);
spx_private err_t spx_web_server_http_header_read(int fd, char *buf, size_t *len);
spx_private err_t spx_web_server_http_content_read(int fd, char *buf, size_t *len, size_t size);
spx_private void spx_web_server_http_body_send(EV_P_ ev_async *watcher, int revents);
spx_private void spx_web_server_watcher_async_register(ev_async *watcher, void(*cb)(struct ev_loop *loop, ev_async *watcher, int revents), void *data);
spx_private void spx_web_server_watcher_io_register(ev_io *watcher ,int fd, void (*cb)(struct ev_loop *loop, ev_io *watcher, int revents), int revents, void *data);
spx_private void spx_web_server_reciver(struct ev_loop *loop, ev_io *watcher, int revents);
spx_private void spx_web_server_recive_request(struct ev_loop *loop, ev_async *watcher, int revents);
spx_private void spx_web_server_recive_request_loop(int revents, void *arg);
spx_private void spx_web_server_parse_request(EV_P_ ev_async *watcher, int revents);
spx_private void spx_web_server_recive_content(EV_P_ ev_async *watcher, int revents);
spx_private void spx_web_server_recive_content_loop(int revents, void *arg);
spx_private void spx_web_server_handle_request(EV_P_ ev_async *watcher, int revents);
spx_private void spx_web_server_send_response(EV_P_ ev_async *watcher, int revents);
spx_private void spx_web_server_request_finish(struct server_context *ctx);
spx_private void spx_web_server_request_exception(struct server_context * ctx, void (*handle)(int client));
spx_private void spx_web_server_ctx_close(struct server_context *ctx);

spx_private struct param_set_node* spx_web_server_param_set_insert(struct request *req, char *key, \
                                int key_len, char *val, int val_len);
spx_private err_t spx_web_server_param_set_free(struct request *req);
spx_private err_t spx_web_server_ctx_init(struct server_context * ctx, char *(*ServerHandler)(struct request *req), \
                                          struct server_config *conf);
spx_private struct server_context* spx_web_server_ctx_pop();
spx_private err_t spx_web_server_ctx_push(struct server_context *ctx);
spx_private int spx_web_server_ctx_get_count();
spx_private struct server_context *spx_web_server_ctx_get_top();

spx_private struct param_set_node* spx_web_server_param_set_insert(struct request *req, char *key, \
                                int key_len, char *val, int val_len){/*{{{*/
    if(NULL == req){
        SpxLog1(g_log, SpxLogError, "spx_web_server_param_set_insert request is NULL");
        return NULL;
    }
    struct param_set_node *param = (struct param_set_node *) calloc(1, sizeof(struct param_set_node));
    if(NULL == param){
        SpxLog1(g_log, SpxLogError, "calloc param failed");
        return NULL;
    }

    strncpy(param->key, key, key_len);
    strncpy(param->value, val, val_len);
    param->next_param = NULL;

    struct param_set *ps = req->p_set;

    if(NULL == ps->param_tail){
        ps->param_head = param;
        ps->param_tail = param;
    }else{
        ps->param_tail->next_param = param;
        ps->param_tail = param;
    }

    return param;
}/*}}}*/

struct param_set_node* spx_web_server_param_set_get(struct request *req, char *key){/*{{{*/
    if(NULL == req){
        SpxLog1(g_log, SpxLogError, "spx_web_server_param_set_get request is NULL");
        return NULL;
    }
    struct param_set *ps = req->p_set;

    struct param_set_node *p_node = ps->param_head;

    while(NULL != p_node){
        if(0 == strncmp(p_node->key, key, strlen(key))){
            return p_node;
        }else{
            p_node = p_node->next_param;
        }
    }

    return NULL;
}/*}}}*/

spx_private err_t spx_web_server_param_set_free(struct request *req){/*{{{*/
    err_t err = 0;
    if(NULL == req){
        SpxLog1(g_log, SpxLogError, "spx_web_server_param_set_get request is NULL");
        return errno;
    }
    struct param_set *ps = req->p_set;
    struct param_set_node *p_node = ps->param_head;

    while(NULL != p_node){
            struct param_set_node *tmp_node = p_node;
            p_node = p_node->next_param;
            free(tmp_node);
    }

    ps->param_head = NULL;
    ps->param_tail = NULL;

    return err;
}/*}}}*/

spx_private err_t spx_web_server_ctx_init(struct server_context * ctx, char *(*ServerHandler)(struct request *req), \
                                          struct server_config *conf){/*{{{*/
    err_t err = 0;
    if(NULL == ctx){
        SpxLog1(g_log, SpxLogError, "spx_web_server_ctx_init ctx is NULL");
        return errno; 
    }

    ctx->p_next_ctx = NULL;
    ctx->p_pre_ctx = NULL;
    ctx->fd = 0;
    ctx->timeout = conf->timeout;

    ctx->req = (struct request*)malloc(sizeof(struct request));
    if(NULL == ctx->req){
        SpxLog1(g_log, SpxLogError, "ctx malloc request failed");        
        err = errno;
    }
    ctx->req->retry = 0;
    ctx->req->header_len = 0;
    ctx->req->header_size = 0;
    ctx->req->content_len = 0;
    ctx->req->content_size = 0;
    ctx->req->p_set = (struct param_set*) malloc(sizeof(struct param_set));
    ctx->req->p_set->param_head = NULL;
    ctx->req->p_set->param_tail = NULL;
    ctx->req->content = NULL;
    
    ctx->resp = (struct response*)malloc(sizeof(struct response));
    if(NULL == ctx->resp){
        SpxLog1(g_log, SpxLogError, "ctx malloc resp failed");        
        err = errno;
    }
    ctx->resp->retry = 0;
    ctx->resp->header_len = 0;
    ctx->resp->header_size = 0;
    ctx->resp->content_len = 0;   
    ctx->resp->content_size = 0;   
    ctx->resp->split_size = 0;
    ctx->resp->content = NULL;

    ctx->ServerHandler = ServerHandler;


    return err;
}/*}}}*/

spx_private struct server_context* spx_web_server_ctx_pop(){/*{{{*/
    struct server_context *ctx = ctx_top;
    if(NULL != ctx){
        ctx_top = ctx->p_pre_ctx;
        ctx_top->p_next_ctx = NULL;
        ctx->p_pre_ctx = NULL;
    }

    return ctx;
}/*}}}*/

spx_private err_t spx_web_server_ctx_push(struct server_context *ctx){/*{{{*/
    err_t err = 0;

    if(ctx == NULL){
        perror("spx_web_server_ctx_push NULL");
        err = errno;
    }else{
        ctx->req->retry = 0;
        ctx->req->header_len = 0;
        ctx->req->header_size = 0;
        ctx->req->content_len = 0;
        ctx->req->content_size = 0;

        ctx->resp->retry = 0;
        ctx->resp->split_size = 0;
        ctx->resp->header_len = 0;
        ctx->resp->header_size = 0;
        ctx->resp->content_size = 0;
        ctx->resp->content_len = 0;

        spx_web_server_param_set_free(ctx->req);
        memset(ctx->req->header, 0, sizeof(ctx->req->header));
        if (ctx->req->content != NULL){
            free(ctx->req->content);
            ctx->req->content = NULL;
        }
        memset(ctx->resp->header, 0, sizeof(ctx->resp->header));
        if (ctx->resp->content != NULL){
            free(ctx->resp->content);
            ctx->resp->content = NULL;
        }

        if(NULL == ctx_top){
            ctx_top = ctx;
        } else {
            ctx_top->p_next_ctx = ctx;
            ctx->p_pre_ctx = ctx_top;
            ctx_top = ctx;
        }
    }
    
    return err;
}/*}}}*/

spx_private int spx_web_server_ctx_get_count(){/*{{{*/
    int count = 0;
    struct server_context *ctx = ctx_top;
    while(ctx != NULL){
        count++;
        ctx = ctx->p_pre_ctx;
    }

    return count;
}/*}}}*/

spx_private struct server_context *spx_web_server_ctx_get_top(){/*{{{*/
    struct server_context *ctx = ctx_top;
    return ctx;
}/*}}}*/

spx_private err_t spx_web_server_ctx_new_pool(struct server_config *conf, char *(*ServerHandler)(struct request *req)){/*{{{*/
   err_t err = 0; 
   uint32_t i = 0;

   for(i = 0; i < conf->context_pool_size; i++){
        struct server_context *ctx = (struct server_context *) malloc(sizeof(struct server_context));
        if(0 != spx_web_server_ctx_init(ctx, ServerHandler, conf)){
           perror("spx_web_server_ctx_new_pool Failed");
           spx_web_server_ctx_free_pool();
           err = errno;
           break;
        }else{
            if(NULL == ctx_top){
                ctx_top = ctx;
            }else{
                ctx_top->p_next_ctx = ctx;
                ctx->p_pre_ctx = ctx_top;
                ctx_top = ctx;
            }
        }
   }

  return err;
}/*}}}*/

spx_private void spx_web_server_ctx_free_pool(){/*{{{*/
    while(NULL != ctx_top){
        struct server_context *ctx = ctx_top;
        ctx_top = ctx->p_pre_ctx;
        if (ctx->req != NULL)
            if (ctx->req->p_set != NULL){
                free(ctx->req->p_set);
            if (ctx->req->content != NULL)
                free(ctx->req->content);
            free(ctx->req);
        }
        if (ctx->resp != NULL){
            if (ctx->resp->content != NULL)
                free(ctx->resp->content);
            free(ctx->resp);
        }
        free(ctx);
    }
}/*}}}*/

spx_private void spx_web_server_watcher_io_register(ev_io *watcher ,int fd, void (*cb)(struct ev_loop *loop, ev_io *watcher, int revents), int revents, void *data){/*{{{*/
    SpxZero(*watcher);
    ev_io_init(watcher, cb, fd, revents);
    watcher->data = data;
}/*}}}*/

spx_private void spx_web_server_watcher_async_register(ev_async *watcher, void(*cb)(struct ev_loop *loop, ev_async *watcher, int revents), void *data){/*{{{*/
    SpxZero(*watcher);
    ev_async_init(watcher, cb);
    watcher->data = data;
}/*}}}*/

spx_private int spx_web_server_http_header_read_is_end(char* buf, size_t len){/*{{{*/
    if((len > 4)&&(*(buf + len -1) == '\n')&&(*(buf + len -2 ) == '\r')&&(*(buf + len - 3) == '\n')&&(*(buf + len -4) == '\r'))
        return 1;
    else
        return 0;
}/*}}}*/

spx_private err_t spx_web_server_http_header_read(int fd, char *buf, size_t *len){/*{{{*/
    SpxErrReset;
    i64_t rc = 0;
    err_t err = 0;

    while(!spx_web_server_http_header_read_is_end(buf, *len)&&(*len <= HTTP_MAX_HEADER_SIZE)){
        rc = read(fd, ((char *) buf) + *len, HTTP_MAX_HEADER_SIZE);
        if(0 > rc){
            err = errno;
            break;
        }else if(0 == rc){
            if(0 == strlen(buf)){
                SpxLog1(g_log, SpxLogError, "connection is closed\n");
                err = -1;
            }
            break;
        }else{
            *len += rc;
        }

    }

    return err;
}/*}}}*/

spx_private err_t spx_web_server_http_content_read(int fd, char *buf, size_t *len, size_t size){/*{{{*/
    SpxErrReset;
    i64_t rc = 0;
    err_t err = 0;

    while(*len < size){
        rc = read(fd, ((char *) buf) + *len, size - *len);
        if(0 > rc){
            err = errno;
            break;
        }else if(0 == rc){
            if(0 == strlen(buf)){
                SpxLog1(g_log, SpxLogError, "connection is closed\n");
                err = -1;
            }
            break;
        }else{
            *len += rc;
        }

    }

    return err;
}/*}}}*/

spx_private i64_t spx_web_server_http_body_write(int fd, char *buf, size_t *len, size_t *size){/*{{{*/
    SpxErrReset;
    i64_t rc = 0;
    err_t err = 0;

    rc = write(fd, ((char *) buf) + *len, *size);

    if(0 > rc){
        err = errno;
    }else{
        *len += rc;
        *size -= rc;
    }

    return err;
}/*}}}*/

spx_private void spx_web_server_http_body_write_loop(int revents, void *arg){/*{{{*/
    struct server_context *ctx = (struct server_context *) arg;  
    if(NULL == ctx){
        SpxLog1(g_log, SpxLogError, "spx_web_server_http_body_send ctx is NULL");
        return;
    }

    if(EV_ERROR & revents){
        SpxLog1(g_log, SpxLogError, "EV_ERROR");
        return;
    }

    if(EV_TIMEOUT & revents){
      if((ctx->resp->retry++) >= RETRY_TIMES){
        spx_web_server_request_exception(ctx, spx_web_server_build_bad_request);
        SpxLog1(g_log, SpxLogError, "EV_TIMEOUT");
        return;
      }else{
        ev_once(main_socket_loop, ctx->fd, EV_WRITE, ctx->timeout, spx_web_server_http_body_write_loop, ctx);
        return;
      }
   }
  
   if(EV_WRITE & revents){
     err_t err = spx_web_server_http_body_write(ctx->fd, ctx->resp->content, 
                                                &ctx->resp->content_len, &ctx->resp->split_size);        
     if((0 == err)&&(0 == ctx->resp->split_size)){
          if(ctx->resp->content_size == ctx->resp->content_len){
                spx_web_server_request_finish(ctx);             
          }else{
            int remain_size = ctx->resp->content_size - ctx->resp->content_len;
            if(remain_size >= SPLIT_SIZE){
                ctx->resp->split_size = SPLIT_SIZE;
            }else{
                ctx->resp->split_size = remain_size;
            }

            spx_web_server_watcher_async_register(&ctx->async_watcher, spx_web_server_http_body_send, ctx);
            ev_async_start(main_socket_loop, &ctx->async_watcher);
            ev_async_send(main_socket_loop, &ctx->async_watcher);
            return;
          }
     }else{
          if((EAGAIN == err || EWOULDBLOCK == err || EINTR == err)||(ctx->resp->content_size > 0)) {
              ev_once(main_socket_loop, ctx->fd, EV_READ, ctx->timeout, spx_web_server_http_body_write_loop, ctx);
              return;
          }else{
             SpxLog2(g_log, SpxLogError, err,"spx_web_server_http_body_send Failed");
             spx_web_server_request_exception(ctx, spx_web_server_build_bad_request);
          }
     }
   }
}/*}}}*/

spx_private void spx_web_server_http_body_send(EV_P_ ev_async *watcher, int revents){/*{{{*/
    ev_async_stop(loop, watcher);
    struct server_context * ctx = (struct server_context *) watcher->data;
    if(NULL == ctx){
        SpxLog1(g_log, SpxLogError, "spx_web_server_http_body_send ctx is NULL");
        return; 
    }

    err_t err = spx_web_server_http_body_write(ctx->fd, ctx->resp->content, 
                                               &ctx->resp->content_len, &ctx->resp->split_size);        
    if((0 == err)&&(0 == ctx->resp->split_size)){
         if(ctx->resp->content_size == ctx->resp->content_len){
            spx_web_server_request_finish(ctx);             
         }else{
            int remain_size = ctx->resp->content_size - ctx->resp->content_len;
            if(remain_size >= SPLIT_SIZE){
                ctx->resp->split_size = SPLIT_SIZE;
            }else{
                ctx->resp->split_size = remain_size;
            }

            spx_web_server_watcher_async_register(&ctx->async_watcher, spx_web_server_http_body_send, ctx);
            ev_async_start(loop, &ctx->async_watcher);
            ev_async_send(loop, &ctx->async_watcher);
            return;
         }
    }else{
         if((EAGAIN == err || EWOULDBLOCK == err || EINTR == err)||(ctx->resp->content_size > 0)) {
             ev_once(main_socket_loop, ctx->fd, EV_READ, ctx->timeout, spx_web_server_http_body_write_loop, ctx);
             return;
         }else{
           SpxLog2(g_log, SpxLogError, err,"spx_web_server_http_body_send Failed");
           spx_web_server_request_exception(ctx, spx_web_server_build_bad_request);
         }
    }
}/*}}}*/

pthread_t spx_web_server_thread_new(SpxLogDelegate *log, struct server_config *conf, err_t *err){/*{{{*/
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    size_t ostack_size =0;
    g_log = log;
    pthread_attr_getstacksize(&attr, &ostack_size);
    if ( (ostack_size != conf->stacksize) && (0 != (*err = pthread_attr_setstacksize(&attr, conf->stacksize)))){
         return 0;
    }

    if(NULL == conf){
        pthread_attr_destroy(&attr);
        return 0;
    }

    pthread_t tid = 0;
    if( 0 != (*err = pthread_create(&tid, &attr, spx_web_server_main_socket_create, conf))){
        pthread_attr_destroy(&attr);
        return 0;
    }

    pthread_attr_destroy(&attr);
    return tid;
}/*}}}*/

spx_private void * spx_web_server_main_socket_create(void *arg){/*{{{*/
    struct server_config * conf = (struct server_config *) arg;
    SpxLogDelegate *log = conf->log;
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

    if( 0 != (err = spx_socket_start(mainsocket, conf->ip, conf->port,\
                                     true, conf->timeout,\
                                     3, conf->timeout,\
                                     false, 0,\
                                     true,\
                                     true, conf->timeout,
                                     1024))){
        SpxLog2(log, SpxLogError, err, "start main socket is fail.");
        goto r1;
    }

    spx_web_server_watcher_io_register(&monitor_watcher, mainsocket, spx_web_server_reciver, EV_READ, log);
    ev_io_start(main_socket_loop, &monitor_watcher); 

    ev_run(main_socket_loop, 0);
r1:
    SpxClose(mainsocket);
    return NULL;
}/*}}}*/

spx_private void spx_web_server_reciver(struct ev_loop *loop, ev_io *watcher, int revents){/*{{{*/
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
            if( EWOULDBLOCK == errno || EAGAIN == errno || EINTR == err){
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


        struct server_context * ctx = spx_web_server_ctx_pop();


        if(NULL != ctx){
            ctx->fd = client_sock;
            spx_web_server_watcher_async_register(&ctx->async_watcher, spx_web_server_recive_request, ctx);
            ev_async_start(loop, &ctx->async_watcher);
            ev_async_send(loop, &ctx->async_watcher);
        }

    }
      
    ev_io_start(loop, watcher);
}/*}}}*/

spx_private void spx_web_server_recive_request(EV_P_ ev_async *watcher, int revents){/*{{{*/
    ev_async_stop(loop, watcher);
    struct server_context *ctx = (struct server_context *) watcher->data;
    if(NULL == ctx){
        SpxLog1(g_log, SpxLogError, "recive request ctx is NULL");
        return; 
    }
    err_t err = spx_web_server_http_header_read(ctx->fd, ctx->req->header, &ctx->req->header_len);        
    if(0 == err){
        spx_web_server_watcher_async_register(&ctx->async_watcher, spx_web_server_parse_request, ctx);
        ev_async_start(loop, &ctx->async_watcher);
        ev_async_send(loop, &ctx->async_watcher);
        return;
    }else{
        if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err) {
                ev_once(loop, ctx->fd, EV_READ, ctx->timeout, spx_web_server_recive_request_loop, ctx);
                return;
        }else{
            SpxLog2(g_log, SpxLogError, err,"spx_web_server_http_header_read Failed");
            if( -1 == err)
                spx_web_server_ctx_close(ctx);
            else
                spx_web_server_request_exception(ctx, spx_web_server_build_bad_request);
            return;
        }
    }
}/*}}}*/

spx_private void spx_web_server_recive_request_loop(int revents, void *arg){/*{{{*/
    struct server_context *ctx = (struct server_context *) arg;
    if(NULL == ctx){
        SpxLog1(g_log, SpxLogError, "spx_web_server_recive_request_loop ctx is NULL");
        return;
    }

    if(EV_ERROR & revents){
        SpxLog1(g_log, SpxLogError, "EV_ERROR");
        return;
    }
    
    if(EV_TIMEOUT & revents){
      if((ctx->req->retry++) >= RETRY_TIMES){
        spx_web_server_request_exception(ctx, spx_web_server_build_bad_request);
        SpxLog1(g_log, SpxLogError, "EV_TIMEOUT");
        return;
      }else{
        ev_once(main_socket_loop, ctx->fd, EV_READ, ctx->timeout, spx_web_server_recive_request_loop, ctx);
        return;
      }
   }
  
   if(EV_READ & revents){
     err_t err = spx_web_server_http_header_read(ctx->fd, ctx->req->header, &ctx->req->header_len);        
     if(0 == err){
         spx_web_server_watcher_async_register(&ctx->async_watcher, spx_web_server_parse_request, ctx);
         ev_async_start(main_socket_loop, &ctx->async_watcher);
         ev_async_send(main_socket_loop, &ctx->async_watcher);
     }else{
          if((EAGAIN == err || EWOULDBLOCK == err || EINTR == err)) {
              ev_once(main_socket_loop, ctx->fd, EV_READ, ctx->timeout, spx_web_server_recive_request_loop, ctx);
              return;
          }else{
              SpxLog2(g_log, SpxLogError, err,"spx_web_server_http_header_read Failed");
              if( -1 == err)
                  spx_web_server_ctx_close(ctx);
              else
                  spx_web_server_request_exception(ctx, spx_web_server_build_bad_request);
              return;
          }
     }
   }

}/*}}}*/

//TODO parse request into param_set
spx_private void spx_web_server_parse_request(EV_P_ ev_async *watcher, int revents){/*{{{*/
    ev_async_stop(loop, watcher);
    err_t err = 0;
    struct server_context *ctx = (struct server_context *) watcher->data;
    if(NULL == ctx){
        SpxLog1(g_log, SpxLogError, "spx_web_server_handle_request ctx is NULL");
        return; 
    }

    if(NULL != ctx->req->header){
        //TODO parse request, set content_length for content recive
        size_t i = 0, j = 0;
        char *buf = (char *)ctx->req->header;
        char request_line[HTTP_MAX_HEADER_SIZE] = {0};

        if(ctx->req->header_len == 0){
            spx_web_server_ctx_close(ctx);
            SpxLog1(g_log, SpxLogError, "client socket is closed");
            return;
        }

        char *token = strtok(ctx->req->header, "\r\n");
        strncpy(request_line, token, strlen(token));

        while (token != NULL){
            if (strstr(token, "Content-Length") != NULL){
                ctx->req->content_len = atoi(token + 16);
                break;
            }
            token = strtok(NULL, "\r\n");
        }

        char method[32] = {0};
        char URI[HTTP_MAX_HEADER_SIZE]= {0};
        token = strtok(request_line, " ");
        strncpy(method, token, strlen(token));

        while (token != NULL){
            token = strtok(NULL, " ");
            strncpy(URI, token, strlen(token));
            break;
        }

        token = strtok(URI+2, "&");  
        while (token != NULL){
            char key[1024] = {0};
            char value[1024] = {0};

            int len = strlen(token);
            int idx = 0;
            int flag = 0;
            int i = 0;
            int key_len = 0, val_len = 0;

            for (idx = 0; idx < len; idx++){
                if ('=' == *(token + idx)){
                    flag = 1;
                    key_len = i;
                    i = 0;
                    continue;
                }

                if (0 == flag){
                    key[i] = *(token + idx);
                    i++;
                } else {
                    value[i] = *(token + idx);
                    i++;
                }
            }

            val_len = i;
            spx_web_server_param_set_insert(ctx->req, key, key_len, value, val_len);
            token = strtok(NULL, "&");
        }

        if(strcasecmp(method, "GET") == 0){
            spx_web_server_watcher_async_register(&ctx->async_watcher,spx_web_server_handle_request, ctx);
            ev_async_start(loop, &ctx->async_watcher);
            ev_async_send(loop, &ctx->async_watcher);
            return;
        }

        if(0 == strcasecmp(method, "POST")){
            spx_web_server_watcher_async_register(&ctx->async_watcher,spx_web_server_recive_content, ctx);
            ev_async_start(loop, &ctx->async_watcher);
            ev_async_send(loop, &ctx->async_watcher);
            return;
        }

        spx_web_server_request_exception(ctx, spx_web_server_build_bad_request);
   } else{
        spx_web_server_request_exception(ctx, spx_web_server_build_bad_request);
   }
}/*}}}*/

spx_private void spx_web_server_recive_content(EV_P_ ev_async *watcher, int revents){/*{{{*/
    ev_async_stop(loop, watcher);
    struct server_context * ctx = (struct server_context *) watcher->data;
    if(NULL == ctx){
        SpxLog1(g_log, SpxLogError, "recive content ctx is NULL");
        return; 
    }
    err_t err = spx_web_server_http_content_read(ctx->fd, ctx->req->content,
                                                 &ctx->req->content_len, ctx->req->content_size);        
    if(0 == err){
        spx_web_server_watcher_async_register(&ctx->async_watcher, spx_web_server_handle_request, ctx);
        ev_async_start(loop, &ctx->async_watcher);
        ev_async_send(loop, &ctx->async_watcher);
        return;
    }else{
        if(EAGAIN == err || EWOULDBLOCK == err || EINTR == err) {
                ev_once(loop, ctx->fd, EV_READ, ctx->timeout, spx_web_server_recive_content_loop, ctx);
                return;
        }else{
            SpxLog2(g_log, SpxLogError, err,"spx_web_server_http_header_read Failed");
            if( -1 == err)
                spx_web_server_ctx_close(ctx);
            else
                spx_web_server_request_exception(ctx, spx_web_server_build_bad_request);
            return;
        }
    }
}/*}}}*/

spx_private void spx_web_server_recive_content_loop(int revents, void *arg){/*{{{*/
    struct server_context *ctx = (struct server_context *) arg;
    if(NULL == ctx){
        SpxLog1(g_log, SpxLogError, "spx_web_server_recive_content_loop ctx is NULL");
        return;
    }

    if(EV_ERROR & revents){
        SpxLog1(g_log, SpxLogError, "EV_ERROR");
        return;
    }
    
    if(EV_TIMEOUT & revents){
      if((ctx->req->retry++) >= RETRY_TIMES){
        spx_web_server_request_exception(ctx, spx_web_server_build_bad_request);
        SpxLog1(g_log, SpxLogError, "EV_TIMEOUT");
        return;
      }else{
        ev_once(main_socket_loop, ctx->fd, EV_READ, ctx->timeout, spx_web_server_recive_content_loop, ctx);
        return;
      }
   }
  
   if(EV_READ & revents){
     err_t err = spx_web_server_http_header_read(ctx->fd, ctx->req->header, &ctx->req->header_len);        
     if(0 == err){
         spx_web_server_watcher_async_register(&ctx->async_watcher, spx_web_server_handle_request, ctx);
         ev_async_start(main_socket_loop, &ctx->async_watcher);
         ev_async_send(main_socket_loop, &ctx->async_watcher);
     }else{
          if((EAGAIN == err || EWOULDBLOCK == err || EINTR == err)) {
              ev_once(main_socket_loop, ctx->fd, EV_READ, ctx->timeout, spx_web_server_recive_content_loop, ctx);
              return;
          }else{
              SpxLog2(g_log, SpxLogError, err,"spx_web_server_http_header_read Failed");
              if( -1 == err)
                  spx_web_server_ctx_close(ctx);
              else
                  spx_web_server_request_exception(ctx, spx_web_server_build_bad_request);
              return;
          }
     }
   }

}/*}}}*/

spx_private void spx_web_server_handle_request(EV_P_ ev_async *watcher, int revents){/*{{{*/
    ev_async_stop(loop, watcher);
    err_t err = 0;
    struct server_context * ctx = (struct server_context *) watcher->data;
    if(NULL == ctx){
        SpxLog1(g_log, SpxLogError, "spx_web_server_handle_request ctx is NULL");
        return; 
    }
   if(NULL != ctx->req){

        //call custom service by user
        ctx->resp->content =  ctx->ServerHandler(ctx->req);
        if (NULL == ctx->resp->content){
            spx_web_server_request_exception(ctx, spx_web_server_build_bad_request);
            SpxLog1(g_log, SpxLogError, "response content is NULL");
            return;
        }

        spx_web_server_watcher_async_register(&ctx->async_watcher,spx_web_server_send_response, ctx);
        ev_async_start(loop, &ctx->async_watcher);
        ev_async_send(loop, &ctx->async_watcher);
   }else{
        spx_web_server_request_exception(ctx, spx_web_server_build_bad_request);
   }
}/*}}}*/

spx_private void spx_web_server_send_response(EV_P_ ev_async *watcher, int revents){/*{{{*/
        ev_async_stop(loop, watcher);
        err_t err = 0;
        struct server_context *ctx = (struct server_context *) watcher->data;
        if(NULL == ctx){
            SpxLog1(g_log, SpxLogError, "spx_web_server_send_response ctx is NULL");
            return; 
        }

        if(NULL != ctx->resp){
            if (ctx->resp->content != NULL)
                ctx->resp->content_size = strlen(ctx->resp->content);
            spx_web_server_build_header(ctx->fd, ctx->resp->content_size);

            int remain_size = ctx->resp->content_size - ctx->resp->content_len;
            if(remain_size >= SPLIT_SIZE){
                ctx->resp->split_size = SPLIT_SIZE;
            }else{
                ctx->resp->split_size = remain_size;
            }

            spx_web_server_watcher_async_register(&ctx->async_watcher, spx_web_server_http_body_send, ctx);
            ev_async_start(loop, &ctx->async_watcher);
            ev_async_send(loop, &ctx->async_watcher);
        }else{
            spx_web_server_request_exception(ctx, spx_web_server_build_bad_request);
        }
}/*}}}*/

spx_private void spx_web_server_request_finish(struct server_context *ctx){/*{{{*/
        spx_web_server_ctx_close(ctx);
}/*}}}*/

spx_private void spx_web_server_request_exception(struct server_context * ctx, void (*handle)(int client)){/*{{{*/
        handle(ctx->fd);
        spx_web_server_ctx_close(ctx);
}/*}}}*/

spx_private void spx_web_server_ctx_close(struct server_context *ctx){/*{{{*/
        SpxClose(ctx->fd);
        spx_web_server_ctx_push(ctx);
}/*}}}*/

/**********************************************************************/
/* Inform the client that the requested web method has not been
 * implemented.
 * Parameter: the client socket */
/**********************************************************************/
spx_private void spx_web_server_build_unimplement(int client)/*{{{*/
{
 char head[1024]={0};
 char body[1024]={0};
 char buf[4096]={0};
 char tmp[1024]={0};
 int content_length = 0;

 snprintf(tmp, sizeof(tmp), "<HTML><HEAD><TITLE>Method Not Implemented\r\n");
 strncpy(body, tmp, strlen(tmp));
 snprintf(tmp, sizeof(tmp), "</TITLE></HEAD>\r\n");
 strncat(body, tmp, strlen(tmp));
 snprintf(tmp, sizeof(tmp), "<BODY><P>HTTP request method not supported.\r\n");
 strncat(body, tmp, strlen(tmp));
 snprintf(tmp, sizeof(tmp), "</BODY></HTML>\r\n");
 strncat(body, tmp, strlen(tmp));

 strncpy(buf, body, strlen(body));

 content_length = strlen(body);

 spx_web_server_build_header(client, content_length);
 send(client, body, strlen(body), 0);
}/*}}}*/

/**********************************************************************/
/* Return the informational HTTP header about a file. */
/* Parameters: the socket to print the header on
 *             the name of the file */
/**********************************************************************/
spx_private void spx_web_server_build_header(int client, int content_length)/*{{{*/
{
    char head[1024]={0};
    char tmp[1024]={0};

    sprintf(tmp, "HTTP/1.1 200 OK\r\n");
    strncpy(head, tmp,  strlen(tmp));
    sprintf(tmp, SERVER_INFO);
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
/* Inform the client that a request it has made has a problem.
 * Parameters: client socket */
/**********************************************************************/
spx_private void spx_web_server_build_bad_request(int client)/*{{{*/
{
    char head[1024]={0};
    char tmp[1024]={0};
    char body[1024] = {"<HTML><BODY><P>Your browser sent a bad request, such as a \
                                    POST without a Content-Length.</HTML></BODY>\r\n"};
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

err_t spx_web_server_start(char *(*ServerHandler)(struct request *req), struct server_config *conf){/*{{{*/
    err_t err = 0;
    if(NULL == conf){
        SpxLog1(g_log,SpxLogError, "start server failed. server config is NULL");
        return -1;
    }

    if(0 != (err = spx_web_server_ctx_new_pool(conf, ServerHandler)) ){
        SpxLog1(g_log, SpxLogError, "spx_web_server_ctx_new_pool failed.");
        return err;
    }

    pthread_t tid = spx_web_server_thread_new(g_log, conf, &err);
    if(0 != err){
        SpxLog2(g_log, SpxLogError, err,
                "create main socket thread is fail."
                "and will exit...");
        return err;
    }

    pthread_join(tid,NULL);
    return 0;
}/*}}}*/

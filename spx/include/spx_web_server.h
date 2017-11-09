/*************************************************************************
    > File Name: spx_web_server.h
    > Author: shuaixiang
    > Mail: shuaixiang@yuewen.com
    > Created Time: Fri 27 Nov 2015 08:11:59 AM UTC
 ************************************************************************/
#ifndef SPX_WEB_SERVER_H
#define SPX_WEB_SERVER_H
#ifdef __cplusplus
extern "C" {
#endif

#include "ev.h"
#include "spx_types.h"

#define MAX_LEN 1024
#define HTTP_HEADER_SIZE 1024

struct server_config{
    SpxLogDelegate *log;
    u64_t stacksize;
    string_t ip;
    i32_t port;
    u32_t timeout;
    u32_t retry_times;
    u32_t context_pool_size;
};

struct param_set_node{
    char key[MAX_LEN];
    char value[MAX_LEN];
    struct param_set_node *next_param;
};

struct param_set{
    struct param_set_node *param_head;
    struct param_set_node *param_tail;
};

struct request{
    size_t retry;
    size_t header_len;
    size_t header_size;
    size_t content_len;
    size_t content_size;
    struct param_set *p_set;

    char header[HTTP_HEADER_SIZE];
    char *content;
};

struct response{
    size_t retry;
    size_t header_len;
    size_t header_size;
    size_t split_size;
    size_t content_len;
    size_t content_size;

    char header[HTTP_HEADER_SIZE];
    char *content;
};

struct server_context{
    char life_cycle;
    int fd;
    ev_tstamp timeout;
    ev_async async_watcher;
    struct server_context *p_next_ctx;
    struct server_context *p_pre_ctx;
    char *(*ServerHandler)(struct request *req); 

    struct request *req;
    struct response *resp;
};

err_t spx_web_server_start(char *(*ServerHandler)(struct request *req), struct server_config *conf);
struct param_set_node* spx_web_server_param_set_get(struct request *req, char *key);

#ifdef __cplusplus
}
#endif
#endif

/*************************************************************************
    > File Name: ydb_tracker_monitor.h
    > Author: shuaixiang
    > Mail: 
    > Created Time: Fri 27 Nov 2015 08:11:59 AM UTC
 ************************************************************************/
#ifndef _YDB_TRACKER_MONITOR_H_
#define _YDB_TRACKER_BALANCE_H_
#ifdef __cplusplus
extern "C"{
#endif

#include "spx_types.h"
#include "spx_io.h"
#include "IdcreatorConfig.h"

struct param_node{
    char key[100];
    char value[100];
    struct param_node *next_param;
};

struct monitor_http_context{
    struct IdcreatorConfig *c;
    char life_cycle;
    int fd;
    ev_tstamp timeout;
    ev_async async_watcher;
    struct monitor_http_context *p_nextCTX;
    struct monitor_http_context *p_preCTX;

    //request
    size_t req_retry;
    size_t req_len;
    size_t req_size; 
    byte_t *request;
    struct param_node *param_head;
    struct param_node *param_tail;

    //response
    size_t resp_retry;
    size_t resp_size;
    size_t split_size;
    byte_t *response;
    size_t resp_len;
};

pthread_t NewIdCreatorHttp(SpxLogDelegate *log, struct IdcreatorConfig *c, err_t *err);

//pthread_t ydb_tracker_monitor_thread_new(SpxLogDelegate *log, struct ydb_tracker_configurtion *c, err_t *err);
//
//spx_private struct param_node* InsertParam(struct monitor_http_context* mtr_ctx, char *key, int key_len, char *val, int val_len);
//
//spx_private struct param_node* GetParam(struct monitor_http_context *mtr_ctx, char *key);
//
//spx_private err_t FreeParam(struct monitor_http_context *mtr_ctx);
//
//spx_private void * ydb_tracker_monitor_mainsocket_create(void *arg);
//
//spx_private err_t NewMonitorCTXPool(int pool_size);
//
//spx_private void FreeMonitorCTXPool();
//
//spx_private void ydb_tracker_monitor_io_watcher_register(ev_io *watcher ,int fd, void (*cb)(struct ev_loop *loop, ev_io *watcher, int revents), int revents, void *data);
//
//spx_private void ydb_tracker_monitor_socket_response_SendResponse(EV_P_ ev_async *watcher, int revents);
//
//spx_private void ydb_tracker_monitor_async_watcher_register(ev_async *watcher, void(*cb)(struct ev_loop *loop, ev_async *watcher, int revents), void *data);
//
//spx_private void ydb_tracker_monitor_socket_main_reciver(struct ev_loop *loop, ev_io *watcher, int revents);
//
//spx_private void ydb_tracker_monitor_socket_request(struct ev_loop *loop, ev_async *watcher, int revents);
//
//spx_private int ydb_tracker_monitor_socket_request_GetRequest_ReadRequest_IsEnd(byte_t* buf, size_t len);
//
//spx_private err_t ydb_tracker_monitor_socket_request_GetRequest_ReadRequest(int fd, byte_t *buf, size_t *len);
//
//spx_private void ydb_tracker_monitor_socket_request_GetRequest(int revents, void *arg);
//
//spx_private void ydb_tracker_monitor_socket_parser(EV_P_ ev_async *watcher, int revents);
//
//spx_private void bad_request(int client);
//
//spx_private void headers(int client, int content_length);
//
//spx_private void unimplemented(int client);
//
//spx_private struct storage_list *ydb_tracker_monitor_socket_response_visitor(string_t groupname);
//
//spx_private void ydb_tracker_monitor_socket_response(EV_P_ ev_async *watcher, int revents);
//
//spx_private char * ydb_tracker_monitor_socket_response_Storage2JSON(struct ydb_remote_storage * storage);
//
//spx_private void ydb_tracker_monitor_socket_response_FreeStorageList(struct storage_list *list);
//
//spx_private void RequestFinish(struct monitor_http_context *mtr_ctx);
//
//spx_private void RequestException(struct monitor_http_context * mtr_ctx);
//
//spx_private void CloseMonitor(struct monitor_http_context *mtr_ctx);

#ifdef _cplusplus
}
#endif
#endif

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
#include <ev.h>

#include "spx_types.h"
#include "spx_string.h"
#include "spx_defs.h"
#include "spx_socket.h"
#include "spx_notifier_module.h"
#include "spx_module.h"
#include "spx_job.h"
#include "spx_socket_accept.h"
#include "spx_time.h"

spx_private ev_io main_watcher;
spx_private void spx_socket_main_reciver(struct ev_loop *loop,
        ev_io *watcher,int revents);

void spx_socket_accept_nb(SpxLogDelegate *log,
        struct ev_loop *loop,int fd){
    SpxZero(main_watcher);
    ev_io_init(&main_watcher,spx_socket_main_reciver,fd,EV_READ);
    main_watcher.data = log;
     ev_io_start(loop,&(main_watcher));
    ev_run(loop,0);
}

spx_private void spx_socket_main_reciver(struct ev_loop *loop,ev_io *watcher,int revents){
    ev_io_stop(loop,watcher);
    SpxLogDelegate *log = (SpxLogDelegate *) watcher->data;
    err_t err = 0;
    while(true){
        struct sockaddr_in client_addr;
        unsigned int socket_len = 0;
        int client_sock = 0;
        socket_len = sizeof(struct sockaddr_in);
        client_sock = accept(watcher->fd, (struct sockaddr *) &client_addr,
                &socket_len);
        if (0 > client_sock) {
            if (EWOULDBLOCK == errno || EAGAIN == errno) {
                break;
            }
            break;
        }

        if (0 == client_sock) {
            break;
        }

        size_t idx = client_sock % g_spx_notifier_module->threadpool->size;
        struct spx_job_context *jc =  spx_job_pool_pop(g_spx_job_pool,&err);
        if(NULL == jc){
            SpxClose(client_sock);
            SpxLog1(log,SpxLogError,\
                    "pop nio context is fail.");
            break;
        }

        SpxLogFmt1(log,SpxLogDebug,"recv socket conntect.wakeup notifier module idx:%d.jc idx:%d."
                ,idx,jc->idx);

        jc->fd = client_sock;
        jc->request_timespan = spx_now();


        struct spx_thread_context *tc = spx_get_thread(g_spx_notifier_module,idx);
        jc->tc = tc;
//        spx_module_dispatch(tc,spx_notifier_module_wakeup_handler, jc);
//        spx_notifier_module_wakeup_handler(EV_WRITE,jc);
        SpxModuleDispatch(spx_notifier_module_wakeup_handler,jc);
    }
    ev_io_start(loop,watcher);
}




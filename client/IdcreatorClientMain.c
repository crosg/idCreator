/*************************************************************
 *                     _ooOoo_
 *                    o8888888o
 *                    88" . "88
 *                    (| -_- |)
 *                    O\  =  /O
 *                 ____/`---'\____
 *               .'  \\|     |//  `.
 *              /  \\|||  :  |||//  \
 *             /  _||||| -:- |||||-  \
 *             |   | \\\  -  /// |   |
 *             | \_|  ''\---/''  |   |
 *             \  .-\__  `-`  ___/-. /
 *           ___`. .'  /--.--\  `. . __
 *        ."" '<  `.___\_<|>_/___.'  >'"".
 *       | | :  `- \`.;`\ _ /`;.`/ - ` : | |
 *       \  \ `-.   \_ __\ /__ _/   .-` /  /
 *  ======`-.____`-.___\_____/___.-`____.-'======
 *                     `=---='
 *  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *           佛祖保佑       永无BUG
 *
 * ==========================================================================
 *
 * this software or lib may be copied only under the terms of the gnu general
 * public license v3, which may be found in the source kit.
 *
 *       Filename:  IdcreatorClientMain.c
 *        Created:  2015年04月09日 13时40分32秒
 *         Author:  Seapeak.Xu (www.94geek.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 ***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "spx_types.h"
#include "spx_thread.h"
#include "spx_periodic.h"

#include "IdcreatorClient.h"
static char *ip = NULL;
static int port = 0;

static void *callback(void *arg);
int main(int argc,char **argv) {
    if(4 != argc){
        printf("please use this cmd: idcreatorclient ip port type\n");
        return 0;
    }


    ip = argv[1];
    port = atoi(argv[2]);
    int type = atoi(argv[3]);
    err_t err = 0;

    pthread_t t0 = spx_thread_new(NULL,128 * 1024,callback,&type,&err);
    /*
    pthread_t t1 = spx_thread_new(NULL,128 * 1024,callback,&type,&err);
    pthread_t t2 = spx_thread_new(NULL,128 * 1024,callback,&type,&err);
    pthread_t t3 = spx_thread_new(NULL,128 * 1024,callback,&type,&err);
    pthread_t t4 = spx_thread_new(NULL,128 * 1024,callback,&type,&err);
    pthread_t t5 = spx_thread_new(NULL,128 * 1024,callback,&type,&err);
    pthread_t t6 = spx_thread_new(NULL,128 * 1024,callback,&type,&err);
    pthread_t t7 = spx_thread_new(NULL,128 * 1024,callback,&type,&err);
    pthread_t t8 = spx_thread_new(NULL,128 * 1024,callback,&type,&err);
    pthread_t t9 = spx_thread_new(NULL,128 * 1024,callback,&type,&err);
  */

    spx_periodic_sleep(600000,0);
    return 0;
}

static void *callback(void *arg){
    err_t err = 0;
    int *type = (int *) arg;
    while(true) {
        u64_t id =  idcreatorClientMakeId(ip,port,*type,&err);
        printf("%ld \n",id);
    }
    return NULL;
}




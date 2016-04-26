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
 *       Filename:  IdcreatorClient.c
 *        Created:  2015年04月09日 13时32分49秒
 *         Author:  Seapeak.Xu (www.94geek.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 ***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


#include "spx_types.h"
#include "spx_socket.h"
#include "spx_alloc.h"
#include "spx_io.h"
#include "spx_message.h"

#include "IdcreatorProtocol.h"

struct IdcreatorClientContext{
    int fd;
    struct spx_msg_header inheader;
    struct spx_msg_header outheader;
    char inbuf[SpxMsgHeaderSize + sizeof(u64_t)];
    char outbuf[SpxMsgHeaderSize + sizeof(u32_t)];
};

u64_t idcreatorClientMakeId(char *ip,int port,int type,err_t *err){
    struct IdcreatorClientContext *icc = spx_alloc_alone(sizeof(*icc),err);
    if(!icc) return 0;
    u64_t id = 0;

    icc->fd = spx_socket_new(err);
    if (*err) return 0;

    *err = spx_socket_set(icc->fd,true,3,3,3,false,0,false,true,2);
    if(*err) goto end;
    *err = spx_socket_connect_nb(icc->fd,ip,port,2);
    if(*err) goto end;

    icc->outheader.protocol = IdcreatorMakeId;
    icc->outheader.version = IdcreatorVersion;
    icc->outheader.bodylen = sizeof(u32_t);

    spx_header_pack(icc->outbuf,&(icc->outheader));
    spx_msg_i2b((uchar_t *) icc->outbuf + SpxMsgHeaderSize,type);

    size_t len = 0;
    *err = spx_write_nb(icc->fd,(byte_t *) icc->outbuf,SpxMsgHeaderSize + sizeof(u32_t),&len);
    if(!spx_socket_read_timeout(icc->fd,20)){
        *err = EBUSY;
        goto end;
    }
    len = 0;
    *err = spx_read_nb(icc->fd,(byte_t *) icc->inbuf,SpxMsgHeaderSize + sizeof(u64_t),&len);
    spx_header_unpack(icc->inbuf,&(icc->inheader));
    if(icc->inheader.err){
        *err = icc->inheader.err;
        goto end;
    }
    if(IdcreatorMakeId != icc->inheader.protocol){
        *err = ENODEV;
        goto end;
    }
    id = spx_msg_b2ul((uchar_t *) icc->inbuf + SpxMsgHeaderSize);

end:
    if(icc->fd) close(icc->fd);
    SpxFree(icc);
    return id;
}


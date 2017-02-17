// =====================================================================================
//
//       Filename:  IdcreatorStateKeep.c
//
//    Description:
//
//        Version:  1.0
//        Created:  2016年11月28日 18时38分30秒
//       Revision:  none
//       Compiler:  g++
//
//         Author:  Seapeak.Xu (www.94geek.com), xvhfeng@gmail.com
//        Company:
//
// =====================================================================================

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "spx_types.h"
#include "spx_defs.h"
#include "spx_string.h"
#include "spx_message.h"
#include "spx_io.h"
#include "spx_path.h"

#include "IdcreatorConfig.h"



u32_t contract_next = 0;
static int fd = 0;
static char *mptr = NULL;

err_t idcreator_statef_open(
        struct IdcreatorConfig *c
        ){
    err_t err = 0;
    bool_t fexist = SpxFileExist(c->id_state_file);
    if(!fexist){
        string_t basepath = spx_basepath(c->id_state_file,&err);
        if(NULL == basepath){
            SpxLogFmt2(c->log,SpxLogError,err,\
                    "creator finder for id-state-file:%s is fail.",
                    c->id_state_file);
            return err;
        }

        err = spx_mkdir(c->log,basepath,0x777);
        SpxStringFree(basepath);
        if(0 != err){
            SpxLogFmt2(c->log,SpxLogError,err,\
                    "creator finder for id-state-file:%s is fail.",
                    c->id_state_file);
            return err;
        }
    }

    fd = open(c->id_state_file,
            O_RDWR|O_CREAT,SpxFileMode);
    if(0 >= fd){
        err = errno;
        SpxLogFmt2(c->log,SpxLogError,err,\
                "open id state file:%s is fail.",
                c->id_state_file);
        return err;
    }

    if(!fexist) {
        if(0 != (err = ftruncate(fd,16))){
            err = errno;
            SpxLogFmt2(c->log,SpxLogError,err,\
                    "truncate id state file:%s to size:%lld is fail.",
                    c->id_state_file,16);
            SpxClose(fd);
            return err;
        }
    }
    mptr = mmap(NULL,\
            16,PROT_READ | PROT_WRITE ,\
            MAP_SHARED,fd,0);
    if(MAP_FAILED == mptr){
        err = errno;
        SpxLogFmt2(c->log,SpxLogError,err,\
                "mmap the id state file file:%s to memory is fail.",
                c->id_state_file);
        SpxClose(fd);
        return err;
    }
    if(fexist){
        uchar_t buff[4] = {0};
        memcpy(buff,mptr,sizeof(int));
        contract_next = spx_msg_b2i(buff);
    }
    return 0 ;
}

void idcreator_statf_sync(int next){
    uchar_t buff[4] = {0};
    spx_msg_i2b(buff,next);
    memcpy(mptr,buff,sizeof(int));
    msync(mptr,sizeof(int),MS_SYNC);
}

void idcreator_statf_close(){
    if(NULL != mptr) {
        munmap(mptr,16);
        mptr = NULL;
    }
    if(0 != fd) {
        SpxClose(fd);
    }
}


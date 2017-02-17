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
 *       Filename:  idcreatorDispatcher.c
 *        Created:  2015年03月03日 15时38分56秒
 *         Author:  Seapeak.Xu (www.94geek.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 ***********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "spx_types.h"
#include "spx_string.h"
#include "spx_defs.h"
#include "spx_log.h"
#include "spx_module.h"
#include "spx_alloc.h"
#include "spx_periodic.h"
#include "spx_atomic.h"
#include "spx_time.h"

#include "IdcreatorConfig.h"
#include "IdcreatorServerContext.h"
#include "IdcreatorProtocol.h"
#include "IdcreatorStateKeep.h"


//static u32_t bitSeed = 0;
static u32_t seed[0x400] = {0};
static u32_t curr_time = 0;

u64_t IdGenerator(int type, int mid, err_t *err){
    u64_t id = 0l;
    time_t token = spx_get_token();
    switch(type){
//        printf("type:%d \n",type);
        case 0x4FF ://specific one,for contract in yuewen company
            {
                int days =  spx_get_tokendays();
//                printf("day:%d \n",days);
                int next = 0;
                do{
                    if(10000 <= contract_next)
                        SpxAtomicVCas(contract_next,10000,0);
                    next = SpxAtomicLazyVIncr(contract_next);
                } while(10000 <= contract_next);
                idcreator_statf_sync(next);
                id =(u64_t)  (days * pow(10,5) + next * 10 + mid);
//                printf("id:%ld \n",id);
                break;
            }
        case 0://book
        case 1://author
        case 2: {//configurtion
                    //free the last 2 bit,for client use by itself
                    int next = 0;
                    do{
                        if(10000 <= seed[type])
                            SpxAtomicVCas(seed[type],10000,0);
                        next = SpxAtomicLazyVIncr(seed[type]);
                    } while(10000 <= seed[type]);
                    id =(u64_t) ((((u64_t) (0xFFFFFFFF & token)) * pow(10,8))
                            + (((u64_t) (0xF & mid)) * pow(10,6))
                            + next * 100);
                    break;
                }
        case 98:{
                    int next = 0;
                    do{
                        if(curr_time == token){
                            next = SpxAtomicVIncr(seed[type]);
                        } else {
                            SpxAtomicVSet(curr_time,token);
                            SpxAtomicVSet(seed[type],0);
                        }
                    } while(10000 <= seed[type]);
                    id =(u64_t) ((((u64_t) (0xFFFFFFFF & token)) * pow(10,8))
                            + (((u64_t) (0xF & mid)) * pow(10,6))
                            + (type * pow(10,4))
                            + next );
                    break;
                }
        case 99:{//incr in the type,but now not used
                    int next = 0;
                    do{
                        if(10000 <= seed[type])
                            SpxAtomicVCas(seed[type],10000,0);
                        next = SpxAtomicLazyVIncr(seed[type]);
                    } while(10000 <= seed[type]);
                    id =(u64_t) ((((u64_t) (0xFFFFFFFF & token)) * pow(10,8))
                            + (((u64_t) (0xF & mid)) * pow(10,6))
                            + (type * pow(10,4))
                            + next );
                    break;
                }
        default://all other
                {
                    int next = 0;
                    do{
                        if(0x3FFF <= seed[type])
                            SpxAtomicVCas(seed[type],0x3fff,0);
                        next = SpxAtomicLazyVIncr(seed[type]);
                    } while(0x3FFF <= seed[type]);
                    id = (u64_t) (
                            ((u64_t) (0xFFFFFFFF & token) << 28)
                            | (((u64_t) (0xF & mid)) << 24 )
                            | (((u64_t) (0x3FF & type)) << 14)
                            | (((u64_t) (0x3FFF & next)))
                            );
                    break;
                }
    }

    return id;
}





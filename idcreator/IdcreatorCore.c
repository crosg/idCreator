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
 volatile int seed_99 = 0;
 volatile int seed_98 = 0;
volatile  u32_t curr_time = 0;

u64_t IdGenerator(int type, int mid, err_t *err){
    volatile u64_t id = 0l;
    time_t token = spx_get_token();
    switch(type){
        case 0x4FF ://specific one,for contract in yuewen company
            {
                int days =  spx_get_tokendays();
                    volatile int next = 0;
                do{
                    if(10000 <= contract_next)
                        SpxAtomicVCas(contract_next,10000,0);
                    next = SpxAtomicLazyVIncr(contract_next);
                } while(10000 <= contract_next);
                idcreator_statf_sync(next);
                id =(u64_t)  (days * pow(10,5) + next * 10 + mid);
                break;
            }
        case 0://book
        case 1://author
        case 2://configurtion
        case 50:// for qidian
            {
                    //free the last 2 bit,for client use by itself
                    volatile int next = 0;
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
                    volatile int next = 0;
                    do{
                        if(curr_time == token){
                            next = __sync_add_and_fetch(&seed_98,1);
                        } else {
                            SpxAtomicVSet(curr_time,token);
                            SpxAtomicVSet(seed_98,0);
                        }
                    } while(10000 <= seed_98);
                    id =(u64_t) ((((u64_t) (0xFFFFFFFF & token)) * pow(10,8))
                            + (((u64_t) (0xF & mid)) * pow(10,6))
                            + (u64_t) (type * pow(10,4)))
                            + (u64_t) next ;
                    break;
                }
        case 99:{//incr in the type,but now not used
                    volatile int next = 0;
                    do{
                        if(10000 <= seed_99)
                            SpxAtomicVCas(seed_99,10000,0);
                        next = SpxAtomicLazyVIncr(seed_99);
                    } while(10000 <= seed_99);
                    id =(u64_t) ((((u64_t) (0xFFFFFFFF & token)) * pow(10,8))
                            + (((u64_t) (0xF & mid)) * pow(10,6))
                            + (type * pow(10,4)))
                            + next ;
                    break;
                }
        default://all other
                {
                    volatile int next = 0;
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





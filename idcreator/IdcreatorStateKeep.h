// =====================================================================================
//
//       Filename:  IdcreatorStateKeep.h
//
//    Description:
//
//        Version:  1.0
//        Created:  2016年11月28日 18时38分33秒
//       Revision:  none
//       Compiler:  g++
//
//         Author:  Seapeak.Xu (www.94geek.com), xvhfeng@gmail.com
//        Company:
//
// =====================================================================================
#ifndef _IDCREATORSTATEKEEP_H_
#define _IDCREATORSTATEKEEP_H_
#ifdef __cplusplus
extern "C" {
#endif

extern u32_t contract_next;


err_t idcreator_statef_open(
    struct IdcreatorConfig *c
    );
void idcreator_statf_sync(int next);
void idcreator_statf_close();

#ifdef __cplusplus
}
#endif
#endif

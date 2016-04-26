/***********************************************************************
 *                              _ooOoo_
 *                             o8888888o
 *                             88" . "88
 *                             (| -_- |)
 *                              O\ = /O
 *                          ____/`---'\____
 *                        .   ' \\| |// `.
 *                         / \\||| : |||// \
 *                       / _||||| -:- |||||- \
 *                         | | \\\ - /// | |
 *                       | \_| ''\---/'' | |
 *                        \ .-\__ `-` ___/-. /
 *                     ___`. .' /--.--\ `. . __
 *                  ."" '< `.___\_<|>_/___.' >'"".
 *                 | | : `- \`.;`\ _ /`;.`/ - ` : | |
 *                   \ \ `-. \_ __\ /__ _/ .-` / /
 *           ======`-.____`-.___\_____/___.-`____.-'======
 *                              `=---='
 *           .............................................
 *                    佛祖镇楼                  BUG辟易
 *            佛曰:
 *                    写字楼里写字间，写字间里程序员；
 *                    程序人员写程序，又拿程序换酒钱。
 *                    酒醒只在网上坐，酒醉还来网下眠；
 *                    酒醉酒醒日复日，网上网下年复年。
 *                    但愿老死电脑间，不愿鞠躬老板前；
 *                    奔驰宝马贵者趣，公交自行程序员。
 *                    别人笑我忒疯癫，我笑自己命太贱；
 *                    不见满街漂亮妹，哪个归得程序员？
 * ==========================================================================
 *
 * this software or lib may be copied only under the terms of the gnu general
 * public license v3, which may be found in the source kit.
 *
 *       Filename:  spx_limits.h
 *        Created:  2014/10/24 17时30分29秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 ****************************************************************************/
#ifndef _SPX_LIMITS_H_
#define _SPX_LIMITS_H_
#ifdef __cplusplus
extern "C" {
#endif


#include <stdlib.h>
#include <stdio.h>

#define SpxI8Max 127
#define SpxI8Min (-128)
#define SpxU8Mac 255
#define SpxI8MaxLength sizeof("-127")
#define SpxU8MaxLength SpxI8MaxLength

#define SpxI16Max 32767
#define SpxI16Min (-32768)
#define SpxU16Max 65535
#define SpxI16MaxLength sizeof("-32768")
#define SpxU16MaxLength SpxI16MaxLength


#define SpxI32Max 2147483647
#define SpxI32Min (-2147483648)
#define SpxU32Max 4294967295
#define SpxI32MaxLength sizeof("-4294967295")
#define SpxU32MaxLength SpxI32MaxLength

#define SpxI64Max 9223372036854775807
#define SpxI64Min -9223372036854775808
#define SpxU64Max 18446744073709551615L
#define SpxI64MaxLength sizeof("-9223372036854775808")
#define SpxU64MaxLength SpxI64MaxLength

#ifdef __cplusplus
}
#endif
#endif

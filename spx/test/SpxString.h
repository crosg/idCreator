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
 *       Filename:  SpxString.h
 *        Created:  2014/12/02 22时07分23秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 ****************************************************************************/
#ifndef _SPXSTRING_H_
#define _SPXSTRING_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#include "spx_types.h"
#include "spx_vector.h"
#include "SpxObject.h"
#include "SpxMemoryPool.h"

typedef struct SpxObject _SpxString;
#define SpxStringBaseAlignedSize SpxObjectAlignSize

    spx_private spx_inline size_t spxStringSize(const string_t s) {
        _SpxString *ss = (_SpxString *) (s - SpxStringBaseAlignedSize);
        return ss->_spxObjectSize;
    }

    spx_private spx_inline size_t spxStringAvail(const string_t s) {
        _SpxString *ss = (_SpxString *) (s - SpxStringBaseAlignedSize);
        return ss->_spxObjectAvail;
    }

    spx_private spx_inline size_t spxStringLength(const string_t s) {
        _SpxString *ss = (_SpxString *) (s - SpxStringBaseAlignedSize);
        return ss->_spxObjectSize - ss->_spxObjectAvail;
    }



string_t spxStringNewWithLength(
        struct SpxMemoryPool *pool,
        const void *init,const size_t initlen,
        err_t *err);

string_t spxStringNewSpace(
        struct SpxMemoryPool *pool,
        size_t initlen,err_t *err);

string_t spxStringNew(
        struct SpxMemoryPool *pool,
        const char *init,err_t *err);

string_t spxStringNewEmpty(
        struct SpxMemoryPool *pool,
        err_t *err);

bool_t spxStringFree(
        struct SpxMemoryPool *pool,
        string_t s);

bool_t spxStringFreeForce(
        struct SpxMemoryPool *pool,
        string_t s);

void spxStringClear(
        string_t s);

string_t spxStringDup(
        struct SpxMemoryPool *pool,
        const string_t s,err_t *err);

string_t spxStringGrowZero(
        struct SpxMemoryPool *pool,
        string_t s, size_t len,
        err_t *err);

string_t spxStringCatLength(
        struct SpxMemoryPool *pool,
        string_t s,
        const void *t,size_t len,
        err_t *err);

string_t spxStringCatAlign(
        struct SpxMemoryPool *pool,
        string_t s,
        const void *t,size_t len,
        size_t align,
        err_t *err);

string_t spxStringCat(
        struct SpxMemoryPool *pool,
        string_t s,const char *t,
        err_t *err);

string_t spxStringCatString(
        struct SpxMemoryPool *pool,
        string_t s,string_t t,
        err_t *err);

string_t spxStringCopyWithLength(
        struct SpxMemoryPool *pool,
        string_t s,size_t offset,
        void *t,size_t len,
        err_t *err);

string_t spxStringCopy(
        struct SpxMemoryPool *pool,
        string_t s,size_t offset,
        const char *t,
        err_t *err);

string_t spxStringCopyString(
        struct SpxMemoryPool *pool,
        string_t s,size_t offset,
        string_t t,
        err_t *err);

string_t soxStringCatAndVprintf(
        struct SpxMemoryPool *pool,
        err_t *err,
        string_t s,
        const char *fmt,
        va_list ap);

string_t spxStringCatAndPrintf(
        struct SpxMemoryPool *pool,
        err_t *err,
        string_t s,
        const char *fmt,
        ...);

void spxStringTrimWith(
        string_t s,
        const char *cset);

void spxStringTrimWithString(
        string_t s,
        string_t t);

void spxStringTrim(
        string_t s);

void spxStringRigthTrimWith(
        string_t s,
        const char *cset);

void spxStringRightTrimWithString(
        string_t s,
        string_t t);

void spxStringRigthTrim(
        string_t s);

void spxStringLeftTrimWith(
        string_t s,
        const char *cset);

void spxStringLeftTrimWithString(
        string_t s,
        string_t t);

void spxStringLeftTrim(
        string_t s);

void spxStringStripLinefeed(
        string_t s);

void spxStringRange(
        string_t s,
        int start,int end);

string_t spxStringRangeNew(
        struct SpxMemoryPool *pool,
        string_t s,
        int start,int end,
        err_t *err);

void spxStringUpdateLength(
        string_t s);

int spxStringCmpString(
        const string_t s1,
        const string_t s2);

int spxStringCaseCmpString(
        const string_t s1,
        const string_t s2);

int spxStringCmp(
        const string_t s1,
        const char *s2);

int spxStringCaseCmp(
        const string_t s1,
        const char *s2);

bool_t spxStringBeginWithString(
        const string_t s1,
        const string_t s2);

bool_t spxStringBeginWith(
        const string_t s1,
        const char *s2);

bool_t spxStringEndWithString(
        const string_t s1,
        const string_t s2);

bool_t spxStringEndWith(
        const string_t s1,
        const char *s2);

bool_t spxStringBeginCaseWithString(
        const string_t s1,
        const string_t s2);

bool_t spxStringBeginCaseWith(
        const string_t s1,
        const char *s2);

bool_t spxStringEndCaseWithString(
        const string_t s1,
        const string_t s2);

string_t *spxStringSplit(
        struct SpxMemoryPool *pool, const char *s,\
        int len, const char *sep, int seplen, \
        int *count,err_t *err);

string_t *spxStringSplitString(
        struct SpxMemoryPool *pool,
        string_t s,
        const char *sep,int seplen,\
        int *count,err_t *err);

string_t *spxStringSplitWithString(
        struct SpxMemoryPool *pool,
        string_t s,string_t sep,
        int *count,err_t *err);

void spxStringFreeSplitres(
        struct SpxMemoryPool *pool,
        string_t *tokens, int count);

void spxStringToLower(string_t s);

void spxStringToUpper(string_t s);

string_t spxStringFromI64(
        struct SpxMemoryPool *pool,
        i64_t value,err_t *err);

string_t spxStringJoin(
        struct SpxMemoryPool *pool,
        char **argv, int argc,
        char *sep, size_t seplen,
        err_t *err);

string_t spxStringJoinString(
        struct SpxMemoryPool *pool,
        string_t *argv,int argc,
        const char *sep, size_t seplen,
        err_t *err);

bool_t spxStringExist(
        string_t s,
        char c);

string_t spxStringPackI32(
        struct SpxMemoryPool *pool,
        string_t s,const i32_t v,
        err_t *err);

string_t spx_string_pack_int(
        struct SpxMemoryPool *pool,
        string_t s,const int v,
        err_t *err);



#define SpxStringRealSize(l) ((l) + 1)

#define SpxString2Char1(s) ((char *)(s))
#define SpxString2Char2(s) ((const char *)(s))

#define SpxStringIsNullOrEmpty(s) (NULL == s \
        || (0 == spx_string_len(s)))
#define SpxStringIsEmpty(s) (NULL != s \
        && (0 == spx_string_len(s)))

#define SpxStringEndWith(s,c) \
    (!SpxStringIsNullOrEmpty(s) \
     && (c == *(s +  spx_string_len(s) - 1)))
#define SpxStringBeginWith(s,c) ( c == *s)

#define SpxZeroPtr(v) memset(v,0,sizeof(*(v)))
#define SpxZero(v) memset(&(v),0,sizeof(v))
#define SpxZeroLen(v,len) memset(v,0,len)

#define SpxMemcpy(d,s,l) (((uchar_t *) memcpy(d,s,l)) + l)

#define SpxStringFree(s) spx_string_free(s);s = NULL
#define SpxStringFreePooling(p,s) spx_string_free_pooling(p,s)
#define SpxStringFreePoolingForce(p,s) spx_string_free_pooling_force(p,s)


#ifdef __cplusplus
}
#endif
#endif

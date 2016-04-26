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
 *       Filename:  SpxString.c
 *        Created:  2014/12/02 22时08分28秒
 *         Author:  Seapeak.Xu (seapeak.cnblog.com), xvhfeng@gmail.com
 *        Company:  Tencent Literature
 *         Remark:
 *
 ***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "spx_types.h"
#include "spx_alloc.h"
#include "spx_errno.h"
#include "spx_defs.h"
#include "SpxObject.h"
#include "SpxMemoryPool.h"
#include "SpxString.h"

union d2i{
    double v;
    uint64_t i;
};

union f2i{
    float v;
    uint32_t i;
};

#define MaxReallocSize (1024*1024)

spx_private void spx_i2b(uchar_t *b,const i32_t n);
spx_private i32_t spx_b2i(uchar_t *b);
spx_private void spx_l2b(uchar_t *b,const i64_t n);
spx_private i64_t spx_b2l(uchar_t *b);

spx_private string_t _spxStringMakeRoomSize(
        struct SpxMemoryPool *pool,
        string_t s, size_t addlen,
        err_t *err)

spx_private string_t _spxStringMakeRoomToSize(
        struct SpxMemoryPool *pool,
        string_t s, size_t size,
        err_t *err);

spx_private void _spxStringIncrLength(string_t s, int incr);

spx_private void _spxStringDecrLength(string_t s, int incr);



string_t spxStringNewWithLength(
        struct SpxMemoryPool *pool,
        const void *init,const size_t initlen,
        err_t *err
        ){/*{{{*/
    string_t s = spxMemoryPoolAlloc(pool,SpxStringRealSize(initlen),err);
    if (s == NULL) return NULL;
    _SpxString *ss =(_SpxString *) (s - SpxStringBaseAlignedSize);
    if (initlen && init) {
        memcpy(s, init, initlen);
        //because align memory in the mempool
        ss->_spxObjectAvail = ss->_spxObjectSize - initlen;
        ss->buf[initlen] = '\0';
    }else {
        ss->_spxObjectAvail = ss->_spxObjectSize - initlen;
    }
    return s;
}/*}}}*/

string_t spxStringNewSpace(
        struct SpxMemoryPool *pool,
        size_t initlen,err_t *err
        ){/*{{{*/
    return spxStringNewWithLength(pool,NULL,initlen,err);
}/*}}}*/

string_t spxStringNew(
        struct SpxMemoryPool *pool,
        const char *init,err_t *err
        ){/*{{{*/
    size_t initlen = (init == NULL) ? 0 : strlen(init);
    return spxStringNewWithLength(pool,init,initlen,err);
}/*}}}*/

string_t spxStringNewEmpty(
        struct SpxMemoryPool *pool,
        err_t *err
        ){/*{{{*/
    return spxStringNewWithLength(pool,NULL,0,err);
}/*}}}*/

bool_t spxStringFree(
        struct SpxMemoryPool *pool,
        string_t s
        ){/*{{{*/
    if(NULL == s){
        return true;
    }
    SpxMemoryPoolFree(pool,s);
    return false;
}/*}}}*/

bool_t spxStringFreeForce(
        struct SpxMemoryPool *pool,
        string_t s
        ){/*{{{*/
    if(NULL == s){
        return true;
    }
    SpxMemoryPoolFreeForce(pool,s);
    return true;
}/*}}}*/

void spxStringClear(
        string_t s
        ){/*{{{*/
    _SpxString *ss =(_SpxString *) (s - SpxStringBaseAlignedSize);
    memset(s,0,ss->_spxObjectSize);
    ss->_spxObjectAvail = ss->_spxObjectSize;
}/*}}}*/

string_t spxStringDup(
        struct SpxMemoryPool *pool,
        const string_t s,err_t *err
        ){/*{{{*/
    return spxStringNewWithLength(pool,s,spxStringSize(s),err);
}/*}}}*/

string_t spxStringGrowZero(
        struct SpxMemoryPool *pool,
        string_t s, size_t len,
        err_t *err
        ){/*{{{*/
    _SpxString *ss = (_SpxString *) (s - SpxStringBaseAlignedSize);
    size_t curlen = ss->_spxObjectSize;
    if (len <= curlen) return s;
    s = _spxStringMakeRoomToSize(pool,s,len,err);
    if (s == NULL) return NULL;
    /* Make sure added region doesn't contain garbage */
    ss = (_SpxString *) (s - SpxStringBaseAlignedSize);
    return s;
}/*}}}*/


string_t spxStringCatLength(
        struct SpxMemoryPool *pool,
        string_t s,
        const void *t,size_t len,
        err_t *err
        ){/*{{{*/
    size_t currlen = spxStringLength(s);
    string_t new = _spxStringMakeRoomSize(pool,s,len,err);
    if(NULL == new) return NULL;
    memcpy(new + currlen,t,len);
    _spxStringDecrLength(new,len);
    return new;
}/*}}}*/

string_t spxStringCatAlign(
        struct SpxMemoryPool *pool,
        string_t s,
        const void *t,size_t len,
        size_t align,
        err_t *err
        ){/*{{{*/
    size_t currlen = spxStringLength(s);
    string_t new = _spxStringMakeRoomSize(pool,s,align,err);
    if(NULL == new) return NULL;
    if(len <= align){
        memcpy(new + currlen,t,len);
    }else {
        memcpy(new + currlen,t,align);
    }
    _spxStringDecrLength(new,align);
    return new;
}/*}}}*/

string_t spxStringCat(
        struct SpxMemoryPool *pool,
        string_t s,const char *t,
        err_t *err
        ){/*{{{*/
    return spxStringCatLength(pool,s,t,strlen(t),err);
}/*}}}*/

string_t spxStringCatString(
        struct SpxMemoryPool *pool,
        string_t s,string_t t,
        err_t *err
        ){/*{{{*/
    return spxStringCatLength(pool,s,t,spxStringLength(t),err);
}/*}}}*/

string_t spxStringCopyWithLength(
        struct SpxMemoryPool *pool,
        string_t s,size_t offset,
        void *t,size_t len,
        err_t *err
        ){/*{{{*/
    size_t osize = spxStringSize(s);
    size_t olen = spxStringLength(s);
    if(osize < offset + len){
        string_t new = _spxStringMakeRoomToSize(pool,s,offset + len,err);
        if(NULL == new) return NULL;
        memcpy(new + offset,t,len);
        _spxStringDecrLength(new,offset - olen + len);
        return new;
    }
    memcpy(s + offset,t,len);
    _spxStringDecrLength(s,offset - olen + len);
    return s;
}/*}}}*/

string_t spxStringCopy(
        struct SpxMemoryPool *pool,
        string_t s,size_t offset,
        const char *t,
        err_t *err
        ){/*{{{*/
    return spxStringCopyWithLength(pool,s,offset,(void *) t,strlen(t),err);
}/*}}}*/

string_t spxStringCopyString(
        struct SpxMemoryPool *pool,
        string_t s,size_t offset,
        string_t t,
        err_t *err
        ){/*{{{*/
    return spxStringCopyWithLength(pool,s,offset,(void *) t,spxStringLength(t),err);
}/*}}}*/

string_t soxStringCatAndVprintf(
        struct SpxMemoryPool *pool,
        err_t *err,
        string_t s,
        const char *fmt,
        va_list ap
        ){/*{{{*/
    va_list cpy;
    size_t buflen = 32;
    string_t buf = NULL;
    string_t tmp = NULL;
    buf = spxStringNewSpace(pool,buflen,err);
    if(NULL == buf) return NULL;

    while(true){
        s[buflen - 2] = '\0';
        va_copy(cpy,ap);
        vsnprintf(buf, buflen, fmt, cpy);
        if('\0' != buf[buflen + 2]){
            buflen *= 2;
            tmp =_spxStringMakeRoomToSize(pool,buf,buflen,err);
            if(NULL == tmp){
                spxStringFree(pool,buf);
                return NULL;
            }
            buf = tmp;
            tmp = NULL;
            spxStringClear(buf);
            continue;
        }
        break;
    }
    tmp = spxStringCatString(pool,s,buf,err);
    spxStringFree(pool,buf);
    return tmp;
}/*}}}*/

string_t spxStringCatAndPrintf(
        struct SpxMemoryPool *pool,
        err_t *err,
        string_t s,
        const char *fmt,
        ...
        ){/*{{{*/
    va_list ap;
    char *t;
    va_start(ap, fmt);
    t = spxStringCatAndPrintf(pool,err,s,fmt,ap);
    va_end(ap);
    return t;
}/*}}}*/

void spxStringTrimWith(
        string_t s,
        const char *cset
        ){/*{{{*/
    _SpxString *ss = (_SpxString *) (s - SpxStringBaseAlignedSize);
    char *start, *end, *sp, *ep;
    size_t olen = spxStringLength(s);
    size_t len;

    sp = start = s;
    ep = end = s+ olen -1;
    while(sp <= end && strchr(cset, *sp)) sp++;
    while(ep > start && strchr(cset, *ep)) ep--;
    len = (sp > ep) ? 0 : ((ep-sp)+1);
    if (ss->buf != sp) memmove(ss->buf, sp, len);
    ss->buf[len] = '\0';
    ss->_spxObjectAvail = ss->_spxObjectSize - len;
}/*}}}*/

void spxStringTrimWithString(
        string_t s,
        string_t t
        ){/*{{{*/
    spxStringTrimWith(s,(const char *) t);
}/*}}}*/

void spxStringTrim(
        string_t s
        ){/*{{{*/
    spxStringTrimWith(s," ");
}/*}}}*/

void spxStringRigthTrimWith(
        string_t s,
        const char *cset
        ){/*{{{*/
    _SpxString *ss = (_SpxString *) (s - SpxStringBaseAlignedSize);
    char *start, *end, *sp, *ep;
    size_t olen = spxStringLength(s);
    size_t len;

    sp = start = s;
    ep = end = s + olen - 1;
    while(ep >= start && strchr(cset, *ep)) ep--;
    len = (sp > ep) ? 0 : ((ep-sp)+1);
    if (ss->buf != sp) memmove(ss->buf, sp, len);
    ss->buf[len] = '\0';
    ss->_spxObjectAvail = ss->_spxObjectSize - len;
}/*}}}*/

void spxStringRightTrimWithString(
        string_t s,
        string_t t
        ){/*{{{*/
    spxStringRigthTrimWith(s,(const char *) t);
}/*}}}*/

void spxStringRigthTrim(
        string_t s
        ){/*{{{*/
    spxStringRightTrimWithString(s," ");
}/*}}}*/

void spxStringLeftTrimWith(
        string_t s,
        const char *cset
        ){/*{{{*/
    _SpxString *ss = (_SpxString *) (s - SpxStringBaseAlignedSize);
    char *start, *end, *sp, *ep;
    size_t olen = spxStringLength(s);
    size_t len;

    sp = start = s;
    ep = end = s + olen - 1;
    while(sp <= end && strchr(cset, *sp)) sp++;
    len = (sp > ep) ? 0 : ((ep-sp)+1);
    if (ss->buf != sp) memmove(ss->buf, sp, len);
    ss->buf[len] = '\0';
    ss->_spxObjectAvail = ss->_spxObjectSize - len;
}/*}}}*/

void spxStringLeftTrimWithString(
        string_t s,
        string_t t
        ){/*{{{*/
    spxStringLeftTrimWith(s,(const char *) t);
}/*}}}*/

void spxStringLeftTrim(
        string_t s
        ){/*{{{*/
    spxStringLeftTrimWith(s, " ");
}/*}}}*/

void spxStringStripLinefeed(
        string_t s
        ){/*{{{*/
    spxStringRigthTrim(s);
    spxStringRigthTrimWith(s,SpxLineEndDlmtString);
}/*}}}*/

void spxStringRange(
        string_t s,
        int start,int end
        ){/*{{{*/
    _SpxString *ss = (_SpxString *) (s - SpxStringBaseAlignedSize);
    size_t newlen, len = spxStringLength(s);

    if (len == 0) return;
    if (start < 0) {
        start = len+start;
        if (start < 0) start = 0;
    }
    if (end < 0) {
        end = len+end;
        if (end < 0) end = 0;
    }
    newlen = (start > end) ? 0 : (end-start)+1;
    if (newlen != 0) {
        if (start >= (signed)len) {
            newlen = 0;
        } else if (end >= (signed)len) {
            end = len-1;
            newlen = (start > end) ? 0 : (end-start)+1;
        }
    } else {
        start = 0;
    }
    if (start && newlen) memmove(ss->buf, ss->buf+start, newlen);
    ss->buf[newlen] = 0;
    ss->_spxObjectAvail = ss->_spxObjectSize - newlen;
}/*}}}*/

string_t spxStringRangeNew(
        struct SpxMemoryPool *pool,
        string_t s,
        int start,int end,
        err_t *err
        ){/*{{{*/
    _SpxString *ss = (_SpxString *) (s - SpxStringBaseAlignedSize);
    size_t newlen, len = spxStringLength(s);

    if (len == 0) return NULL;
    if (start < 0) {
        start = len+start;
        if (start < 0) start = 0;
    }
    if (end < 0) {
        end = len+end;
        if (end < 0) end = 0;
    }
    newlen = (start > end) ? 0 : (end-start)+1;
    if (newlen != 0) {
        if (start >= (signed)len) {
            newlen = 0;
        } else if (end >= (signed)len) {
            end = len-1;
            newlen = (start > end) ? 0 : (end-start)+1;
        }
    } else {
        start = 0;
    }
    return  spxStringNewWithLength(pool,ss->buf + start,newlen,err);
}/*}}}*/

void spxStringUpdateLength(
        string_t s
        ){/*{{{*/
    _SpxString *ss = (_SpxString *) (s - SpxStringBaseAlignedSize);
    int reallen = strlen(s);
    ss->_spxObjectAvail = ss->_spxObjectSize - reallen;
}/*}}}*/

int spxStringCmpString(
        const string_t s1,
        const string_t s2
        ){/*{{{*/
    size_t l1, l2, minlen;
    int cmp;

    l1 = spxStringLength(s1);
    l2 = spxStringLength(s2);
    minlen = (l1 < l2) ? l1 : l2;
    cmp = memcmp(s1,s2,minlen);
    if (cmp == 0) return l1-l2;
    return cmp;
}/*}}}*/

int spxStringCaseCmpString(
        const string_t s1,
        const string_t s2
        ){/*{{{*/
    size_t l1, l2, minlen;
    int cmp;

    l1 = spxStringLength(s1);
    l2 = spxStringLength(s2);
    minlen = (l1 < l2) ? l1 : l2;
    char a,b;
    size_t i = 0;
    for(; i < minlen; i++){
        a = *(s1 + i);
        b = *(s2 + i);
        a = (65 <= a && 90 >= a) ? a + 32 : a;
        b = (65 <= b && 90 >= b) ? b + 32 : b;
        cmp = a -b;
        if(0 != cmp) return cmp;
    }
    if (cmp == 0) return l1-l2;
    return cmp;
}/*}}}*/

int spxStringCmp(
        const string_t s1,
        const char *s2
        ){/*{{{*/
    size_t l1, l2, minlen;
    int cmp;

    l1 = spxStringLength(s1);
    l2 = strlen(s2);
    minlen = (l1 < l2) ? l1 : l2;
    cmp = memcmp(s1,s2,minlen);
    if (cmp == 0) return l1-l2;
    return cmp;
}/*}}}*/

int spxStringCaseCmp(
        const string_t s1,
        const char *s2
        ){/*{{{*/
    size_t l1, l2, minlen;
    int cmp;

    l1 = spxStringLength(s1);
    l2 = strlen(s2);
    minlen = (l1 < l2) ? l1 : l2;
    char a,b;
    size_t i = 0;
    for(; i < minlen; i++){
        a = *(s1 + i);
        b = *(s2 + i);
        a = (65 <= a && 90 >= a) ? a + 32 : a;
        b = (65 <= b && 90 >= b) ? b + 32 : b;
        cmp = a -b;
        if(0 != cmp) return cmp;
    }
    if (cmp == 0) return l1-l2;
    return cmp;
}/*}}}*/

bool_t spxStringBeginWithString(
        const string_t s1,
        const string_t s2
        ){/*{{{*/
    size_t l1, l2;
    l1 = spxStringLength(s1);
    l2 = spxStringLength(s2);
    if(l1 < l2) return false;
    char a,b;
    size_t i = 0;
    for(; i < l2; i++){
        a = *(s1 + i);
        b = *(s2 + i);
        if(a != b) return false;
    }
    return true;
}/*}}}*/


bool_t spxStringBeginWith(
        const string_t s1,
        const char *s2
        ){/*{{{*/
    size_t l1, l2;
    l1 = spxStringLength(s1);
    l2 = strlen(s2);
    if(l1 < l2) return false;
    char a,b;
    size_t i = 0;
    for(; i < l2; i++){
        a = *(s1 + i);
        b = *(s2 + i);
        if(a != b) return false;
    }
    return true;
}/*}}}*/


bool_t spxStringEndWithString(
        const string_t s1,
        const string_t s2
        ){/*{{{*/
    size_t l1, l2;
    l1 = spxStringLength(s1);
    l2 = spxStringLength(s2);
    if(l1 < l2) return false;
    char *p1 = s1 + l1;
    char *p2 = s2 + l2;
    char a,b;
    size_t i = 0;
    for(; i < l2; i++){
        a = *(p1 - i);
        b = *(p2 - i);
        if(a != b) return false;
    }
    return true;
}/*}}}*/

bool_t spxStringEndWith(
        const string_t s1,
        const char *s2
        ){/*{{{*/
    size_t l1, l2;
    l1 = spxStringLength(s1);
    l2 = strlen(s2);
    if(l1 < l2) return false;
    char *p1 = s1 + l1;
    char *p2 = (char *) s2 + l2;
    char a,b;
    size_t i = 0;
    for(; i < l2; i++){
        a = *(p1 - i);
        b = *(p2 - i);
        if(a != b) return false;
    }
    return true;
}/*}}}*/

bool_t spxStringBeginCaseWithString(
        const string_t s1,
        const string_t s2
        ){/*{{{*/
    size_t l1, l2;
    l1 = spxStringLength(s1);
    l2 = spxStringLength(s2);
    if(l1 < l2) return false;
    char a,b;
    size_t i = 0;
    for(; i < l2; i++){
        a = *(s1 + i);
        b = *(s2 + i);
        a = (65 <= a && 90 >= a) ? a + 32 : a;
        b = (65 <= b && 90 >= b) ? b + 32 : b;
        if(a != b) return false;
    }
    return true;
}/*}}}*/


bool_t spxStringBeginCaseWith(
        const string_t s1,
        const char *s2
        ){/*{{{*/
    size_t l1, l2;

    l1 = spxStringLength(s1);
    l2 = strlen(s2);
    if(l1 < l2) return false;
    char a,b;
    size_t i = 0;
    for(; i < l2; i++){
        a = *(s1 + i);
        b = *(s2 + i);
        a = (65 <= a && 90 >= a) ? a + 32 : a;
        b = (65 <= b && 90 >= b) ? b + 32 : b;
        if(a != b) return false;
    }
    return true;
}/*}}}*/


bool_t spxStringEndCaseWithString(
        const string_t s1,
        const string_t s2
        ){/*{{{*/
    size_t l1, l2;
    l1 = spxStringLength(s1);
    l2 = spxStringLength(s2);
    if(l1 < l2) return false;
    char *p1 = s1 + l1;
    char *p2 = s2 + l2;
    char a,b;
    size_t i = 0;
    for(; i < l2; i++){
        a = *(p1 - i);
        b = *(p2 - i);
        a = (65 <= a && 90 >= a) ? a + 32 : a;
        b = (65 <= b && 90 >= b) ? b + 32 : b;
        if(a != b) return false;
    }
    return true;
}/*}}}*/


string_t *spxStringSplit(
        struct SpxMemoryPool *pool, const char *s,\
        int len, const char *sep, int seplen, \
        int *count,err_t *err
        ){/*{{{*/
    int elements = 0, slots = 5, start = 0, j;
    string_t *tokens;

    if (seplen < 1 || len < 0) return NULL;

    tokens = spxMemoryPoolAlloc(pool,sizeof(string_t) * slots,err);
    if (tokens == NULL) return NULL;

    if (len == 0) {
        *count = 0;
        return tokens;
    }
    for (j = 0; j < (len-(seplen-1)); j++) {
        /* make sure there is room for the next element and the final one */
        if (slots < elements+2) {
            string_t *newtokens;

            slots *= 2;
            newtokens = spxMemoryPoolReAlloc(pool,tokens,sizeof(_SpxString) * slots,err);
//            newtokens = spx_realloc(tokens,sizeof(struct sds)*slots,err);
            if (newtokens == NULL) goto cleanup;
            tokens = newtokens;
        }
        /* search the separator */
        if ((seplen == 1 && *(s+j) == sep[0]) || (memcmp(s+j,sep,seplen) == 0)) {
            tokens[elements] = spxStringNewWithLength(pool,s + start,j-start,err);
//            tokens[elements] = spx_string_newlen(s+start,j-start,err);
            if (tokens[elements] == NULL) goto cleanup;
            elements++;
            start = j+seplen;
            j = j+seplen-1; /* skip the separator */
        }
    }
    /* Add the final element. We are sure there is room in the tokens array. */
    tokens[elements] = spxStringNewWithLength(pool,s + start,len-start,err);
//    tokens[elements] = spx_string_newlen(s+start,len-start,err);
    if (tokens[elements] == NULL) goto cleanup;
    elements++;
    *count = elements;
    return tokens;

cleanup:
    {
        int i;
        for (i = 0; i < elements; i++) spxStringFree(pool,tokens[i]);
        SpxMemoryPoolFree(pool,tokens);
        *count = 0;
        return NULL;
    }
}/*}}}*/

string_t *spxStringSplitString(
        struct SpxMemoryPool *pool,
        string_t s,
        const char *sep,int seplen,\
        int *count,err_t *err
        ){/*{{{*/
    return spxStringSplit(pool,s,spxStringLength(s),sep,seplen,count,err);
}/*}}}*/

string_t *spxStringSplitWithString(
        struct SpxMemoryPool *pool,
        string_t s,string_t sep,
        int *count,err_t *err
        ){/*{{{*/
    return spxStringSplit(pool,s,spxStringLength(s),sep,spxStringLength(sep),count,err);
}/*}}}*/

void spxStringFreeSplitres(
        struct SpxMemoryPool *pool,
        string_t *tokens, int count
        ){/*{{{*/
    if (!tokens) return;
    while(count--)
        spxStringFree(pool,tokens[count]);
    SpxFree(tokens);
}/*}}}*/

void spxStringToLower(string_t s){/*{{{*/
    int len = spxStringLength(s), j;
    for (j = 0; j < len; j++) s[j] = tolower(s[j]);
}/*}}}*/

void spxStringToUpper(string_t s){/*{{{*/
    int len = spxStringLength(s), j;
    for (j = 0; j < len; j++) s[j] = toupper(s[j]);
}/*}}}*/

string_t spxStringFromI64(
        struct SpxMemoryPool *pool,
        i64_t value,err_t *err){/*{{{*/
    char buf[SpxI64Size + 1], *p;
    unsigned long long v;

    v = (value < 0) ? -value : value;
    p = buf+(SpxI64Size); /* point to the last character */
    do {
        *p-- = '0'+(v%10);
        v /= 10;
    } while(v);
    if (value < 0) *p-- = '-';
    p++;
    return spxStringNewWithLength(pool,p,SpxI64Size-(p-buf),err);
}/*}}}*/



/* Helper function for sdssplitargs() that returns non zero if 'c'
 * is a valid hex digit. */
int is_hex_digit(char c) {/*{{{*/
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}/*}}}*/

/* Helper function for sdssplitargs() that converts a hex digit into an
 * integer from 0 to 15 */
int hex_digit_to_int(char c) {/*{{{*/
    switch(c) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'a': case 'A': return 10;
    case 'b': case 'B': return 11;
    case 'c': case 'C': return 12;
    case 'd': case 'D': return 13;
    case 'e': case 'E': return 14;
    case 'f': case 'F': return 15;
    default: return 0;
    }
}/*}}}*/


string_t spxStringJoin(
        struct SpxMemoryPool *pool,
        char **argv, int argc,
        char *sep, size_t seplen,
        err_t *err
        ){/*{{{*/
    string_t join = spxStringNewEmpty(pool,err);
    if(NULL == join) return NULL;
    int j;
    string_t new;

    for (j = 0; j < argc; j++) {
       new  = spxStringCat(pool,join, argv[j],err);
       if(NULL == new){
            spxStringFree(pool,join);
            return NULL;
       }
       join = new;
       if (j != argc-1) {
           new = spxStringCatLength(pool,join,sep,seplen,err);
           if(NULL == new){
               spxStringFree(pool,join);
               return NULL;
           }
           join = new;
       }
    }
    return join;
}/*}}}*/

string_t spxStringJoinString(
        struct SpxMemoryPool *pool,
        string_t *argv,int argc,
        const char *sep, size_t seplen,
        err_t *err
        ){/*{{{*/
    string_t join = spxStringNewEmpty(pool,err);
    int j;
    string_t new = NULL;

    for (j = 0; j < argc; j++) {
        new = spxStringCatString(pool,join, argv[j],err);
        if(NULL == new){
            spxStringFree(pool,join);
            return NULL;
        }
        join = new;
        if (j != argc-1) {
            new = spxStringCatLength(pool,join,sep,seplen,err);
            if(NULL == new){
                spxStringFree(pool,join);
                return NULL;
            }
            join = new;
        }
    }
    return join;
}/*}}}*/

bool_t spxStringExist(
        string_t s,
        char c
        ){/*{{{*/
    size_t len = spxStringLength(s);
    size_t i = 0;
    for( ; i < len; i++){
        if(c == s[i]){
            return true;
        }
    }
    return false;
}/*}}}*/

string_t spxStringPackI32(
        struct SpxMemoryPool *pool,
        string_t s,const i32_t v,
        err_t *err
        ){/*{{{*/
    if(NULL == s){
        *err =  EINVAL;
        return NULL;
    }

    size_t curlen = spxStringLength(s);
    string_t new = NULL;
    new = _spxStringMakeRoomSize(pool,s,SpxI32Size,err);
    if (NULL == new) return NULL;
    spx_i2b((uchar_t *) new + curlen,v);
    _spxStringIncrLength(new,SpxI32Size);
    return s;
}/*}}}*/

string_t spx_string_pack_int(
        struct SpxMemoryPool *pool,
        string_t s,const int v,
        err_t *err
        ){/*{{{*/
    return spxStringPackI32(pool,s,(i32_t) v,err);
}/*}}}*/


spx_private void spx_i2b(uchar_t *b,const i32_t n){/*{{{*/
    *b++ = (n >> 24) & 0xFF;
    *b++ = (n >> 16) & 0xFF;
    *b++ = (n >> 8) & 0xFF;
    *b++ = n & 0xFF;
}/*}}}*/

spx_private i32_t spx_b2i(uchar_t *b){/*{{{*/
    i32_t n =  (i32_t ) ((((i32_t) (*b)) << 24)
            | (((i32_t) (*(b + 1))) << 16)
            | (((i32_t) (*(b+2))) << 8)
            | ((i32_t) (*(b+3))));
    b += sizeof(i32_t);
    return n;
}/*}}}*/

spx_private void spx_l2b(uchar_t *b,const i64_t n){/*{{{*/
    *b++ = (n >> 56) & 0xFF;
    *b++ = (n >> 48) & 0xFF;
    *b++ = (n >> 40) & 0xFF;
    *b++ = (n >> 32) & 0xFF;
    *b++ = (n >> 24) & 0xFF;
    *b++ = (n >> 16) & 0xFF;
    *b++ = (n >> 8) & 0xFF;
    *b++ = n & 0xFF;
}/*}}}*/

spx_private i64_t spx_b2l(uchar_t *b){/*{{{*/
    i64_t n =  (((i64_t) (*b)) << 56)
        | (((i64_t) (*(b+1))) << 48)
        | (((i64_t) (*(b + 2))) << 40)
        | (((i64_t) (*(b + 3))) << 32)
        | (((i64_t) (*(b + 4))) << 24)
        | (((i64_t) (*(b + 5))) << 16)
        | (((i64_t) (*(b + 6))) << 8)
        | ((i64_t) (*(b + 7)));
    b += sizeof(i64_t);
    return n;
}/*}}}*/


spx_private string_t _spxStringMakeRoomSize(
        struct SpxMemoryPool *pool,
        string_t s, size_t addlen,
        err_t *err
        ) {/*{{{*/
    _SpxString *newString;
    size_t free = spxStringAvail(s);
    size_t len, newlen;
    if (free >= addlen) return s;

    len = spxStringLength(s);
    newlen = (len+addlen);
    if (newlen < MaxReallocSize)
        newlen *= 2;
    else
        newlen += MaxReallocSize;
    newString = spxMemoryPoolReAlloc(pool,s,newlen,err);
    if (newString == NULL) return NULL;
    newString->_spxObjectAvail = newString->_spxObjectSize - len;
    return newString->buf;
}/*}}}*/


spx_private string_t _spxStringMakeRoomToSize(
        struct SpxMemoryPool *pool,
        string_t s, size_t size,
        err_t *err
        ) {/*{{{*/
    _SpxString *newString;
    size_t oSize = spxStringSize(s);
    size_t len = spxStringLength(s);
    if (oSize >= size ) return s;

    newString = spxMemoryPoolReAlloc(pool,s,size,err);
    if (newString == NULL) return NULL;
    newString->_spxObjectAvail = newString->_spxObjectSize - len;
    return newString->buf;
}/*}}}*/

spx_private void _spxStringIncrLength(string_t s, int incr){/*{{{*/
    _SpxString *ss = (_SpxString *) (s - SpxStringBaseAlignedSize);
    ss->_spxObjectAvail -= incr;
    s[ss->_spxObjectSize - ss->_spxObjectAvail] = '\0';
}/*}}}*/

spx_private void _spxStringDecrLength(string_t s, int incr){/*{{{*/
    _SpxString *ss = (_SpxString *) (s - SpxStringBaseAlignedSize);
    ss->_spxObjectAvail += incr;
    s[ss->_spxObjectSize - ss->_spxObjectAvail] = '\0';
}/*}}}*/


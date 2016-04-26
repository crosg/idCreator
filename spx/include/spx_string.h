/*****************************
 * this is the string function from the spx_string_ lib
 *
 * thanks for redis and author
 * **************************/

#ifndef SPX_STRING_H
#define SPX_STRING_H
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

struct sds{
    int len;
    int free;
    char buf[];
};

spx_private spx_inline size_t spx_string_rlen(const string_t s) {
    return strlen(s);
}

spx_private spx_inline size_t spx_string_len(const string_t s) {
    struct sds *sh = (void*)(s-sizeof *sh);
    return sh->len;
}

spx_private spx_inline size_t spx_string_avail(const string_t s) {
    struct sds *sh = (void*)(s-sizeof *sh);
    return sh->free;
}

spx_private spx_inline size_t spx_string_msize(const string_t s){
    struct sds *sh = (void*)(s-sizeof *sh);
    return sh->len + sh->free;
}

string_t spx_string_newlen(const void *init, size_t initlen,err_t *err);

string_t spx_string_new(const char *init,err_t *err);

string_t spx_string_empty(err_t *err);
string_t spx_string_emptylen(size_t initlen,err_t *err);

string_t spx_string_dup(const string_t s,err_t *err);

void spx_string_free(string_t s);

/* Grow the sds to have the specified length. Bytes that were not part of
 * the original length of the sds will be set to zero.
 *
 * if the specified length is smaller than the current length, no operation
 * is performed. */
string_t spx_string_grow_zero(string_t s, size_t len,err_t *err);

string_t spx_string_catlen(string_t s, const void *t, size_t len,err_t *err);

string_t spx_string_cat(string_t s, const char *t,err_t *err);

string_t spx_string_cat_string(string_t s, const string_t t,err_t *err);

string_t spx_string_cpylen(string_t s, const char *t, size_t len,err_t *err);
string_t spx_string_catalign(string_t s, const void *t,size_t len,
        size_t align,err_t *err);

string_t spx_string_cpy(string_t s, const char *t,err_t *err);

string_t spx_string_cat_vprintf(err_t *err,string_t s, \
        const char *fmt, va_list ap);

string_t spx_string_cat_printf(err_t *err,string_t s, const char *fmt, ...);

/* Remove the part of the string from left and from right composed just of
 * contiguous characters found in 'cset', that is a null terminted C string.
 *
 * After the call, the modified sds string is no longer valid and all the
 * references must be substituted with the new pointer returned by the call.
 *
 * Example:
 *
        x = sdsnew("xxciaoyyy");
        sdstrim(x,"xy");
 * printf("%s\n", s);
 *
 * Output will be just "Hello World".
 */
void spx_string_trim(string_t s, const char *cset);
void spx_string_ltrim(string_t s, const char *cset);
void spx_string_rtrim(string_t s, const char *cset);

/*
 *  strip the line feed.
 *  line feed is SpxLineEndDltmString
 */
void spx_string_strip_linefeed(string_t s);

/* Turn the string into a smaller (or equal) string containing only the
 * substring specified by the 'start' and 'end' indexes.
 *
 * start and end can be negative, where -1 means the last character of the
 * string, -2 the penultimate character, and so forth.
 *
 * The interval is inclusive, so the start and end characters will be part
 * of the resulting string.
 *
 * The string is modified in-place.
 *
 * Example:
 *
 * s = sdsnew("Hello World");
 * sdsrange(s,1,-1); => "ello World"
 */
void spx_string_range(string_t s, int start, int end);
/*
 * not change the old string,and alloc the new string for range string.
 * then,if the old string is nouseful,please free it by yourself.
 */
string_t spx_string_range_new(string_t s, int start, int end,err_t *err);
/* Set the sds string length to the length as obtained with strlen(), so
 * considering as content only up to the first null term character.
 *
 * This function is useful when the sds string is hacked manually in some
 * way, like in the following example:
 *
 * s = sdsnew("foobar");
 * s[2] = '\0';
 * sdsupdatelen(s);
 * printf("%d\n", sdslen(s));
 *
 * The output will be "2", but if we comment out the call to sdsupdatelen()
 * the output will be "6" as the string was modified but the logical length
 * remains 6 bytes. */
void spx_string_updatelen(string_t s);

void spx_string_clear(string_t s);

int spx_string_cmp(const string_t s1, const string_t s2);
int spx_string_casecmp_string(const string_t s1, const string_t s2);
int spx_string_casecmp(const string_t s1, const char *s2);

bool_t spx_string_begin_with(const string_t s1,const char *s2);
bool_t spx_string_begin_casewith(const string_t s1,const char *s2);
bool_t spx_string_begin_with_string(const string_t s1,const string_t s2);
bool_t spx_string_end_with_string(const string_t s1,const string_t s2);
bool_t spx_string_begin_casewith_string(const string_t s1,const string_t s2);
bool_t spx_string_end_casewith_string(const string_t s1,const string_t s2);

string_t *spx_string_split(string_t s,
        const char *sep,int seplen,\
        int *count,err_t *err);

string_t *spx_string_split_string(string_t s,string_t sep,\
        int *count,err_t *err);

string_t *spx_string_splitlen(const char *s,\
        int len, const char *sep, int seplen, \
        int *count,err_t *err);

void spx_string_free_splitres(string_t *tokens, int count);

void spx_string_tolower(string_t s);

void spx_string_toupper(string_t s);

string_t spx_string_from_i64(i64_t value,err_t *err);

/* Append to the sds string "s" an escaped string representation where
 * all the non-printable characters (tested with isprint()) are turned into
 * escapes in the form "\n\r\a...." or "\x<hex-number>".
 *
        x = sdsnewlen("\a\n\0foo\r",7);
        y = sdscatrepr(sdsempty(),x,sdslen(x));
        test_cond("sdscatrepr(...data...)",
            memcmp(y,"\"\\a\\n\\x00foo\\r\"",15) == 0)
 * After the call, the modified sds string is no longer valid and all the
 * references must be substituted with the new pointer returned by the call. */
string_t spx_string_catrepr(string_t s, const char *p, size_t len,err_t *err);


/* Helper function for sdssplitargs() that returns non zero if 'c'
 * is a valid hex digit. */
int is_hex_digit(char c);

/* Helper function for sdssplitargs() that converts a hex digit into an
 * integer from 0 to 15 */
int hex_digit_to_int(char c);

/* Split a line into arguments, where every argument can be in the
 * following programming-language REPL-alike form:
 *
 * foo bar "newline are supported\n" and "\xff\x00otherstuff"
 *
 * The number of arguments is stored into *argc, and an array
 * of sds is returned.
 *
 * The caller should free the resulting array of sds strings with
 * sdsfreesplitres().
 *
 * Note that sdscatrepr() is able to convert back a string into
 * a quoted string in the same format sdssplitargs() is able to parse.
 *
 * The function returns the allocated tokens on success, even when the
 * input string is empty, or NULL if the input contains unbalanced
 * quotes or closed quotes followed by non space characters
 * as in: "foo"bar or "foo'
 */
string_t *spx_string_splitargs(const char *line, int *argc,err_t *err);

/* Modify the string substituting all the occurrences of the set of
 * characters specified in the 'from' string to the corresponding character
 * in the 'to' array.
 *
 * For instance: sdsmapchars(mystring, "ho", "01", 2)
 * will have the effect of turning the string "hello" into "0ell1".
 *
 * The function returns the sds string pointer, that is always the same
 * as the input pointer since no resize is needed. */
string_t spx_string_map_chars(string_t s,\
        const char *from, const char *to, size_t setlen);

string_t spx_string_join(char **argv, int argc, char *sep, size_t seplen,err_t *err);

string_t spx_string_join_string(string_t *argv,\
        int argc, const char *sep, size_t seplen,err_t *err);

bool_t spx_string_exist(string_t s,char c);

string_t spx_string_pack_i32(string_t s,const i32_t v,err_t *err);
string_t spx_string_pack_int(string_t s,const int v,err_t *err);
string_t spx_string_pack_i64(string_t s,const i64_t v,err_t *err);
string_t spx_string_pack_u64(string_t s,const u64_t v,err_t *err);


/* Low level functions exposed to the user API */
/* Enlarge the free space at the end of the sds string so that the caller
 * is sure that after calling this function can overwrite up to addlen
 * bytes after the end of the string, plus one more byte for nul term.
 *
 * Note: this does not change the *length* of the sds string as returned
 * by sdslen(), but only the free buffer space we have. */
string_t spxStringMakeRoomFor(string_t s, size_t addlen,err_t *err);

/* Increment the sds length and decrements the left free space at the
 * end of the string according to 'incr'. Also set the null term
 * in the new end of the string.
 *
 * This function is used in order to fix the string length after the
 * user calls sdsMakeRoomFor(), writes something after the end of
 * the current string, and finally needs to set the new length.
 *
 * Note: it is possible to use a negative increment in order to
 * right-trim the string.
 *
 * Usage example:
 *
 * Using sdsIncrLen() and sdsMakeRoomFor() it is possible to mount the
 * following schema, to cat bytes coming from the kernel to the end of an
 * sds string without copying into an intermediate buffer:
 *
 * oldlen = sdslen(s);
 * s = sdsMakeRoomFor(s, BUFFER_SIZE);
 * nread = read(fd, s+oldlen, BUFFER_SIZE);
 * ... check for nread <= 0 and handle it ...
 * sdsIncrLen(s, nread);
 */
void spxStringIncrLen(string_t s, int incr);

string_t spxStringRemoveFreeSpace(string_t s,err_t *err);

size_t spxStringAllocSize(string_t s);


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
#define SpxStringRealSize(l) ((l) + 1)

#define SpxZeroPtr(v) memset(v,0,sizeof(*(v)))
#define SpxZero(v) memset(&(v),0,sizeof(v))
#define SpxZeroLen(v,len) memset(v,0,len)

#define SpxMemcpy(d,s,l) (((uchar_t *) memcpy(d,s,l)) + l)

#define SpxStringFree(s) \
    if(NULL != s) \
do { \
    spx_string_free(s);\
    s = NULL;\
}while(false)

#define SpxStringFreePooling(p,s) spx_string_free_pooling(p,s)
#define SpxStringFreePoolingForce(p,s) spx_string_free_pooling_force(p,s)

#ifdef __cplusplus
}
#endif
#endif

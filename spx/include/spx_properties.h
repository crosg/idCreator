/*
 * =====================================================================================
 *
 *       Filename:  spx_properties.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2014/06/19 13时54分14秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#ifndef _SPX_PROPERTIES_H_
#define _SPX_PROPERTIES_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "spx_types.h"
#include "spx_map.h"

    struct spx_properties;

    typedef void SpxPropertyLineDeserializeDelegate(string_t line,struct spx_properties *p,err_t *err);
    typedef string_t SpxPropertyLineSerializeDelegate(struct spx_map_node *node,err_t *err);
    typedef err_t SpxPropertyParserBeforeDelegate(struct spx_properties *p);
    typedef err_t SpxPropertyParserAfterDelegate(struct spx_properties *p);

    struct spx_properties{
        SpxLogDelegate *log;
        //        struct spx_mpool *mpool;
        struct spx_map *configurtion;
        SpxPropertyParserBeforeDelegate *before;
        SpxPropertyParserAfterDelegate *after;
        SpxPropertyLineDeserializeDelegate *line_deserialize;
        SpxPropertyLineSerializeDelegate *line_serialize;
    };

    struct spx_properties *spx_properties_new(SpxLogDelegate *log,\
            SpxPropertyLineDeserializeDelegate *line_deserialize,\
            SpxPropertyLineSerializeDelegate *line_serialize,\
            SpxPropertyParserBeforeDelegate *before,\
            SpxPropertyParserAfterDelegate *after,\
            err_t *err);
    struct spx_properties *spx_properties_parser(struct spx_properties *p,string_t filename,err_t *err);
    void *spx_properties_get(struct spx_properties *p,string_t name,size_t *size);
    err_t spx_properties_set(struct spx_properties *p,string_t name,void *value,size_t size);
    string_t spx_properties_tostring(struct spx_properties *p,err_t *err);
    void spx_properties_store(struct spx_properties *p,string_t filename,err_t *err);
    void spx_properties_free(struct spx_properties **p);

#ifdef __cplusplus
}
#endif
#endif

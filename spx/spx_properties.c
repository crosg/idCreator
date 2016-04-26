/*
 * =====================================================================================
 *
 *       Filename:  spx_properties.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2014/06/19 13时54分06秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *   Organization:
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>


#include "spx_types.h"
#include "spx_properties.h"
#include "spx_collection.h"
#include "spx_alloc.h"
#include "spx_defs.h"
#include "spx_path.h"
#include "spx_io.h"

struct spx_properties *spx_properties_new(SpxLogDelegate *log,\
        SpxPropertyLineDeserializeDelegate *line_deserialize,\
        SpxPropertyLineSerializeDelegate *line_serialize,\
            SpxPropertyParserBeforeDelegate *before,\
            SpxPropertyParserAfterDelegate *after,\
        err_t *err){
    struct spx_properties *p = spx_alloc_alone(sizeof(*p),err);
    if(NULL == p){
        return NULL;
    }
    p->log = log;

    p->configurtion = spx_map_new(log,spx_pjw,
            spx_collection_string_default_cmper,
            NULL,
            NULL,
            NULL,
            err);
    if(NULL == p->configurtion){
        goto r1;
    }
    p->line_serialize = line_serialize;
    p->line_deserialize = line_deserialize;
    p->before = before;
    p->after = after;
    return p;

r1:
    SpxFree(p);
    return NULL;

}
struct spx_properties *spx_properties_parser(struct spx_properties *p,\
        string_t filename,err_t *err){
    SpxErrReset;
    if(NULL != p->before){
        *err = p->before(p);
        if(0 != *err){
            return NULL;
        }
    }
    FILE *f = fopen(filename,"r");
    if(NULL == f){
        *err = errno;
        return p;
    }

    string_t line = spx_string_newlen(NULL,SpxStringRealSize(SpxLineSize),err);
    if(NULL == line){
        goto r1;
    }

    while(NULL != fgets(line,SpxLineSize,f)){
        spx_string_updatelen(line);
        spx_string_trim(line," ");
        if(!SpxStringBeginWith(line,'#')){
            p->line_deserialize(line,p,err);
        }
        spx_string_clear(line);
    }
    if(NULL != p->after){
        *err = p->after(p);
    }

r1:
    fclose(f);
    return p;
}

void *spx_properties_get(struct spx_properties *p,string_t name,size_t *size){
    return spx_map_get(p->configurtion,name,spx_string_len(name),size);
}

err_t spx_properties_set(struct spx_properties *p,string_t name,void *value,size_t size){
    return spx_map_insert(p->configurtion,name,spx_string_len(name),value,size);
}

string_t spx_properties_tostring(struct spx_properties *p,err_t *err){
    string_t context = NULL;
    struct spx_map_iter *iter = spx_map_iter_new(p->configurtion,err);
    if(NULL == iter){
        return NULL;
    }
    context = spx_string_empty(err);
    if(NULL == context){
    }
    struct spx_map_node *node = NULL;
    while(NULL != (node = spx_map_iter_next(iter,err))){
        if(0 != *err){
        goto r2;
        }
        string_t line =  p->line_serialize(node,err);
        if(NULL == line){
            goto r2;
        }

        string_t new_context = spx_string_cat_string(context,line,err);
        if(NULL == new_context){
            goto r2;
        }
        context = new_context;
    }
    goto r1;
r2:
    spx_string_free(context);
r1:
    spx_map_iter_free(&iter);
    return context;
}

void spx_properties_store(struct spx_properties *p,string_t filename,err_t *err){
    string_t context = spx_properties_tostring(p,err);
    if(NULL == context){
        return;
    }

    string_t basepath = spx_basepath(filename,err);
    if(NULL == basepath){
        goto r1;
    }

    if(!spx_is_dir(basepath,err)){
        if(0 != *err){
            goto r2;
        }
        if(0 != (*err = spx_mkdir(p->log,basepath,SpxPathMode))){
            goto r2;
        }
    }

    FILE *fp = fopen(filename,"a+");
    if(NULL == fp){
        *err = errno;
    }
    size_t len = 0;
    if(0 != (*err = spx_fwrite_string(fp,context,spx_string_len(context),&len))){
        goto r3;
    }

r3:
    fclose(fp);

r2:
    spx_string_free(basepath);
r1:
    spx_string_free(context);
    return;
}

void spx_properties_free(struct spx_properties **p){
    if(NULL != (*p)->configurtion){
        spx_map_free(&((*p)->configurtion));
    }
    SpxFree(*p);
    return;
}

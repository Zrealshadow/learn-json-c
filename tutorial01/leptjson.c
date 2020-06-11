#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <stdlib.h>  /* NULL */
#include <string.h>
#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch) ((ch<='9') && (ch>='0'))
#define ISDIGIT1TO9(ch) ((ch<='9') && (ch>'0'))

typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

#if 0
static int lept_parse_null(lept_context* c, lept_value* v) {
    EXPECT(c, 'n'); 
    /* c->json ++ */
    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_NULL;
    return LEPT_PARSE_OK;
}
static int lept_parse_true(lept_context*c,lept_value* v ){
    EXPECT(c,'t');
    if(c->json[0]!='r' || c->json[1]!='u' || c->json[2]!='e')
        return LEPT_PARSE_INVALID_VALUE;
    c->json+=3;
    v->type=LEPT_TRUE;
    return LEPT_PARSE_OK;
}


static int lept_parse_false(lept_context* c, lept_value *v){
    EXPECT(c,'f');
    if(c->json[0]!='a' || c->json[1]!='l' || c->json[2]!='s' || c->json[3]!='e')
        return LEPT_PARSE_INVALID_VALUE;
    c->json+=4;
    v->type=LEPT_FALSE;
    return LEPT_PARSE_OK;
}
#endif

static int lept_parse_literal(lept_context*c,lept_value*v,char*keyword,lept_type type){
    unsigned long l=strlen(keyword);
    size_t i=0;
    if(strlen(c->json)<l) return LEPT_PARSE_INVALID_VALUE;
    for(;i<l;i++){
        if(c->json[i]!=keyword[i])
            return LEPT_PARSE_INVALID_VALUE;
    }
    c->json+=l;
    v->type=type;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context *c, lept_value *v){
    const char*p=c->json;
    if(*p=='-') p++;
    if(*p==0)p++;
    else{
        /*整数部分*/
        if(!ISDIGIT1TO9(*p))return LEPT_PARSE_INVALID_VALUE;
        while(ISDIGIT(*p))p++;
    }
    if(*p=='.'){
        p++;
        if(!ISDIGIT(*p))return LEPT_PARSE_INVALID_VALUE;
        while(ISDIGIT(*p))p++;
    }

    if(*p=='E' || *p=='e'){
        p++;
        if(*p=='+' || *p=='-') p++;
        if(!ISDIGIT(*p))return LEPT_PARSE_INVALID_VALUE;
        while(ISDIGIT(*p))p++;
    }
    v->n=strtod(c->json,NULL);
    errno=0; /*记录最后一次的错误 代号 ERANGE 代表超出范围 HUGE_VAL代表double无穷大常量*/
    if(errno==ERANGE && (v->n==HUGE_VAL || v->n== -HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;
    v->type=LEPT_NUMBER;
    c->json=p;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 'n':  return lept_parse_literal(c,v,"null",LEPT_NULL);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
        case 't':return lept_parse_literal(c,v,"true",LEPT_TRUE);
        case 'f':return lept_parse_literal(c,v,"false",LEPT_FALSE);
        default:   return lept_parse_number(c,v);
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    ret=lept_parse_value(&c, v);
    if(ret==LEPT_PARSE_OK){
        lept_parse_whitespace(&c);
        if(*c.json!='\0'){
            return LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value *v){
    assert(v!=NULL && v->type==LEPT_NUMBER);
    return v->n;
}

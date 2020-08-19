#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <stdlib.h>  /* NULL */
#include <string.h>

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch) ((ch<='9') && (ch>='0'))
#define ISDIGIT1TO9(ch) ((ch<='9') && (ch>'0'))

typedef struct {
    const char* json;
    char* stack;
    size_t size,top;
}lept_context;

static void* lept_context_push(lept_context *c,size_t size){
    void * ret;
    assert(size>0);
    if (c->top+size>=c->size){
        if(c->size==0){
            c->size=LEPT_PARSE_STACK_INIT_SIZE;
        }
        while(c->top+size>=c->size){
            c->size+= c->size >> 1; //c.size=c.size*1.5
            c->stack=(char*)realloc(c->stack,c->size);
        }
    }
    ret=c->stack+c->top;
    c->top+=size;
    return ret;
}

static void * lept_context_pop(lept_context *c,size_t size){
    assert(c->top>size);
    c->top-=size;
    return c->stack+c->top;
}

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
    if(*p=='0')p++;
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
    errno=0; /*记录最后一次的错误 代号 ERANGE 代表超出范围 HUGE_VAL代表double无穷大常量*/
    v->u.n=strtod(c->json,NULL);
    if(errno==ERANGE && (v->u.n==HUGE_VAL || v->u.n== -HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;
    v->type=LEPT_NUMBER;
    c->json=p;
    return LEPT_PARSE_OK;
}

#define PUTC(c,ch) do {*(char *)lept_context_push(c,sizeof(char))=(ch);} while(0)


static int lept_parse_string(lept_context *c,lept_value *v){
    size_t head=c->top,len;
    const char * p;
    EXPECT(c,'\"');
    p=c->json;
    for(;;){
        char ch=*p++;
        switch (ch)
        {
        case '\"':
            len=c->top-head;
            lept_set_string(v,(const char *) lept_context_pop(c,len),len);
            c->json=p;
            return LEPT_PARSE_OK;
        case '\0':
            c->top=head;
            return LEPT_PARSE_MISS_QUOTATION_MARK;
        case '\\':
            char ch_next=*p++;
            switch(ch_next){
                case '\"': PUTC(c,'\"'); break;
                case '\\': PUTC(c,'\\'); break;
                case '/': PUTC(c,'/');break;
                case 'b': PUTC(c,'\b'); break;
                case 'f': PUTC(c,'\f'); break;
                case 'n': PUTC(c,'\n'); break;
                case 'r': PUTC(c,'\r'); break;
                case 't': PUTC(c,'\t'); break;
                default:
                    c->top=head;
                    return LEPT_PARSE_INVALID_STRING_ESCAPE;
            }
        default:
            if((unsigned char) ch <0x20){
                c->top=head;
                return LEPT_PARSE_INVALID_STRING_CHAR;
            }
            PUTC(c,ch);
        }
    }

    // while((*p)!='\"'){
        
    // }
    
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 'n':  return lept_parse_literal(c,v,"null",LEPT_NULL);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
        case '\"': return lept_parse_string(c,v); 
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
    c.stack=NULL;
    c.size=0;
    c.top=0;
    lept_init(v);
    lept_parse_whitespace(&c);
    if((ret=lept_parse_value(&c,v))==LEPT_PARSE_OK){
        lept_parse_whitespace(&c);
        if(*c.json!='\0'){
            v->type=LEPT_NULL;
            return LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    assert(c.top==0);
    free(c.stack);
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

void lept_free(lept_value *v){
    assert(v!=NULL);
    if(v->type==LEPT_STRING){
        free(v->u.s.s);
    }
    v->type=NULL;
}


void lept_set_number(lept_value*v,double n){
    assert(v!=NULL);
    lept_set_null(v);
    v->type=LEPT_NUMBER;
    v->u.n=n;
}

double lept_get_number(const lept_value *v){
    assert(v!=NULL && v->type==LEPT_NUMBER);
    return v->u.n;
}

void lept_set_string(lept_value *v,const char *s, size_t len){
    assert(v!=NULL && (s!=NULL || len==0));
    lept_free(v);
    v->u.s.s=(char *) malloc(len+1);
    memcpy(v->u.s.s,s,len);
    v->u.s.s[len]='\0';
    v->u.s.len=len;
    v->type=LEPT_STRING;
}

size_t lept_get_string_length(const lept_value *v){
    assert(v!=NULL && (v->type==LEPT_STRING));
    return v->u.s.len;
}

const char * lept_get_string(const lept_value *v){
    assert(v!=NULL && (v->type==LEPT_STRING));
    return v->u.s.s;
}

int lept_get_boolean(const lept_value *v){
    assert(v->type==LEPT_FALSE || v->type==LEPT_FALSE);
    return v->type;
}

void lept_set_boolean(lept_value *v,int b){
    assert(v!=NULL &&(b==LEPT_FALSE || b==LEPT_TRUE));
    v->type=b;
}

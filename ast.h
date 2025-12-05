#ifndef AST
#define AST

#include "inttypes.h"

enum type_type
{
    VAR_SCALAR,
    VAR_CLASS,
    VAR_RECORD,
    VAR_ARRAY,
    VAR_PIPE,
};

struct type_scalar
{
    char *name;
    size_t size;
};

struct type_array
{
    struct type *base;
};

struct type_pipe
{
    struct type *base;
};

struct type_record
{
    char *name;

    int64_t fields_len;
    struct type *fields_type[32];
    char *fields_name[32];
};

struct type_class
{
    char *name;

    /* class is "pointer" type, pointing on "record" type */   
    struct type *record;
};

struct type
{
    enum type_type type;
    size_t size;
    union
    {
        struct type_scalar scalar;
        struct type_array array;
        struct type_pipe pipe;
        struct type_class class;
        struct type_record record;
    };
};


size_t size_of_type(struct type *t);


struct variable
{
    struct type *type;
    char *name;
};


enum expression_type
{
    EXPR_LITERAL_INT,
    EXPR_LITERAL_STRING,
    EXPR_VARIABLE,
    
    EXPR_OP_ADD,
    EXPR_OP_SUB,
    EXPR_OP_MUL,
    EXPR_OP_DIV,
    EXPR_OP_MINUS,
    EXPR_OP_PLUS,
    
    EXPR_OP_AND,
    EXPR_OP_OR,
    EXPR_OP_XOR,
    EXPR_OP_NOT,
    
    EXPR_OP_BAND,
    EXPR_OP_BOR,
    EXPR_OP_BXOR,
    EXPR_OP_BNOT,
};


struct expression
{
    enum expression_type type;
    struct expression *childs[2];
    int64_t            childs_len;
    /* for literals */
    void *data;
};


struct statement_worker
{
    struct worker *worker;
    struct variable *inputs[32];
    struct variable *outputs[32];
};


struct statement_loop
{
    struct expression *expr;
    struct block *block;
};


struct statement_match
{
    struct expression *expr;
    struct variable *cases_default[32];
    struct expression *cases_literal[32];
    struct expression *cases_guard[32];
    struct block *cases_block[32];
    int64_t cases_len;
};

enum statement_type
{
    STMT_BLOCK,
    STMT_LOOP,
    STMT_MATCH,
    STMT_EXPRESSION,
    STMT_WORKER,
    STMT_BREAK,
};

struct statement
{
    enum statement_type type;
    union
    {
        struct block *block;
        struct statement_loop loop;
        struct statement_match match;
        struct expression *expr;
        struct statement_worker worker;
    };
};


struct block
{
    struct block *parent;
    
    struct variable **locals;
    int64_t           locals_len;
    int64_t           locals_alloc;
    
    struct statement **code;
    int64_t            code_len;
    int64_t            code_alloc;
};


struct worker
{
    char *name;
    
    struct variable* inputs[32];
    int64_t         inputs_len;
    struct variable* outputs[32];
    int64_t         outputs_len;
    struct block *body;
};


struct program
{
    struct worker **workers;
    int64_t         workers_len;
    int64_t         workers_alloc;
    
    struct type **types;
    int64_t       types_len;
    int64_t       types_alloc;
};

struct program *parse(const char *s);
struct type *get_expr_type(struct program *p, struct expression *e);

#endif

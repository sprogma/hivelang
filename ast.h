#ifndef AST
#define AST

#include "inttypes.h"

enum type_type
{
    VAR_SCALAR,
    VAR_STRUCT,
    VAR_UNION,
    VAR_ARRAY,
    VAR_PIPE,
};

struct type
{
    enum type_type type;
    struct type *base;
};

struct variable
{
    struct type *type;
};


struct block
{
    struct variable *locals;
    int64_t          locals_len;
    int64_t          locals_alloc;
    struct statement *code;
    int64_t           code_len;
    int64_t           code_alloc;
};


struct worker
{
    struct variable input[32];
    int64_t         input_len;
    struct variable outputs[32];
    int64_t         output_len;
    struct block body;
};


struct program
{
    struct worker *workers;
    int64_t        workers_len;
    int64_t        workers_alloc;
    
    struct type *types;
    int64_t      types_len;
    int64_t      types_alloc;
};

#endif

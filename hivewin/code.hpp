#ifndef CODE_H
#define CODE_H

#include "inttypes.h"
#include <unordered_map>

typedef int64_t type_id;
typedef int64_t worker_id;
typedef int64_t var_id;

enum type_type
{
    VAR_SCALAR,
    VAR_CLASS,
    VAR_RECORD,
    VAR_ARRAY,
    VAR_PIPE,
    VAR_PROMISE,
};

struct type
{
    int64_t id;
    enum type_type type;
    union {
        struct type_scalar {char *name; int64_t size;} scalar;
        struct type_record {char *name; type_id *fields; int64_t fields_len;} record;
        struct type_class {char *name; type_id *fields; int64_t fields_len;} cls;
        struct type_array {type_id base;} array;
        struct type_pipe {type_id base;} pipe;
        struct type_promise {type_id base;} promise;
    };
};

struct variable
{
    int64_t id;
    int64_t type;
};



enum expression_type
{
    EXPR_LITERAL_INT,
    EXPR_LITERAL_STRING,
    EXPR_LITERAL_ARRAY,
    EXPR_ARRAY_INDEX,
    EXPR_VARIABLE,
    EXPR_QUERY,
    EXPR_PUSH,
    EXPR_OP,
};

enum op_type
{
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_AND, OP_NOT, OP_OR, OP_XOR,
    OP_BAND, OP_BNOT, OP_BOR, OP_BXOR,
    OP_LT, OP_LE, OP_GT, OP_GE, OP_EQ, OP_NE,
};

constexpr static inline int64_t op_get_num_childs(enum op_type op)
{
    switch (op)
    {
        case OP_ADD: case OP_SUB: case OP_MUL: case OP_DIV: case OP_MOD:
        case OP_AND: case OP_OR: case OP_XOR:
        case OP_BAND: case OP_BOR: case OP_BXOR:
        case OP_LT: case OP_LE: case OP_GT: case OP_GE: case OP_EQ: case OP_NE:
            return 2;
        case OP_NOT: case OP_BNOT:
            return 1;
    }
}

struct expression
{
    enum expression_type type;
    struct expression *childs[2];
    int64_t            childs_len;
    union
    {
        void *pdata;
        int64_t idata;
    };
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
        struct expression *expr;
        struct statement_loop { struct expression *expr; struct block *block; } loop;
        struct statement_match { struct expression *expr; 
                                 var_id cases_default[32]; 
                                 struct expression *cases_literal[32];
                                 struct expression *cases_guard[32];
                                 struct block *cases_block[32];
                                 int64_t cases_len; } match;
        struct statement_worker { worker_id worker; struct expression *inputs[32]; var_id outputs[32]; } worker;
    };
};


struct block
{
    struct block *parent;
    
    struct variable *locals;
    int64_t          locals_len;
    
    struct statement *code;
    int64_t           code_len;
};


struct worker
{
    int64_t id;
    
    var_id *inputs;
    int64_t inputs_len;
    var_id *outputs;
    int64_t outputs_len;
    struct block *body;

    /* metadata */
    double execution_cost;
    int64_t multiplication;

    /* compiler cache */
    void (*compiled)();
};

struct program
{
    std::unordered_map<int64_t, struct type *>types;
    std::unordered_map<int64_t, struct worker *>workers;
};


struct program *program_from_bytes(uint8_t *encoded);


#endif

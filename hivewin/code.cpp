#include "code.hpp"

#include "string.h"
#include "stdlib.h"


// #define READ_64(ptr) read_many((uint8_t *)ptr, &bytes, 8)
#define READ_64() read64packed(bytes)
#define READ(ptr, count) read_many((uint8_t *)ptr, bytes, count)


int64_t read64packed(uint8_t **bytes)
{
    int64_t res = 0;
    uint8_t *pos = *bytes;
    uint8_t size = *pos;
    pos++;
    if (size & 0x80)
    {
        res = size & ~0x80;
    }
    else
    {
        memcpy(&res, pos, size);
        pos += size;
    }
    *bytes = pos;
    return res;
}

void read_many(uint8_t *ptr, uint8_t **bytes, int64_t size)
{
    memcpy(ptr, *bytes, size);
    *bytes += size;
}



static void read_type(struct program *p, uint8_t **bytes, struct type *t)
{
    t->id = READ_64();
    t->type = (enum type_type)READ_64();
    t->cached_size = 0;
    switch (t->type)
    {
        case VAR_SCALAR:
        {
            int64_t len = READ_64();
            t->scalar.name = (char *)calloc(1, len + 1);
            READ(t->scalar.name, len);
            t->scalar.size = READ_64();
            t->cached_size += t->scalar.size;
            break;
        }
        case VAR_CLASS:
        {
            int64_t len = READ_64();
            t->cls.name = (char *)calloc(1, len + 1);
            READ(t->cls.name, len);
            t->cls.fields_len = READ_64();
            for (int i = 0; i < t->cls.fields_len; ++i)
            {
                t->cls.fields[i] = READ_64();
                t->cached_size += p->types[t->cls.fields[i]]->cached_size;
            }
            break;
        }
        case VAR_RECORD:
        {
            int64_t len = READ_64();
            t->record.name = (char *)calloc(1, len + 1);
            READ(t->record.name, len);
            t->record.fields_len = READ_64();
            for (int i = 0; i < t->record.fields_len; ++i)
            {
                t->record.fields[i] = READ_64();
                t->cached_size += p->types[t->record.fields[i]]->cached_size;
            }
            break;
        }
        case VAR_ARRAY:
            t->array.base = READ_64();
            t->cached_size += 8;
            break;
        case VAR_PROMISE:
            t->promise.base = READ_64();
            t->cached_size += 8;
            break;
        case VAR_PIPE:
            t->pipe.base = READ_64();
            t->cached_size += 8;
            break;
    }
}


static void read_variable(struct program *p, uint8_t **bytes, struct variable *v)
{
    (void)p;
    v->id = READ_64();
    v->type = READ_64();
}


static enum op_type read_op(struct program *p, uint8_t **bytes)
{
    (void)p;
    return (enum op_type)READ_64();
}


static void read_expression(struct program *p, uint8_t **bytes, struct expression **expr)
{
    struct expression *e = *expr = (struct expression *)malloc(sizeof(*e));
    e->type = (enum expression_type)READ_64();
    switch (e->type)
    {
        case EXPR_LITERAL_INT:
            e->idata = READ_64();
            break;
        case EXPR_LITERAL_STRING:
        {    
            int64_t len = READ_64();
            e->pdata = (char *)calloc(1, len + 1);
            READ(e->pdata, len);
            break;
        }
        case EXPR_LITERAL_ARRAY:
            e->idata = READ_64();
            read_expression(p, bytes, &e->childs[0]);
            break;
        case EXPR_ARRAY_INDEX:
            read_expression(p, bytes, &e->childs[0]);
            read_expression(p, bytes, &e->childs[1]);
            break;
        case EXPR_VARIABLE:
            e->idata = READ_64();
            break;
        case EXPR_QUERY:
            read_expression(p, bytes, &e->childs[0]);
            break;
        case EXPR_PUSH:
            read_expression(p, bytes, &e->childs[0]);
            read_expression(p, bytes, &e->childs[1]);
            break;
        case EXPR_OP:
            e->idata = read_op(p, bytes);
            e->childs_len = op_get_num_childs((enum op_type)e->idata);
            if (e->childs_len == 1)
            {
                read_expression(p, bytes, &e->childs[0]);
            }
            else if (e->childs_len == 2)
            {
                read_expression(p, bytes, &e->childs[0]);
                read_expression(p, bytes, &e->childs[1]);
            }
            break;
    }
}

static void read_code(struct program *p, uint8_t **bytes, struct block *b)
{
    b->locals_len = READ_64();
    b->locals = (struct variable *)malloc(sizeof(*b->locals) * b->locals_len);
    for (int i = 0; i < b->locals_len; ++i)
    {
        read_variable(p, bytes, &b->locals[i]);
    }
    b->code_len = READ_64();
    b->code = (struct statement *)malloc(sizeof(*b->code) * b->code_len);
    for (int i = 0; i < b->code_len; ++i)
    {
        b->code[i].type = (enum statement_type)READ_64();
        switch (b->code[i].type)
        {
            case STMT_BLOCK:
                b->code[i].block = (struct block *)calloc(1, sizeof(*b->code[i].block));
                read_code(p, bytes, b->code[i].block);
                break;
            case STMT_LOOP:
                read_expression(p, bytes, &b->code[i].loop.expr);
                b->code[i].loop.block = (struct block *)calloc(1, sizeof(*b->code[i].loop.block));
                read_code(p, bytes, b->code[i].loop.block);
                break;
            case STMT_MATCH:
                read_expression(p, bytes, &b->code[i].match.expr);
                b->code[i].match.cases_len = READ_64();
                for (int j = 0; j < b->code[i].match.cases_len; ++j)
                {
                    int64_t data = READ_64();
                    if (data != 0)
                    {
                        b->code[i].match.cases_default[j] = data;
                    }
                    else
                    {
                        read_expression(p, bytes, &b->code[i].match.cases_literal[j]);
                    }
                    data = READ_64();
                    if (data == 1)
                    {
                        read_expression(p, bytes, &b->code[i].match.cases_guard[j]);
                    }
                    else { }
                    b->code[i].match.cases_block[j] = (struct block *)calloc(1, sizeof(*b->code[i].match.cases_block[j]));
                    read_code(p, bytes, b->code[i].match.cases_block[j]);
                }
                break;
            case STMT_EXPRESSION:
                read_expression(p, bytes, &b->code[i].expr);
                break;
            case STMT_WORKER:
            {
                b->code[i].worker.worker = READ_64();
                int64_t inputs = READ_64();
                for (int j = 0; j < inputs; ++j)
                {
                    read_expression(p, bytes, &b->code[i].worker.inputs[j]);
                }
                int64_t outputs = READ_64();
                for (int j = 0; j < outputs; ++j)
                {
                    b->code[i].worker.outputs[j] = READ_64();
                }
                break;
            }
            case STMT_BREAK:
                break;
        }
    }
}

static void read_worker(struct program *p, uint8_t **bytes, struct worker *w)
{
    w->id = READ_64();
    w->inputs_len = READ_64();
    w->inputs = (var_id *)malloc(sizeof(*w->inputs) * w->inputs_len);
    for (int i = 0; i < w->inputs_len; ++i)
    {
        w->inputs[i] = READ_64();
    }
    w->outputs_len = READ_64();
    w->outputs = (var_id *)malloc(sizeof(*w->outputs) * w->outputs_len);
    for (int i = 0; i < w->outputs_len; ++i)
    {
        w->outputs[i] = READ_64();
    }
    w->body = (struct block *)calloc(1, sizeof(*w->body));
    read_code(p, bytes, w->body);
}

struct program *program_from_bytes(uint8_t *encoded)
{
    uint8_t **bytes = &encoded;
    struct program *res = new struct program; /*(struct program *)malloc(sizeof(*res));

    new(&res->types)std::unordered_map<int64_t, struct type *>;
    new(&res->workers)std::unordered_map<int64_t, struct worker *>;*/
    
    int64_t types_len = READ_64();
    for (int i = 0; i < types_len; ++i)
    {
        struct type *tp = (struct type *)malloc(sizeof(*tp));
        read_type(res, bytes, tp);
        res->types[tp->id] = tp;
    }
    int64_t workers_len = READ_64();
    for (int i = 0; i < workers_len; ++i)
    {
        struct worker *wk = (struct worker *)malloc(sizeof(*wk));
        read_worker(res, bytes, wk);
        res->workers[wk->id] = wk;
    }

    printf("Program read.\n");
    return res;
}

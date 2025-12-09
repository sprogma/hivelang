#include "stdio.h"

#include "code.hpp"
#include "dynamic.hpp"


struct context
{
    std::unordered_map<int64_t, int64_t> offsets;
    std::unordered_map<int64_t, struct variable *> locals;
    int64_t next_address;
    struct hive *h;
    struct execution_worker *ew;
    struct worker *w;
};


struct hive *create_hive(struct program *prg)
{
    struct hive *h = new struct hive; /*(struct hive *)calloc(1, sizeof(*h));

    new(&h->objects)std::unordered_map<object_id, struct object_origin>;
    new(&h->workers)std::unordered_map<worker_id, struct execution_worker *>;
    */
    
    h->program = prg;
    
    return h;
}


void allocate_memory_for_locals(struct context *ctx, struct block *code)
{    
    for (int i = 0; i < code->locals_len; ++i)
    {
        ctx->offsets[code->locals[i].id] = ctx->next_address;
        ctx->locals[code->locals[i].id] = &code->locals[i];
        ctx->next_address += ctx->h->program->types[code->locals[i].type]->cached_size;
    }
}

static void print_op(FILE *f, enum op_type t)
{
    switch (t)
    {
        case OP_ADD: fprintf(f, "+"); break;
        case OP_SUB: fprintf(f, "-"); break;
        case OP_MUL: fprintf(f, "*"); break;
        case OP_DIV: fprintf(f, "/"); break;
        case OP_MOD: fprintf(f, "%%"); break;

        case OP_AND: fprintf(f, "&&"); break;
        case OP_NOT: fprintf(f, "!"); break;
        case OP_OR: fprintf(f, "||"); break;
        case OP_XOR: fprintf(f, "^"); break;

        case OP_BAND: fprintf(f, "&"); break;
        case OP_BNOT: fprintf(f, "~"); break;
        case OP_BOR: fprintf(f, "|"); break;
        case OP_BXOR: fprintf(f, "^"); break;

        case OP_LT: fprintf(f, "<"); break;
        case OP_LE: fprintf(f, "<="); break;
        case OP_GT: fprintf(f, ">"); break;
        case OP_GE: fprintf(f, ">"); break;
        case OP_EQ: fprintf(f, "=="); break;
        case OP_NE: fprintf(f, "!="); break;
    }
    return;
}


type_id get_base_type(struct context *ctx, char *name)
{
    for (auto &x : ctx->h->program->types)
    {
        switch (x.second->type)
        {
            case VAR_SCALAR:
                if (strcmp(x.second->scalar.name, name) == 0) { return x.second->id; }
                break;
            case VAR_CLASS:
                if (strcmp(x.second->cls.name, name) == 0) { return x.second->id; }
                break;
            case VAR_RECORD:
                if (strcmp(x.second->record.name, name) == 0) { return x.second->id; }
                break;
            default:
                break;
        }
    }
    printf("Error - type not found\n");
    exit(1);
}


type_id get_complex_type(struct context *ctx, enum type_type type, struct type *base)
{
    for (int i = 0; i < ctx->result->types_len; ++i)
    {
        switch (type)
        {
            case VAR_ARRAY:
                if (ctx->result->types[i]->type == type && ctx->result->types[i]->array.base == base)
                {
                    return ctx->result->types[i];
                }
                break;
            case VAR_PIPE:
                res->pipe.base = base;
                break;
            case VAR_PROMISE:
                res->promise.base = base;
                break;
            default:
                printf("Error!\n");
                exit(1);
        }
    }
    struct type *res = new_type();
    res->type = type;
    return res;
}


type_id get_expr_type(struct context *ctx, struct expression *e)
{
    switch (e->type)
    {
        case EXPR_VARIABLE: return ctx->locals[e->idata]->type;
        case EXPR_LITERAL_INT: return get_type_id("i32");
        case EXPR_LITERAL_STRING: return get_complex_type(VAR_ARRAY, get_base_type("byte"));
        case EXPR_LITERAL_ARRAY: return get_complex_type(VAR_ARRAY, (struct type *)e->pdata);
        case EXPR_PUSH: return get_expr_type(p, e->childs[1]);
        case EXPR_ARRAY_INDEX:
        {
            struct type *t = get_expr_type(p, e->childs[0]);
            if (t->type != VAR_ARRAY) { printf("Error: Index of something that isn't array\n"); return t; }
            return t->array.base;
        }
        case EXPR_QUERY:
        {
            struct type *t = get_expr_type(p, e->childs[0]);
            if (t->type == VAR_PIPE) { return t->array.base; }
            if (t->type == VAR_PROMISE) { return t->array.base; }
            if (t->type == VAR_ARRAY) { return get_base_type("i64"); }
            printf("Error: Query of something that isn't pipe or array\n");
            return t;
        }
        case EXPR_OP:
        {
            /* if have 2 childs, both types must be same, so return left one */
            if (e->childs_len == 0) { printf("Error: EXPR_OP node without childs\n"); }
            return get_expr_type(p, e->childs[0]);
        }
    }
}



char *generate_expression(char *buf, struct context *ctx, struct expression *e)
{
    buf += sprintf(buf, "(");
    switch (e->type)
    {
        case EXPR_LITERAL_INT:
            buf += sprintf(buf, "%lld", e->idata);
            break;
        case EXPR_LITERAL_STRING:
            buf += sprintf(buf, "strdup(\"%s\")", (char *)e->pdata);
            break;
        case EXPR_LITERAL_ARRAY:
            buf += sprintf(buf, "malloc(%lld * \n", ctx->h->program->types[e->idata]->cached_size);
            buf = generate_expression(buf, ctx, e->childs[0]);
            buf += sprintf(buf, ")");
            break;
        case EXPR_ARRAY_INDEX:
            switch (e->childs[0]->size)
            {
                
            }
            buf += sprintf(buf, "\"index\":null,");
            buf += sprintf(buf, "\"from\":");
            buf = generate_expression(buf, ctx, e->childs[0]);
            buf += sprintf(buf, ",");
            buf += sprintf(buf, "\"at\":");
            buf = generate_expression(buf, ctx, e->childs[1]);
            break;
        case EXPR_VARIABLE:
            buf += sprintf(buf, "\"variable\":%lld", ((struct variable *)e->pdata)->id);
            break;
        case EXPR_QUERY:
            buf += sprintf(buf, "\"query\":"); buf = generate_expression(buf, ctx, e->childs[0]);
            break;
        case EXPR_PUSH:
            buf += sprintf(buf, "\"push\":null,");
            buf += sprintf(buf, "\"to\":");
            buf = generate_expression(buf, ctx, e->childs[0]);
            buf += sprintf(buf, ",");
            buf += sprintf(buf, "\"from\":");
            buf = generate_expression(buf, ctx, e->childs[1]);
            break;
        case EXPR_OP:
            buf += sprintf(buf, "\"op\":");
            print_op(f, e->idata);
            buf += sprintf(buf, ",");
            buf += sprintf(buf, "\"childs\":[");
            if (e->childs_len == 1)
            {
                buf = generate_expression(buf, ctx, e->childs[0]);
            }
            else if (e->childs_len == 2)
            {
                buf = generate_expression(buf, ctx, e->childs[0]);
                buf += sprintf(buf, ",");
                buf = generate_expression(buf, ctx, e->childs[1]);
            }
            buf += sprintf(buf, "]");
            break;
    }
    buf += sprintf(buf, ")");
    return buf;
}



char *generate_code(char *buf, struct context *ctx, struct block *code)
{
    (void)ctx;

    for (int i = 0; i < code->code_len; ++i)
    {
        switch (code->code[i].type)
        {
            case STMT_BLOCK:
            {
                buf += sprintf(buf, "{");
                allocate_memory_for_locals(ctx, code->code[i].block);
                buf = generate_code(buf, ctx, code->code[i].block);
                buf += sprintf(buf, "}");
                break;
            }
            case STMT_LOOP:
            {
                buf += sprintf(buf, "while (");
                generate_expression(buf, ctx, code->code[i].loop.expr);
                buf += sprintf(buf, "){");
                allocate_memory_for_locals(ctx, code->code[i].loop.block);
                buf = generate_code(buf, ctx, code->code[i].loop.block);
                buf += sprintf(buf, "}");
                break;
            }
            case STMT_MATCH:
                break;
            case STMT_EXPRESSION:
                break;
            case STMT_WORKER:
                break;
            case STMT_BREAK:
                break;
        }
    }
    return buf;
}


void compile_worker(struct hive *h, worker_id wk)
{
    struct context ctx;

    ctx.ew = h->workers[wk] = (struct execution_worker *)calloc(1, sizeof(*h->workers[wk]));
    ctx.w = h->program->workers[wk];
    ctx.h = h;


    ctx.ew->time_executed = 0.0;
    ctx.ew->time_executed_prev = 0.0;
    new(&ctx.ew->calls)std::unordered_map<server_id, int64_t>;

    ctx.next_address = 0;

    /* for now, */
    /* gen C code, after that compile it, and use as worker */

    char *text = (char *)malloc(1024 * 64);
    char *end = text;
    end += sprintf(end, "void wk_%lld(unsigned char *input){", ctx.w->id);
    /* distribute memory between locals of 1st level */
    allocate_memory_for_locals(&ctx, ctx.w->body);
    /* parse input into variables */
    {
        int64_t offset = 0;
        for (int i = 0; i < ctx.w->inputs_len; ++i)
        {
            var_id x = ctx.w->inputs[i];
            struct variable *var = NULL;
            for (int j = 0; j < ctx.w->body->locals_len; ++j)
            {
                if (ctx.w->body->locals[j].id == x)
                {
                    var = &ctx.w->body->locals[j];
                }
            }
            if (var != NULL)
            {
                /* copy arg to locals */
                int64_t size = ctx.h->program->types[var->type]->cached_size;
                end += sprintf(end, "memcpy(locals + %lld, input + %lld, %lld);", ctx.offsets[x], offset, size);
                offset += size;
            }
            else
            {
                printf("Error: cant find local variable %lld\n", x);
                ctx.w->compiled = NULL;
                return;
            }
        }
    }
    end = generate_code(end, &ctx, ctx.w->body);
    end += sprintf(end, "}\n");

    /* build */
    FILE *f = fopen("tmp/a.c", "w");
    if (f == NULL)
    {
        printf("Can't open tmp/a.c for write\n");
        ctx.w->compiled = NULL;
        return;
    }
    fwrite(text, 1, end - text, f);
    fclose(f);

    system("cmd /c type tmp\\a.c");
    
    uint8_t *bytes = (uint8_t *)malloc(16 * 1024);
    f = fopen("tmp/a.bin", "rb");
    if (f == NULL)
    {
        printf("Compilation failed [no result file found]\n");
        ctx.w->compiled = NULL;
        return;
    }
    int64_t len = fread(bytes, 1, 16 * 1024, f);
    fclose(f);
    
    ctx.w->compiled = (void (*)(uint8_t *))bytes;
    ctx.w->compiled_len = len;
}

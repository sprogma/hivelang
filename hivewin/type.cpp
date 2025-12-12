#include "stdio.h"

#include "code.hpp"
#include "dynamic.hpp"
#include "context.hpp"


type_id get_base_type(struct context *ctx, const char *name)
{
    for (auto &x : ctx->h->program->types)
    {
        switch (x.second->type)
        {
            case VAR_SCALAR: if (strcmp(x.second->scalar.name, name) == 0) { return x.second->id; } break;
            case VAR_CLASS: if (strcmp(x.second->cls.name, name) == 0) { return x.second->id; } break;
            case VAR_RECORD: if (strcmp(x.second->record.name, name) == 0) { return x.second->id; } break;
            default: break;
        }
    }
    printf("Error - type not found\n");
    exit(1);
}


type_id get_complex_type(struct context *ctx, enum type_type type, type_id base)
{
    for (auto &x : ctx->h->program->types)
    {
        if (x.second->type == type)
        {
            switch (type)
            {
                case VAR_ARRAY:
                    if (x.second->array.base == base)
                        return x.second->id;
                    break;
                case VAR_PIPE:
                    if (x.second->pipe.base == base)
                        return x.second->id;
                    break;
                case VAR_PROMISE:
                    if (x.second->promise.base == base)
                        return x.second->id;
                    break;
                default:
                    printf("Error! [unknown type]\n");
                    exit(1);
            }
        }
    }
    printf("[Error! unknown type]\n");
    exit(1);
}


type_id get_expr_type(struct context *ctx, struct expression *e)
{
    switch (e->type)
    {
        case EXPR_VARIABLE: return ctx->locals[e->idata]->type;
        case EXPR_LITERAL_INT: return get_base_type(ctx, "i32");
        case EXPR_LITERAL_STRING: return get_complex_type(ctx, VAR_ARRAY, get_base_type(ctx, "byte"));
        case EXPR_LITERAL_ARRAY: return get_complex_type(ctx, VAR_ARRAY, e->idata);
        case EXPR_PUSH: return get_expr_type(ctx, e->childs[1]);
        case EXPR_ARRAY_INDEX:
        {
            return ctx->h->program->types[get_expr_type(ctx, e->childs[0])]->array.base;
        }
        case EXPR_QUERY:
        {
            struct type *t = ctx->h->program->types[get_expr_type(ctx, e->childs[0])];
            if (t->type == VAR_PIPE) { return t->array.base; }
            if (t->type == VAR_PROMISE) { return t->array.base; }
            if (t->type == VAR_ARRAY) { return get_base_type(ctx, "i64"); }
            exit(1);
        }
        case EXPR_OP:
        {
            return get_expr_type(ctx, e->childs[0]);
        }
    }
}


size_t get_type_size(struct context *ctx, type_id tid)
{
    struct type *t = ctx->h->program->types[tid];
    if (t->cached_size == 0)
    {
        switch (t->type)
        {
            case VAR_SCALAR:
                t->cached_size = t->scalar.size;
                break;
            case VAR_CLASS:
                t->cached_size = 8;
                break;
            case VAR_RECORD:
                {
                    size_t res = 0;
                    for (int64_t i = 0; i < t->record.fields_len; ++i)
                    {
                        res += get_type_size(ctx, t->record.fields[i]);
                    }
                    t->cached_size = res;
                }
                break;
            case VAR_ARRAY:
                t->cached_size = 8;
                break;
            case VAR_PROMISE:
                t->cached_size = 8;
                break;
            case VAR_PIPE:
                t->cached_size = 8;
                break;
        }
    }
    return t->cached_size;
}

int type_is_integer(struct context *ctx, type_id tid)
{
    struct type *t = ctx->h->program->types[tid];
    return t->type == VAR_SCALAR && 
    (
        strcmp(t->scalar.name, "i64") == 0 ||
        strcmp(t->scalar.name, "i32") == 0 ||
        strcmp(t->scalar.name, "i16") == 0 ||
        strcmp(t->scalar.name, "i8") == 0 ||
        strcmp(t->scalar.name, "byte") == 0 ||
        strcmp(t->scalar.name, "word") == 0 ||
        strcmp(t->scalar.name, "dword") == 0 ||
        strcmp(t->scalar.name, "qword") == 0
    );
}

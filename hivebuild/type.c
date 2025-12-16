#include "ast.h"
#include "stdlib.h"
#include "stdio.h"


size_t size_of_type(struct type *t)
{
    switch (t->type)
    {
        case VAR_SCALAR:
            return t->scalar.size;
        case VAR_CLASS:
            return 8;
        case VAR_RECORD:
            {
                size_t res = 0;
                for (int64_t i = 0; i < t->record.fields_len; ++i)
                {
                    res += size_of_type(t->record.fields_type[i]);
                }
                return res;
            }
        case VAR_ARRAY:
            return 8;
        case VAR_PROMISE:
            return 8;
        case VAR_PIPE:
            return 8;
    }
}


struct type *get_expr_type(struct program *p, struct expression *e)
{
    switch (e->type)
    {
        case EXPR_VARIABLE:
            return ((struct variable *)e->pdata)->type;
        case EXPR_LITERAL_INT:
            return get_base_type("i64");
        case EXPR_LITERAL_STRING:
            return get_complex_type(VAR_ARRAY, get_base_type("byte"));
        case EXPR_LITERAL_ARRAY:
            return get_complex_type(VAR_ARRAY, (struct type *)e->pdata);
        case EXPR_ARRAY_INDEX:
        {
            struct type *t = get_expr_type(p, e->childs[0]);
            if (t->type != VAR_ARRAY)
            {
                printf("Error: Index of something that isn't array\n");
                return t;
            }
            return t->array.base;
        }
        case EXPR_QUERY:
        {
            struct type *t = get_expr_type(p, e->childs[0]);
            if (t->type == VAR_PIPE)
            {
                return t->array.base;
            }
            if (t->type == VAR_PROMISE)
            {
                return t->array.base;
            }
            if (t->type == VAR_ARRAY)
            {
                return get_base_type("i64");
            }
            printf("Error: Query of something that isn't pipe or array\n");
            return t;
        }
        case EXPR_PUSH:
            return get_expr_type(p, e->childs[1]);
        case EXPR_OP:
        {
            /* if have 2 childs, both types must be same, so return left one */
            if (e->childs_len == 0)
            {
                printf("Error: EXPR_OP node without childs\n");
            }
            return get_expr_type(p, e->childs[0]);
        }
    }
}


int is_lvalue(struct program *p, struct expression *e)
{
    (void)p;
    switch (e->type)
    {
        case EXPR_VARIABLE:
            return 1;
        case EXPR_LITERAL_INT:
            return 0;
        case EXPR_LITERAL_STRING:
            return 0;
        case EXPR_LITERAL_ARRAY:
            return 0;
        case EXPR_ARRAY_INDEX:
            return 1;
        case EXPR_QUERY:
            return 0;
        case EXPR_PUSH:
            return 0;
        case EXPR_OP:
            return 0;
    }
}

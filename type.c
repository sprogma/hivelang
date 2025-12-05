#include "ast.h"
#include "stdlib.h"


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
        case VAR_PIPE:
            return 8;
    }
}


struct type *get_expr_type(struct program *p, struct expression *e)
{
    (void)e;
    return p->types[0];
}

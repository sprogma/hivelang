#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "ctype.h"
#include "stdarg.h"


#include "ast.h"

/* to update list use */
/* gc .\parser.c | sls "const\s+char\s*\*\s*\w*\(.*\)\s*$" | % {"$_;"} */


const char *strend(const char *a);
const char *skip_to(const char *a, const char *pattern);
const char *skip_including(const char *a, const char *pattern);
const char *skip_spaces(const char *s);
const char *skip_comments(const char *s);
const char *skip_prefix(const char *s, const char *prefix);
const char *skip_identifer(const char *s);
const char *parse_arg(const char *s);
const char *parse_out(const char *s);
const char *parse_expression(const char *s, const char *e, struct expression *expr);
const char *parse_declaration(const char *s, const char *e);
const char *parse_match_branch(const char *s, struct statement *stmt);
const char *parse_stmt(const char *s);
const char *parse_stmt_block(const char *s);
const char *parse_worker(const char *s);


/* globals */
const char *filename;
const char *code;

struct compilation_context
{
    struct block *curr;
    struct worker *worker;
    
    struct program *result;
} context;


struct block *new_block()
{
    struct block *res = calloc(1, sizeof(*res));
    res->parent = context.curr;
    return res;
}


struct type *new_type()
{
    if (context.result->types_len >= context.result->types_alloc)
    {
        context.result->types_alloc = 2 * context.result->types_alloc + !context.result->types_alloc;
        context.result->types = realloc(context.result->types, sizeof(*context.result->types) * context.result->types_alloc);
    }
    struct type *x = calloc(1, sizeof(*x));
    x->id = context.result->types_len + 1;
    context.result->types[context.result->types_len++] = x;
    return x;
}


struct type *get_base_type(char *name)
{
    for (int i = 0; i < context.result->types_len; ++i)
    {
        switch (context.result->types[i]->type)
        {
            case VAR_SCALAR:
                if (strcmp(context.result->types[i]->scalar.name, name) == 0)
                {
                    return context.result->types[i];
                }
                break;
            case VAR_CLASS:
                if (strcmp(context.result->types[i]->class.name, name) == 0)
                {
                    return context.result->types[i];
                }
                break;
            case VAR_RECORD:
                if (strcmp(context.result->types[i]->record.name, name) == 0)
                {
                    return context.result->types[i];
                }
                break;
            default:
                break;
        }
    }
    return NULL;
}


struct type *get_complex_type(enum type_type type, struct type *base)
{
    for (int i = 0; i < context.result->types_len; ++i)
    {
        if (context.result->types[i]->type == type && context.result->types[i]->array.base == base)
        {
            return context.result->types[i];
        }
    }
    struct type *res = new_type();
    res->type = type;
    switch (type)
    {
        case VAR_ARRAY:
            res->array.base = base;
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
    return res;
}


struct worker *get_worker(const char *name)
{
    for (int i = 0; i < context.result->workers_len; ++i)
    {
        if (strcmp(context.result->workers[i]->name, name) == 0)
        {
            return context.result->workers[i];
        }
    }
    return NULL;
}


struct variable *get_variable_at(struct block *block, const char *name)
{
    for (int i = 0; i < block->locals_len; ++i)
    {
        if (strcmp(block->locals[i]->name, name) == 0)
        {
            return block->locals[i];
        }
    }
    if (block->parent != NULL)
    {
        return get_variable_at(block->parent, name);
    }
    return NULL;
}

struct variable *get_variable(const char *name)
{
    return get_variable_at(context.curr, name);
}


struct variable *new_variable(struct type *type, char *name)
{
    if (context.curr->locals_len >= context.curr->locals_alloc)
    {
        context.curr->locals_alloc = 2 * context.curr->locals_alloc + !context.curr->locals_alloc;
        context.curr->locals = realloc(context.curr->locals, sizeof(*context.curr->locals) * context.curr->locals_alloc);
    }
    struct variable *x = calloc(1, sizeof(*x));
    x->name = name;
    x->type = type;
    x->id = context.result->variable_id++;
    context.curr->locals[context.curr->locals_len++] = x;
    return x;
}


struct expression *new_expr()
{
    return calloc(1, sizeof(struct expression));
}


struct statement *new_statement()
{
    if (context.curr->code_len >= context.curr->code_alloc)
    {
        context.curr->code_alloc = 2 * context.curr->code_alloc + !context.curr->code_alloc;
        context.curr->code = realloc(context.curr->code, sizeof(*context.curr->code) * context.curr->code_alloc);
    }
    struct statement *x = calloc(1, sizeof(*x));
    context.curr->code[context.curr->code_len++] = x;
    return x;
}


struct worker *new_worker()
{
    if (context.result->workers_len >= context.result->workers_alloc)
    {
        context.result->workers_alloc = 2 * context.result->workers_alloc + !context.result->workers_alloc;
        context.result->workers = realloc(context.result->workers, sizeof(*context.result->workers) * context.result->workers_alloc);
    }
    struct worker *res = calloc(1, sizeof(*res));
    context.result->workers[context.result->workers_len++] = res;
    memset(res, 0, sizeof(*res));
    res->id = context.result->workers_len + 1;
    return res;
}


int starts_with(const char *a, const char *pattern)
{
    return strncmp(a, pattern, strlen(pattern)) == 0;
}

const char *strend(const char *a)
{
    return a + strlen(a);
}

const char *skip_to(const char *a, const char *pattern)
{
    const char *res = strstr(a, pattern);
    return res ? res : strend(a);
}

const char *skip_including(const char *a, const char *pattern)
{
    const char *res = strstr(a, pattern);
    return res ? res + strlen(pattern) : strend(a);
}

const char *skip_spaces(const char *s)
{
    while (isspace(*s)){s++;} 
    return s;
}

const char *skip_comments(const char *s)
{
    while (1)
    {
        s = skip_spaces(s);
        if (starts_with(s, "//"))
        {
            s = skip_including(s, "\n");
        }
        else
        {
            break;
        }
    }
    return s;
}

const char *skip_prefix(const char *s, const char *prefix)
{
    return s + strlen(prefix);
}

const char *skip_identifer(const char *s)
{
    while (isalpha(*s) || isdigit(*s) || *s == '_' || *s == '$' || *s == '@' || *s == '!') { s++; }
    return s;
}

char *substring(const char *s, const char *e)
{
    char *r = malloc(e - s + 1);
    memcpy(r, s, e - s);
    r[e - s] = 0;
    return r;
}

int all_digits(const char *s, const char *e)
{
    int res = 1;
    for (const char *t = s; t < e; ++t)
    {
        res &= !!isdigit(*t);
    }
    return res;
}


int log_count = 0;
void log_error(const char *s, const char *format, ...)
{
    if (log_count > 100)
    {
        return;
    }
    log_count++;
    if (log_count == 100)
    {
        printf("Too many errors. Won't print other errors\n");
    }
    const char *rv = s;
    while (rv > code && *rv != '\n') {--rv;}
    int column = s - rv;
    int line = 1;
    for (const char *t = code; t < s; line += (*t++ == '\n')){}
    if (column == 0)
    {
        line++;
    }
    printf("Log at %s:%d:%d\n", filename, line, column);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}


void validate_expression(const char *s, struct expression *expr)
{
    switch (expr->type)
    {
        case EXPR_LITERAL_INT:
            return;
        case EXPR_LITERAL_STRING:
            return;
        case EXPR_LITERAL_ARRAY:
            return;
        case EXPR_VARIABLE:
            return;
        case EXPR_ARRAY_INDEX:
        {
            validate_expression(s, expr->childs[0]);
            validate_expression(s, expr->childs[1]);
            struct type *t1 = get_expr_type(context.result, expr->childs[0]);
            struct type *t2 = get_expr_type(context.result, expr->childs[1]);
            if (t1->type != VAR_ARRAY)
            {
                log_error(s, "Error: Index of something that isn't array\n");
            }
            if (t2->type != VAR_SCALAR && 
                                    strcmp(t2->scalar.name, "i8") != 0 && 
                                    strcmp(t2->scalar.name, "i16") != 0 && 
                                    strcmp(t2->scalar.name, "i32") != 0 && 
                                    strcmp(t2->scalar.name, "i64") != 0 && 
                                    strcmp(t2->scalar.name, "byte") != 0 && 
                                    strcmp(t2->scalar.name, "word") != 0 && 
                                    strcmp(t2->scalar.name, "dword") != 0 && 
                                    strcmp(t2->scalar.name, "qword") != 0)
            {
                log_error(s, "Error: Index from array not using integer type\n");
            }
            return;
        }
        case EXPR_QUERY:
        {
            validate_expression(s, expr->childs[0]);
            struct type *t = get_expr_type(context.result, expr->childs[0]);
            if (t->type != VAR_PIPE && t->type != VAR_ARRAY && t->type != VAR_PROMISE)
            {
                log_error(s, "Error: Query of something that isn't pipe or array\n");
            }
            return;
        }
        case EXPR_PUSH:
        {
            validate_expression(s, expr->childs[0]);
            validate_expression(s, expr->childs[1]);
            struct type *t1 = get_expr_type(context.result, expr->childs[0]);
            struct type *t2 = get_expr_type(context.result, expr->childs[1]);
            if (t1 == t2)
            {
                if (!is_lvalue(context.result, expr->childs[0]))
                {
                    log_error(s, "Error: Push to not lvalue\n");
                }
                return;
            }
            if (t1->type == VAR_PROMISE && t2 == t1->promise.base)
            {
                return;
            }
            if (t1->type == VAR_PIPE && t2 == t1->pipe.base)
            {
                return;
            }
            log_error(s, "Error: Push have different types, and not pushing to pipe of same basetype\n");
        }
        case EXPR_OP:
        {
            if (expr->childs_len == 0)
            {
                log_error(s, "Error: EXPR_OP node without childs\n");
            }
            if (expr->childs_len == 1)
            {
                validate_expression(s, expr->childs[0]);
                return;
            }
            if (expr->childs_len == 2)
            {
                validate_expression(s, expr->childs[0]);
                validate_expression(s, expr->childs[1]);
                struct type *t1 = get_expr_type(context.result, expr->childs[0]);
                struct type *t2 = get_expr_type(context.result, expr->childs[1]);
                if (t1->type == VAR_SCALAR && t2->type == VAR_SCALAR)
                {
                    return;
                }
                if (t1 != t2)
                {
                    log_error(s, "Error: Operator applied to 2 different types [and types aren't both scalars]\n");
                }
                return;
            }
        }

    }
}


const char *parse_type(const char *s, struct type **ptype)
{
    /* 1. type */
    s = skip_spaces(s);
    const char *type_end = skip_identifer(s);
    if (s == type_end)
    {
        log_error(s, "Excepted identifer [type name]\n");
        return s;
    }
    
    char *base_type = substring(s, type_end);
    s = type_end;
    printf("Type: %s\n", base_type);

    struct type *type = get_base_type(base_type);
    *ptype = type;

    if (type == NULL)
    {
        log_error(s, "Unknown type: %s\n", base_type);
    }
    
    
    /* 2. qualifers */
    s = skip_spaces(s);
    while (1)
    {
        s = skip_spaces(s);
        if (starts_with(s, "["))
        {
            s++;
            const char *end = skip_to(s, "]");
            printf("Array[]\n");
            s = end + 1;
            s = skip_spaces(s);

            type = get_complex_type(VAR_ARRAY, type);
            *ptype = type;
        }
        else if (starts_with(s, "|"))
        {
            s++;
            s = skip_spaces(s);
            printf("Pipe\n");

            type = get_complex_type(VAR_PIPE, type);
            *ptype = type;
        }
        else if (starts_with(s, "?"))
        {
            s++;
            s = skip_spaces(s);
            printf("Promise\n");

            type = get_complex_type(VAR_PROMISE, type);
            *ptype = type;
        }
        else
        {
            break;
        }
    }

    return s;
}


const char *parse_arg(const char *s)
{
    struct type *type;
    s = parse_type(s, &type);

    s = skip_spaces(s);
    const char *name_end = skip_identifer(s);
    if (s == name_end)
    {
        log_error(s, "Excepted identifer [variable name]\n");
        return s;
    }
    
    char *name = substring(s, name_end);
    s = name_end;

    printf("arg Name: %s\n", name);

    /* create variable */
    struct variable *var = new_variable(type, name);
    context.worker->inputs[context.worker->inputs_len++] = var;
        
    return s;
}

const char *parse_out(const char *s)
{
    struct type *type;
    s = parse_type(s, &type);

    s = skip_spaces(s);
    const char *name_end = skip_identifer(s);
    if (s == name_end)
    {
        log_error(s, "Excepted identifer [variable name]\n");
        return s;
    }
    
    char *name = substring(s, name_end);
    s = name_end;

    printf("Name: %s\n", name);

    /* create variable */
    struct variable *var = new_variable(type, name);
    context.worker->outputs[context.worker->outputs_len++] = var;
    
    return s;
}


const char *parse_expression(const char *s, const char *e, struct expression *expr)
{
    /* 1. skip spaces - if there is ( and ), then */
    s = skip_spaces(s);
    if (s > e) { s = e; }
    while (e > s && isspace(e[-1])) { --e; }

    if (*s == '(' && e[-1] == ')')
    {
        return parse_expression(s + 1, e - 1, expr);
    }
        
    char *exp = substring(s, e);
    printf("Expression: %s\n", exp);

    /* 1. parse groups, and signs between them */
    {
        int ballance = 0;
        int string = 0;
        const char *pos = NULL, *posend = NULL, *s_reserve = s;
        int prior = -1;
        while (s < e)
        {
            if (string)
            {
                if (*s == '\\') { if (strchr("ntave", s[1]) == NULL) {log_error(s, "Unknown escaped character: <%c>", s[1]);} s += 2; }
                else if (*s == '\"') { s++; string = 0; }
            }
            else
            {
                if (*s == '(') { s++; ballance++; }
                else if (*s == ')') { s++; ballance--; }
                else if (*s == '[') { s++; ballance++; }
                else if (*s == ']') { s++; ballance--; }
                else if (ballance == 0)
                {
                    /* try to parse operator */
                         if (prior <= 999 && starts_with(s, "<-")) { pos = s; prior = 999; posend = s = skip_prefix(s, "<-"); }
                         
                    else if (prior <  250 && starts_with(s, "&&")) { pos = s; prior = 250; posend = s = skip_prefix(s, "&&"); }
                    else if (prior <  249 && starts_with(s, "||")) { pos = s; prior = 249; posend = s = skip_prefix(s, "||"); }
                    else if (prior <  248 && starts_with(s, "^^")) { pos = s; prior = 248; posend = s = skip_prefix(s, "^^"); }
                    
                    else if (prior <  200 && starts_with(s, "<=")) { pos = s; prior = 200; posend = s = skip_prefix(s, "<="); }
                    else if (prior <  199 && starts_with(s, ">=")) { pos = s; prior = 199; posend = s = skip_prefix(s, ">="); }
                    else if (prior <  198 && starts_with(s, "<>")) { pos = s; prior = 198; posend = s = skip_prefix(s, "<>"); }
                    else if (prior <  197 && starts_with(s, "<")) { pos = s; prior = 197; posend = s = skip_prefix(s, "<"); }
                    else if (prior <  196 && starts_with(s, ">")) { pos = s; prior = 196; posend = s = skip_prefix(s, ">"); }
                    else if (prior <  195 && starts_with(s, "=")) { pos = s; prior = 195; posend = s = skip_prefix(s, "="); }
                    
                    else if (prior <  155 && starts_with(s, "+")) { pos = s; prior = 151; posend = s = skip_prefix(s, "+"); }
                    else if (prior <  155 && starts_with(s, "-")) { pos = s; prior = 150; posend = s = skip_prefix(s, "-"); }
                    
                    else if (prior <  105 && starts_with(s, "*")) { pos = s; prior = 102; posend = s = skip_prefix(s, "*"); }
                    else if (prior <  105 && starts_with(s, "/")) { pos = s; prior = 101; posend = s = skip_prefix(s, "/"); }
                    else if (prior <  105 && starts_with(s, "%")) { pos = s; prior = 100; posend = s = skip_prefix(s, "%"); }
                    
                    else if (prior <  75 && starts_with(s, "&")) { pos = s; prior = 75; posend = s = skip_prefix(s, "&"); }
                    else if (prior <  74 && starts_with(s, "|")) { pos = s; prior = 74; posend = s = skip_prefix(s, "|"); }
                    else if (prior <  73 && starts_with(s, "^")) { pos = s; prior = 73; posend = s = skip_prefix(s, "^"); }

                    else if (prior <  50 && starts_with(s, "!!")) { pos = s; prior = 50; posend = s = skip_prefix(s, "!!"); }
                    else if (prior <  49 && starts_with(s, "!")) { pos = s; prior = 49; posend = s = skip_prefix(s, "!"); }
                    
                    else if (prior <  25 && starts_with(s, "?")) { pos = s; prior = 25; posend = s = skip_prefix(s, "?"); }
                    else
                    {
                        s++;
                    }
                }
                else
                {
                    s++;
                }
            }
        }

        if (ballance != 0)
        {
            log_error(s, "Error: not closed ( [ or not opened ) ] detected\n");
        }
        s = s_reserve;

        switch (prior)
        {
            case -1:
                /* term */
                if (*s == '?')
                {
                    expr->type = EXPR_QUERY;
                    expr->childs_len = 1;
                    expr->childs[0] = new_expr();
                    parse_expression(skip_prefix(s, "?"), e, expr->childs[0]);
                }
                else if (*s == '"' && e[-1] == '"')
                {
                    expr->type = EXPR_LITERAL_STRING;
                    expr->childs_len = 0;
                    expr->pdata = substring(s, e);
                }
                else if (e[-1] == ']')
                {
                    const char *brace = e;
                    {
                        int ballance = 0, string = 0;
                        while (brace > s && (*brace != '[' || string == 1 || ballance != 0))
                        {
                            if (string)
                            {
                                     if (brace[-2] == '\\') { brace -= 2; }
                                else if (brace[-1] == '"') { brace--; string = 0; }
                            }
                            else
                            {
                                     if (brace[-1] == ')') { ballance++; brace--; }
                                else if (brace[-1] == '(') { ballance--; brace--; }
                                else if (brace[-1] == ']') { ballance++; brace--; }
                                else if (brace[-1] == '[') { ballance--; brace--; }
                                else { brace--; }
                            }
                        }
                    }
                    if (*brace != '[')
                    {
                        log_error(e - 1, "Found not opened ]\n");
                        return e;
                    }
                    printf("IS ARRAY SUBSCR.\n");
                    expr->type = EXPR_ARRAY_INDEX;
                    expr->childs_len = 2;
                    expr->childs[0] = new_expr();
                    expr->childs[1] = new_expr();
                    parse_expression(s, brace, expr->childs[0]);
                    parse_expression(brace + 1, e - 1, expr->childs[1]);
                }
                else if (all_digits(s, e))
                {
                    printf("IS INT: %s\n", substring(s, e));
                    expr->type = EXPR_LITERAL_INT;
                    expr->childs_len = 0;
                    char *tmp = substring(s, e), *end;
                    expr->idata = strtoll(tmp, &end, 10);
                    if (*end != 0)
                    {
                        log_error(s, "strtoll can't parse integer <%s>\n", tmp);
                    }
                    free(tmp);
                }
                else if (skip_to(s, ":") < e)
                {
                    /* array allocation */
                    const char *colon = skip_to(s, ":");
                    struct type *type;
                    const char *res = parse_type(s, &type);
                    res = skip_spaces(res);
                    if (res != colon)
                    {
                        log_error(res, "In array allocation left subexpression isn't type\n");
                        return e;
                    }
                    expr->type = EXPR_LITERAL_ARRAY;
                    expr->pdata = type;
                    expr->childs[0] = new_expr();
                    parse_expression(colon + 1, e, expr->childs[0]);
                }
                else
                {
                    expr->type = EXPR_VARIABLE;
                    expr->childs_len = 0;
                    char *tmp = substring(s, e);
                    expr->pdata = get_variable(tmp);
                    if (expr->pdata == NULL)
                    {
                        log_error(s, "Error: Variable named <%s> isn't defined\n", tmp);
                    }
                    free(tmp);
                }
                break;
                
            #define PARSE_2CHILD \
                expr->childs_len = 2; \
                expr->childs[0] = new_expr(); \
                expr->childs[1] = new_expr(); \
                parse_expression(s, pos, expr->childs[0]); \
                parse_expression(posend, e, expr->childs[1]);
                
            #define PARSE_1CHILD \
                expr->childs_len = 1; \
                expr->childs[0] = new_expr(); \
                parse_expression(posend, e, expr->childs[0]); \
                
            case 999: expr->type = EXPR_PUSH; PARSE_2CHILD break;
            case 250: expr->type = EXPR_OP; expr->idata = OP_AND; PARSE_2CHILD break;
            case 249: expr->type = EXPR_OP; expr->idata = OP_OR; PARSE_2CHILD break;
            case 248: expr->type = EXPR_OP; expr->idata = OP_XOR; PARSE_2CHILD break;
            case 200: expr->type = EXPR_OP; expr->idata = OP_LE; PARSE_2CHILD break;
            case 199: expr->type = EXPR_OP; expr->idata = OP_GE; PARSE_2CHILD break;
            case 198: expr->type = EXPR_OP; expr->idata = OP_NE; PARSE_2CHILD break;
            case 197: expr->type = EXPR_OP; expr->idata = OP_LT; PARSE_2CHILD break;
            case 196: expr->type = EXPR_OP; expr->idata = OP_GT; PARSE_2CHILD break;
            case 195: expr->type = EXPR_OP; expr->idata = OP_EQ; PARSE_2CHILD break;
            case 151: expr->type = EXPR_OP; expr->idata = OP_ADD; PARSE_2CHILD break;
            case 150: expr->type = EXPR_OP; expr->idata = OP_SUB; PARSE_2CHILD break;
            case 102: expr->type = EXPR_OP; expr->idata = OP_MUL; PARSE_2CHILD break;
            case 101: expr->type = EXPR_OP; expr->idata = OP_DIV; PARSE_2CHILD break;
            case 100: expr->type = EXPR_OP; expr->idata = OP_MOD; PARSE_2CHILD break;
            case 75:  expr->type = EXPR_OP; expr->idata = OP_BAND; PARSE_2CHILD break;
            case 74:  expr->type = EXPR_OP; expr->idata = OP_BOR; PARSE_2CHILD break;
            case 73:  expr->type = EXPR_OP; expr->idata = OP_BXOR; PARSE_2CHILD break;
            case 50:  expr->type = EXPR_OP; expr->idata = OP_NOT; PARSE_1CHILD break;
            case 49:  expr->type = EXPR_OP; expr->idata = OP_BNOT; PARSE_1CHILD break;
            case 25:  expr->type = EXPR_QUERY; PARSE_1CHILD break;
            default:
                printf("Error [at line %d]\n", __LINE__);
                exit(1);
        }
    }
    
    return e;
}


const char *parse_declaration(const char *s, const char *e)
{
    /* 1. skip "decl" string */
    s = skip_spaces(s);
    s = skip_prefix(s, "decl");
    s = skip_spaces(s);
    
    struct type *type;
    s = parse_type(s, &type);

    s = skip_spaces(s);
    const char *name_end = skip_identifer(s);
    if (s == name_end || s >= e)
    {
        log_error(s, "Excepted identifer [variable name]\n");
        return s;
    }
    
    char *name = substring(s, name_end);
    s = name_end;

    printf("Decl Name: %s\n", name);

    /* create variable */
    new_variable(type, name);

    s = skip_spaces(s);
    if (s < e)
    {
        log_error(s, "Found Trash in type declaration\n");
        return e;
    }
    
    return e;
}


const char *parse_match_branch(const char *s, struct statement *stmt)
{
    s = skip_spaces(s);

    int id = stmt->match.cases_len++;

    stmt->match.cases_default[id] = NULL;
    stmt->match.cases_guard[id] = NULL;

    /* open new block, to declare variables in it */
    struct block *restore = context.curr;
    stmt->match.cases_block[id] = context.curr = new_block();
    
    if (starts_with(s, "default "))
    {
        s = skip_prefix(s, "default");
        s = skip_spaces(s);
        
        const char *e = skip_to(s, "{");
        const char *grd = skip_including(s, ":");

        if (grd < e)
        {
            stmt->match.cases_guard[id] = new_expr();
            parse_expression(grd, e, stmt->match.cases_guard[id]);
            /* validate result */
            validate_expression(grd, stmt->match.cases_guard[id]);
            
            char *guard = substring(grd, e);
            printf("Guard: %s\n", guard);
            e = grd - 1;
        }

        const char *end = e;
        while (s < end && isspace(end[-1])) { end--; }
        if (s == end)
        {
            log_error(s, "Error: empty default variable name\n");
        }
        char *name = substring(s, end);
        
        stmt->match.cases_default[id] = new_variable(get_expr_type(context.result, stmt->match.expr), name);

        printf("Default named: <%s>\n", name);
    }
    else
    {
        /* read value */
        const char *e = skip_to(s, "{");
        const char *grd = skip_including(s, ":");

        if (grd < e)
        {
            stmt->match.cases_guard[id] = new_expr();
            parse_expression(grd, e, stmt->match.cases_guard[id]);
            /* validate result */
            validate_expression(grd, stmt->match.cases_guard[id]);
            
            char *guard = substring(grd, e);
            printf("Guard: %s\n", guard);
            e = grd - 1;
        }
        
        stmt->match.cases_literal[id] = new_expr();
        parse_expression(s, e, stmt->match.cases_literal[id]);
        /* validate result */
        validate_expression(s, stmt->match.cases_literal[id]);

        char *branch = substring(s, e);

        printf("Branch: %s\n", branch);
    }
    
    s = skip_to(s, "{");
    
    s = parse_stmt_block(s);
    context.curr = restore;
    return s;
}


void parse_worker_call(const char *s, const char *e, struct statement *stmt)
{
    (void)e;
    
    s = skip_spaces(s);

    if (*s != '(')
    {
        log_error(s, "Error: worker call not started by '('\n"); return;
    }
    /* skip braces - find worker name */
    const char *end_brace = s;
    {
        int bal = 1, str = 0; end_brace++;
        while (str || *end_brace != ')' || bal != 1)
        {
            if (str)
            {
                     if (*end_brace == '\\') end_brace++;
                else if (*end_brace == '"') str = 0;
                end_brace++;
            }
            else
            {
                     if (*end_brace == '"') str = 1;
                else if (*end_brace == ')') bal--;
                else if (*end_brace == '(') bal++;
            }
            end_brace++;
        }
    }
    const char *name_pos = skip_spaces(end_brace + 1);
    const char *name_end = skip_identifer(name_pos);
    char *name = substring(name_pos, name_end);

    /* find worker by name */
    printf("Call worker %s\n", name);
    struct worker *w = get_worker(name);
    if (w == NULL)
    {
        log_error(name_pos, "Error: worker with name <%s> isn't defined\n", name);
        return;
    }
    free(name);

    stmt->worker.worker = w;
    
    /* find all input variables */
    {
        const char *pos = s + 1, *end = NULL;
        int id = 0;
        while (pos < end_brace)
        {
            end = skip_to(pos, ",");
            if (end > end_brace) { end = end_brace; }
            struct expression *expr = new_expr();
            parse_expression(pos, end, expr);
            /* validate result */
            validate_expression(pos, expr);
            stmt->worker.inputs[id] = expr;
            pos = end + 1;
            id++;
        }
        if (id != w->inputs_len)
        {
            log_error(end_brace, "Error: Waited for %d inputs for %s, but have %d inputs\n", w->inputs_len, w->name, id);
        }
    }

    /* declare all output variables */
    {
        const char *pos = skip_spaces(name_end), *end = NULL, *last; 
        int id = 0;
        last = skip_to(pos, ")");
        if (*pos != '(')
        {
            log_error(pos, "Expected ( after worker name [output list]\n");
        }
        pos++;
        while (pos < last)
        {
            end = skip_to(pos, ",");
            if (end > last) { end = last; }
            char *name = substring(pos, end);
            printf("Output named %s\n", name);
            struct variable *var = new_variable(w->outputs[id]->type, name);
            stmt->worker.outputs[id] = var;
            pos = end + 1;
            id++;
        }
        if (id != w->outputs_len)
        {
            log_error(end_brace, "Error: Waited for %d outputs for %s, but have %d outputs\n", w->outputs_len, w->name, id);
        } 
    }
    
    
}


const char *parse_stmt(const char *s)
{
    s = skip_spaces(s);
    
    if (starts_with(s, "while"))
    {
        s = skip_prefix(s, "while");

        struct statement *stmt = new_statement();
        stmt->type = STMT_LOOP;
        stmt->loop.expr = new_expr();
        
        /* parse guard expression */
        const char *e = skip_to(s, "{");
        s = parse_expression(s, e, stmt->loop.expr);

        printf("While guard parsed\n");
        
        s = e;
        /* parse body */

        struct block *restore = context.curr;
        stmt->loop.block = context.curr = new_block();
        s = parse_stmt_block(s);
        context.curr = restore;
        
        printf("While body parsed\n");
        
        return s;
    }
    else if (starts_with(s, "match"))
    {
        s = skip_prefix(s, "match");

        struct statement *stmt = new_statement();
        stmt->type = STMT_MATCH;
        stmt->match.expr = new_expr();
        
        /* parse guard expression */
        const char *e = skip_to(s, "{");
        s = parse_expression(s, e, stmt->match.expr);

        printf("Match guard parsed\n");
        
        s = e + 1;
        /* parse body */
        while (1)
        {
            s = skip_spaces(s);
            if (starts_with(s, "}")) { s++; break; }
            s = parse_match_branch(s, stmt);
            s = skip_spaces(s);
            if (starts_with(s, "}")) { s++; break; }
        }
        
        printf("Match body parsed\n");
        
        return s;
    }
    else if (starts_with(s, "break"))
    {
        struct statement *stmt = new_statement();
        stmt->type = STMT_BREAK;
        printf("Parsed break\n");
        const char *e = skip_including(s, ";");
        return e;
    }
    else if (starts_with(s, "decl "))
    {
        /* not generate stmt */
        const char *e = skip_to(s, ";");
        if (!starts_with(e, ";"))
        {
            log_error(s, "Excepted ; after declaration\n");
        }
        s = parse_declaration(s, e);
        s = e + 1;
        return s;
    }
    else if (starts_with(s, "~"))
    {
        s = skip_prefix(s, "~");
        const char *e = skip_to(s, ";");
        struct statement *stmt = new_statement();
        stmt->type = STMT_WORKER;
        parse_worker_call(s, e, stmt);
        return e + 1;
    }
    else
    {
        /* expression */
        const char *e = skip_to(s, ";");
        if (!starts_with(e, ";"))
        {
            log_error(s, "Excepted ; after expression\n");
        }
            
        struct statement *stmt = new_statement();
        stmt->type = STMT_EXPRESSION;
        stmt->expr = new_expr();
        s = parse_expression(s, e, stmt->expr);
        /* validate result */
        validate_expression(s, stmt->expr);
        s = e + 1;
        return s;
    }
}


const char *parse_stmt_block(const char *s)
{
    s = skip_spaces(s);
    if (!starts_with(s, "{"))
    {
        log_error(s, "Parsing error: excepted '{' in code block declaration\n");
        return s + 1;
    }
    
    s++;
    while (1)
    {
        s = skip_spaces(s);
        if (starts_with(s, "}")) { s++; break; }
        s = parse_stmt(s);
    }
    
    return s;    
}


const char *parse_worker(const char *s)
{   
    context.curr = NULL;
    context.curr = new_block();
    context.worker = new_worker();
    context.worker->inputs_len = 0;
    context.worker->outputs_len = 0;
    context.worker->body = context.curr;
    
    s = skip_comments(s);
    if (!starts_with(s, "("))
    {
        log_error(s, "Parsing error: excepted '(' in inputs declaration\n");
        if (*s != 0) { s++; }
        return s;
    }
    
    /* parse inputs */
    s++;
    while (1)
    {
        s = skip_spaces(s);
        if (starts_with(s, ")")) { s++; break; }
        s = parse_arg(s);
        s = skip_spaces(s);
        if (starts_with(s, ")")) { s++; break; }
        s = skip_including(s, ";");
    }

    printf("Inputs parsed\n");

    /* parse name */
    s = skip_spaces(s);
    const char *end = skip_identifer(s);

    char *name = substring(s, end);
    printf("Worker Name: %s\n", name);

    context.worker->name = name;
    
    s = end;
    
    /* parse outputs */
    s = skip_spaces(s);
    if (!starts_with(s, "("))
    {
        log_error(s, "Parsing error: excepted '(' in outputs declaration\n");
        return s;
    }
    s++;
    while (1)
    {
        s = skip_spaces(s);
        if (starts_with(s, ")")) { s++; break; }
        s = parse_out(s);
        s = skip_spaces(s);
        if (starts_with(s, ")")) { s++; break; }
        s = skip_including(s, ";");
    }

    printf("Outputs parsed\n");

    /* all outputs must be pipes or promises for now */
    for (int i = 0; i < context.worker->outputs_len; ++i)
    {
        if (context.worker->outputs[i]->type->type != VAR_PIPE && context.worker->outputs[i]->type->type != VAR_PROMISE)
        {
            log_error(s, "Error: for now all return types must be PIPEs or PROMISEs.\n");
        }
    }

    /* parse body */

    s = parse_stmt_block(s);

    printf("Body parsed\n");

    context.curr = NULL;
    context.worker = NULL;
    
    return s;
}


struct program *parse(const char *s)
{
    log_count = 0;
    code = s;
    filename = "a.test";

    context.result = calloc(1, sizeof(*context.result));
    context.result->variable_id = 1;

    {
        /* default types */
        struct type *t;

        t = new_type();
        t->type = VAR_SCALAR;
        t->scalar.name = "i64";
        t->scalar.size = 8;

        t = new_type();
        t->type = VAR_SCALAR;
        t->scalar.name = "i32";
        t->scalar.size = 4;

        t = new_type();
        t->type = VAR_SCALAR;
        t->scalar.name = "i16";
        t->scalar.size = 2;

        t = new_type();
        t->type = VAR_SCALAR;
        t->scalar.name = "i8";
        t->scalar.size = 1;
        
        t = new_type();
        t->type = VAR_SCALAR;
        t->scalar.name = "qword";
        t->scalar.size = 8;
        
        t = new_type();
        t->type = VAR_SCALAR;
        t->scalar.name = "dword";
        t->scalar.size = 4;
        
        t = new_type();
        t->type = VAR_SCALAR;
        t->scalar.name = "word";
        t->scalar.size = 2;
        
        t = new_type();
        t->type = VAR_SCALAR;
        t->scalar.name = "byte";
        t->scalar.size = 1;
    }
    
    /* 1. read all functions */
    while (1)
    {
        s = parse_worker(s);
        s = skip_spaces(s);
        if (log_count >= 100)
        {
            break;
        }
        if (*s == 0)
        {
            break;
        }
    }

    if (log_count != 0)
    {
        return NULL;
    }

    return context.result;
}

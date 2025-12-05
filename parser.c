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
const char *parse_function(const char *s);


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
    context.result->types[context.result->types_len++] = x;
    return x;
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
    while (isalpha(*s) || isdigit(*s) || *s == '_' || *s == '$' || *s == '@') { s++; }
    return s;
}

char *substring(const char *s, const char *e)
{
    char *r = malloc(e - s + 1);
    memcpy(r, s, e - s);
    r[e - s] = 0;
    return r;
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

    struct type *type = *ptype = new_type();
    /* find name in type table */
    // TODO: this
    type->type = VAR_SCALAR;
    type->scalar.name = "int"; 
    type->scalar.size = 4; 
    
    
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

            type = new_type();
            type->type = VAR_ARRAY;
            type->array.base = *ptype; 
            *ptype = type;
        }
        else if (starts_with(s, "|"))
        {
            s++;
            s = skip_spaces(s);
            printf("Pipe\n");

            type = new_type();
            type->type = VAR_PIPE;
            type->array.base = *ptype; 
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
    printf("SET ARG ADDRESS: %p [%p=%s]\n", var, var->name, var->name);
        
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
    char *exp = substring(s, e);
    printf("Expression: %s\n", exp);
    
    expr->type = EXPR_LITERAL_STRING;
    expr->data = exp;
    
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

    if (starts_with(s, "default "))
    {
        s = skip_prefix(s, "default");
        s = skip_spaces(s);
        
        const char *e = skip_to(s, "{");
        const char *grd = skip_to(s, ":");

        if (grd < e)
        {
            stmt->match.cases_guard[id] = new_expr();
            parse_expression(grd, e, stmt->match.cases_guard[id]);
            
            char *guard = substring(grd, e);
            printf("Guard: %s\n", guard);
            e = grd;
        }
        
        char *name = substring(s, e);
        
        stmt->match.cases_default[id] = new_variable(get_expr_type(context.result, stmt->match.expr), name);

        printf("Default named: %s\n", name);
    }
    else
    {
        /* read value */
        const char *e = skip_to(s, "{");
        const char *grd = skip_to(s, ":");

        if (grd < e)
        {
            stmt->match.cases_guard[id] = new_expr();
            parse_expression(grd, e, stmt->match.cases_guard[id]);
            
            char *guard = substring(grd, e);
            printf("Guard: %s\n", guard);
            e = grd;
        }
        
        stmt->match.cases_literal[id] = new_expr();
        parse_expression(s, e, stmt->match.cases_literal[id]);

        char *branch = substring(s, e);

        printf("Branch: %s\n", branch);
    }
    
    s = skip_to(s, "{");
    
    struct block *restore = context.curr;
    stmt->match.cases_block[id] = context.curr = new_block();
    s = parse_stmt_block(s);
    context.curr = restore;
    return s;
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
    else if (starts_with(s, "decl"))
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


const char *parse_function(const char *s)
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
    
    /* 1. read all functions */
    while (1)
    {
        s = parse_function(s);
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

    return context.result;
}

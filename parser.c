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
const char *parse_expression(const char *s, const char *e);
const char *parse_declaration(const char *s, const char *e);
const char *parse_match_branch(const char *s);
const char *parse_stmt(const char *s);
const char *parse_stmt_block(const char *s);
const char *parse_function(const char *s);


/* globals */
const char *filename;
const char *code;


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


const char *parse_arg(const char *s)
{
    /* 1. type */
    s = skip_spaces(s);
    const char *type_end = skip_identifer(s);
    if (s == type_end)
    {
        log_error(s, "Excepted identifer [type name]\n");
        return s;
    }
    
    char *type = substring(s, type_end);
    s = type_end;
    printf("Type: %s\n", type);
    
    /* 2. qualifers */
    s = skip_spaces(s);
    while (starts_with(s, "["))
    {
        s++;
        const char *end = skip_to(s, "]");
        printf("Array[]\n");
        s = end + 1;
        s = skip_spaces(s);
    }
    s = skip_spaces(s);
    while (starts_with(s, "|"))
    {
        s++;
        s = skip_spaces(s);
        printf("Pipe\n");
    }

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
    
    return s;
}


const char *parse_expression(const char *s, const char *e)
{
    char *exp = substring(s, e);
    printf("Expression: %s\n", exp);
    return e;
}


const char *parse_declaration(const char *s, const char *e)
{
    char *exp = substring(s, e);
    printf("Declaration: %s\n", exp);
    return e;
}


const char *parse_match_branch(const char *s)
{
    s = skip_spaces(s);

    if (starts_with(s, "default "))
    {
        s = skip_prefix(s, "default");
        s = skip_spaces(s);
        
        const char *e = skip_to(s, "{");

        char *name = substring(s, e);

        printf("Default named: %s\n", name);
    }
    else
    {
        /* read value */
        const char *e = skip_to(s, "{");

        char *branch = substring(s, e);

        printf("Branch: %s\n", branch);
    }
    
    s = skip_to(s, "{");
    s = parse_stmt_block(s);
    return s;
}


const char *parse_stmt(const char *s)
{
    s = skip_spaces(s);
    
    if (starts_with(s, "while"))
    {
        s = skip_prefix(s, "while");
        
        /* parse guard expression */
        const char *e = skip_to(s, "{");
        s = parse_expression(s, e);

        printf("While guard parsed\n");
        
        s = e;
        /* parse body */
        s = parse_stmt_block(s);
        
        printf("While body parsed\n");
        
        return s;
    }
    else if (starts_with(s, "match"))
    {
        s = skip_prefix(s, "match");
        
        /* parse guard expression */
        const char *e = skip_to(s, "{");
        s = parse_expression(s, e);

        printf("Match guard parsed\n");
        
        s = e + 1;
        /* parse body */
        while (1)
        {
            s = skip_spaces(s);
            if (starts_with(s, "}")) { s++; break; }
            s = parse_match_branch(s);
            s = skip_spaces(s);
            if (starts_with(s, "}")) { s++; break; }
        }
        
        printf("Match body parsed\n");
        
        return s;
    }
    else if (starts_with(s, "break"))
    {
        printf("Parsed break\n");
        const char *e = skip_including(s, ";");
        return e;
    }
    else if (starts_with(s, "decl"))
    {
        const char *e = skip_to(s, ";");
        if (!starts_with(e, ";"))
        {
            log_error(s, "Excepted ; after declaration\n");
        }
        s = parse_declaration(s, e);
        s = e + 1;
        return s;
    }
    /* expression */
    const char *e = skip_to(s, ";");
    if (!starts_with(e, ";"))
    {
        log_error(s, "Excepted ; after expression\n");
    }
    s = parse_expression(s, e);
    s = e + 1;
    return s;
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
        s = parse_arg(s);
        s = skip_spaces(s);
        if (starts_with(s, ")")) { s++; break; }
        s = skip_including(s, ";");
    }

    printf("Outputs parsed\n");

    /* parse body */

    s = parse_stmt_block(s);

    printf("Body parsed\n");
    
    return s;
}


void parse(const char *s)
{
    log_count = 0;
    code = s;
    filename = "a.test";
    
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
}


int main()
{
    const char *s = R"code(

        (int a; int b)range(int| y)
        {
            decl int t;
            t = a;
            while t <= b
            {
                y <- t;
                t <- t + 1;
            }
        }
    
        (int| x)primes(int| y)
        {
            while (1)
            {
                decl int t;
                t <- ?x;
                decl int i;
                i <- 2;
                while i < t
                {
                    match (t % i = 0)
                    {
                        -1 -> {break;}
                    }
                    i = i + 1;
                }
                match i = t
                {
                    0 -> {y <- t;}
                }
            }
        }

        (int| x)factor(int[]| y)
        {
            while 1
            {
                decl int z;
                z <- 0;
                decl int[] t;
                t <- int[50];
                declbint i;
                i <- ?x;
                (2, i)range(k);
                (k)primes(u);
                while i != 1
                {
                    let d = ?u;
                    while i % d = 0
                    {
                        i = i / d;
                        t[z] = d;
                        z = z + 1;
                    }
                }
                y <- t;
            }
        }

        (char[] x)atoi@libc(int y) {}

        (char[]| s)main()
        {
            (t)factor(z);
            while (1)
            {
                decl char[] x;
                x <- ?s;
                (x)atoi@libc.dll(y);
                (in)factor(out);
                in <- y;
                decl int k;
                k <- ?out;
                match z
                {
                    default p -> { ("%d", p)printf(); }
                }
            }
        }

    

)code";

    parse(s);
    printf("Parsed!\n");
    
    return 0;
}

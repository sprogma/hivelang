#include "ast.h"
#include "export.h"

#include "stdio.h"
#include "stdlib.h"


static void print_type(struct type *t)
{
    switch (t->type)
    {
        case VAR_SCALAR:
            printf("Scalar %s [size %lld] ", t->scalar.name, size_of_type(t));
            break;
        case VAR_CLASS:
            printf("Class %s [size %lld] ", t->class.name, size_of_type(t));
            break;
        case VAR_RECORD:
            printf("Record %s [size %lld] ", t->record.name, size_of_type(t));
            break;
        case VAR_ARRAY:
            printf("Array [size %lld] of type ", size_of_type(t));
            print_type(t->array.base);
            break;
        case VAR_PIPE:
            printf("Pipe [size %lld] of type ", size_of_type(t));
            print_type(t->pipe.base);
            break;
    }
}


static void print_variable(struct variable *v)
{
    printf("Var %s of type: ", v->name);
    print_type(v->type);
}


static void print_op(enum op_type t)
{
    switch (t)
    {
        case OP_ADD: printf(" ADD "); break;
        case OP_SUB: printf(" SUB "); break;
        case OP_MUL: printf(" MUL "); break;
        case OP_DIV: printf(" DIV "); break;
        case OP_MOD: printf(" MOD "); break;

        case OP_AND: printf(" AND "); break;
        case OP_NOT: printf(" NOT "); break;
        case OP_OR: printf(" OR "); break;
        case OP_XOR: printf(" XOR "); break;

        case OP_BAND: printf(" BAND "); break;
        case OP_BNOT: printf(" BNOT "); break;
        case OP_BOR: printf(" BOR "); break;
        case OP_BXOR: printf(" BXOR "); break;

        case OP_LT: printf(" LT "); break;
        case OP_LE: printf(" LE "); break;
        case OP_GT: printf(" GT "); break;
        case OP_GE: printf(" GE "); break;
        case OP_EQ: printf(" EQ "); break;
        case OP_NE: printf(" NE "); break;
    }
    return;
}


static void print_expression(struct expression *e)
{
    printf("(");
    switch (e->type)
    {
        case EXPR_LITERAL_INT:
            printf("%lld", e->idata);
            break;
        case EXPR_LITERAL_STRING:
            printf("\"%s\"", (char *)e->pdata);
            break;
        case EXPR_LITERAL_ARRAY:
            printf("<type at %p>:", e->pdata); print_expression(e->childs[0]);
            break;
        case EXPR_ARRAY_INDEX:
            print_expression(e->childs[0]); printf("["); print_expression(e->childs[1]); printf("]");
            break;
        case EXPR_VARIABLE:
            printf("%s", ((struct variable *)e->pdata)->name);
            break;
        case EXPR_QUERY:
            printf("?"); print_expression(e->childs[0]);
            break;
        case EXPR_PUSH:
            print_expression(e->childs[0]); printf(" <- "); print_expression(e->childs[1]);
            break;
        case EXPR_OP:
            if (e->childs[0] != NULL){ print_expression(e->childs[0]);}
            print_op((enum op_type)e->idata);
            if (e->childs[1] != NULL){ print_expression(e->childs[1]);}
            break;
    }
    printf(")");
}

static void print_code_indented(struct block *b, int indent)
{
    #define P {for (int i = 0; i < indent; ++i) putchar(' ');}
    P printf("Block %p ", b); printf("[parent %p]\n", b->parent);
    for (int i = 0; i < b->locals_len; ++i)
    {
        P printf("local %2d: ", i); print_variable(b->locals[i]);
        printf("\n");
    }
    P printf("...\n");
    for (int i = 0; i < b->code_len; ++i)
    {
        switch (b->code[i]->type)
        {
            case STMT_BLOCK:
                P printf("Block:\n");
                print_code_indented(b->code[i]->block, indent + 4);
                break;
            case STMT_LOOP:
                P printf("Loop: guard: "); print_expression(b->code[i]->loop.expr); printf("\n");
                P printf("Block:\n");
                print_code_indented(b->code[i]->loop.block, indent + 4);
                break;
            case STMT_MATCH:
                P printf("Match: guard: "); print_expression(b->code[i]->match.expr); printf("\n");
                P printf("cases:\n");
                for (int j = 0; j < b->code[i]->match.cases_len; ++j)
                {
                    P 
                    printf("case <"); 
                    if (b->code[i]->match.cases_default[j] != NULL)
                    {
                        printf("default:");
                        print_variable(b->code[i]->match.cases_default[j]);
                    }
                    else
                    {
                        print_expression(b->code[i]->match.cases_literal[j]);
                    }
                    if (b->code[i]->match.cases_guard[j] != NULL)
                    {
                        printf("> where <"); 
                        {
                            print_expression(b->code[i]->match.cases_guard[j]);
                        }
                    }
                    printf("> then\n"); 
                    print_code_indented(b->code[i]->match.cases_block[j], indent + 4);
                }
                break;
            case STMT_EXPRESSION:
                P printf("Expr:"); print_expression(b->code[i]->expr); printf("\n");
                break;
            case STMT_WORKER:
                P printf("Call: %s\n", b->code[i]->worker.worker->name);
                break;
            case STMT_BREAK:
                P printf("Break.\n");
                break;
        }
    }
    #undef P
}

static void print_code(struct block *b)
{
    print_code_indented(b, 4);
}

static void print_worker(struct worker *w)
{
    printf("Worker %s\n", w->name);
    for (int i = 0; i < w->inputs_len; ++i)
    {
        printf("Input  %2d: ", i);
        print_variable(w->inputs[i]);
        printf("\n");
    }
    for (int i = 0; i < w->outputs_len; ++i)
    {
        printf("Output %2d: ", i);
        print_variable(w->outputs[i]);
        printf("\n");
    }
    printf("\n");
    print_code(w->body);
    printf("\n");
    printf("\n");
}


int export_program_bytes(struct program *program, const char *filename)
{
    for (int i = 0; i < program->workers_len; ++i)
    {
        print_worker(program->workers[i]);
    }
    return 0;
}

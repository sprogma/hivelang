#include "ast.h"
#include "workflow.h"

#include "stdio.h"
#include "stdlib.h"


static void print_type(struct type *t)
{
    switch (t->type)
    {
        case VAR_SCALAR:
            printf("Scalar %s [size %lld] ", t->scalar.name, t->size);
            break;
        case VAR_CLASS:
            printf("Class %s [size %lld] ", t->class.name, t->size);
            break;
        case VAR_RECORD:
            printf("Record %s [size %lld] ", t->record.name, t->size);
            break;
        case VAR_ARRAY:
            printf("Array [size %lld] of type ", t->size);
            print_type(t->array.base);
            break;
        case VAR_PIPE:
            printf("Pipe [size %lld] of type ", t->size);
            print_type(t->pipe.base);
            break;
    }
}


static void print_variable(struct variable *v)
{
    printf("Var %s of type: ", v->name);
    print_type(v->type);
    printf("\n");
}

static void print_expression(struct expression *e)
{
    printf("[Expr %d]", e->type);
    for (int i = 0; i < e->childs_len; ++i)
    {
        print_expression(e->childs[i]);
    }
}

static void print_code_indented(struct block *b, int indent)
{
    #define P {for (int i = 0; i < indent; ++i) putchar(' ');}
    P printf("Block %p ", b); printf("[parent %p]\n", b->parent);
    for (int i = 0; i < b->locals_len; ++i)
    {
        P printf("local %2d: ", i); print_variable(b->locals[i]);
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
    }
    for (int i = 0; i < w->outputs_len; ++i)
    {
        printf("Output %2d: ", i);
        print_variable(w->outputs[i]);
    }
    printf("\n");
    print_code(w->body);
    printf("\n");
    printf("\n");
}

void print_workflow(struct program *p)
{
    for (int i = 0; i < p->workers_len; ++i)
    {
        print_worker(p->workers[i]);
    }
}


void build_workflow(struct program *p)
{
    print_workflow(p);
}

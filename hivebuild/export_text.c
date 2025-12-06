#include "ast.h"
#include "export.h"

#include "stdio.h"
#include "stdlib.h"


static void print_type(FILE *f, struct type *t)
{
    switch (t->type)
    {
        case VAR_SCALAR:
            fprintf(f, "Scalar %s [size %lld] ", t->scalar.name, size_of_type(t));
            break;
        case VAR_CLASS:
            fprintf(f, "Class %s [size %lld] ", t->class.name, size_of_type(t));
            break;
        case VAR_RECORD:
            fprintf(f, "Record %s [size %lld] ", t->record.name, size_of_type(t));
            break;
        case VAR_ARRAY:
            fprintf(f, "Array [size %lld] of type ", size_of_type(t));
            print_type(f, t->array.base);
            break;
        case VAR_PIPE:
            fprintf(f, "Pipe [size %lld] of type ", size_of_type(t));
            print_type(f, t->pipe.base);
            break;
    }
}


static void print_variable(FILE *f, struct variable *v)
{
    fprintf(f, "Var %s of type: ", v->name);
    print_type(f, v->type);
}


static void print_op(FILE *f, enum op_type t)
{
    switch (t)
    {
        case OP_ADD: fprintf(f, " ADD "); break;
        case OP_SUB: fprintf(f, " SUB "); break;
        case OP_MUL: fprintf(f, " MUL "); break;
        case OP_DIV: fprintf(f, " DIV "); break;
        case OP_MOD: fprintf(f, " MOD "); break;

        case OP_AND: fprintf(f, " AND "); break;
        case OP_NOT: fprintf(f, " NOT "); break;
        case OP_OR: fprintf(f, " OR "); break;
        case OP_XOR: fprintf(f, " XOR "); break;

        case OP_BAND: fprintf(f, " BAND "); break;
        case OP_BNOT: fprintf(f, " BNOT "); break;
        case OP_BOR: fprintf(f, " BOR "); break;
        case OP_BXOR: fprintf(f, " BXOR "); break;

        case OP_LT: fprintf(f, " LT "); break;
        case OP_LE: fprintf(f, " LE "); break;
        case OP_GT: fprintf(f, " GT "); break;
        case OP_GE: fprintf(f, " GE "); break;
        case OP_EQ: fprintf(f, " EQ "); break;
        case OP_NE: fprintf(f, " NE "); break;
    }
    return;
}


static void print_expression(FILE *f, struct expression *e)
{
    fprintf(f, "(");
    switch (e->type)
    {
        case EXPR_LITERAL_INT:
            fprintf(f, "%lld", e->idata);
            break;
        case EXPR_LITERAL_STRING:
            fprintf(f, "\"%s\"", (char *)e->pdata);
            break;
        case EXPR_LITERAL_ARRAY:
            fprintf(f, "<type at %p>:", e->pdata); print_expression(f, e->childs[0]);
            break;
        case EXPR_ARRAY_INDEX:
            print_expression(f, e->childs[0]); fprintf(f, "["); print_expression(f, e->childs[1]); fprintf(f, "]");
            break;
        case EXPR_VARIABLE:
            fprintf(f, "%s", ((struct variable *)e->pdata)->name);
            break;
        case EXPR_QUERY:
            fprintf(f, "?"); print_expression(f, e->childs[0]);
            break;
        case EXPR_PUSH:
            print_expression(f, e->childs[0]); fprintf(f, " <- "); print_expression(f, e->childs[1]);
            break;
        case EXPR_OP:
            if (e->childs[0] != NULL){ print_expression(f, e->childs[0]);}
            print_op(f, (enum op_type)e->idata);
            if (e->childs[1] != NULL){ print_expression(f, e->childs[1]);}
            break;
    }
    fprintf(f, ")");
}

static void print_code_indented(FILE *f, struct block *b, int indent)
{
    #define P {for (int i = 0; i < indent; ++i) putc(' ', f);}
    P fprintf(f, "Block %p ", b); fprintf(f, "[parent %p]\n", b->parent);
    for (int i = 0; i < b->locals_len; ++i)
    {
        P fprintf(f, "local %2d: ", i); print_variable(f, b->locals[i]);
        fprintf(f, "\n");
    }
    P fprintf(f, "...\n");
    for (int i = 0; i < b->code_len; ++i)
    {
        switch (b->code[i]->type)
        {
            case STMT_BLOCK:
                P fprintf(f, "Block:\n");
                print_code_indented(f, b->code[i]->block, indent + 4);
                break;
            case STMT_LOOP:
                P fprintf(f, "Loop: guard: "); print_expression(f, b->code[i]->loop.expr); fprintf(f, "\n");
                P fprintf(f, "Block:\n");
                print_code_indented(f, b->code[i]->loop.block, indent + 4);
                break;
            case STMT_MATCH:
                P fprintf(f, "Match: guard: "); print_expression(f, b->code[i]->match.expr); fprintf(f, "\n");
                P fprintf(f, "cases:\n");
                for (int j = 0; j < b->code[i]->match.cases_len; ++j)
                {
                    P 
                    fprintf(f, "case <"); 
                    if (b->code[i]->match.cases_default[j] != NULL)
                    {
                        fprintf(f, "default:");
                        print_variable(f, b->code[i]->match.cases_default[j]);
                    }
                    else
                    {
                        print_expression(f, b->code[i]->match.cases_literal[j]);
                    }
                    if (b->code[i]->match.cases_guard[j] != NULL)
                    {
                        fprintf(f, "> where <"); 
                        {
                            print_expression(f, b->code[i]->match.cases_guard[j]);
                        }
                    }
                    fprintf(f, "> then\n"); 
                    print_code_indented(f, b->code[i]->match.cases_block[j], indent + 4);
                }
                break;
            case STMT_EXPRESSION:
                P fprintf(f, "Expr:"); print_expression(f, b->code[i]->expr); fprintf(f, "\n");
                break;
            case STMT_WORKER:
                P fprintf(f, "Call: %s\n", b->code[i]->worker.worker->name);
                break;
            case STMT_BREAK:
                P fprintf(f, "Break.\n");
                break;
        }
    }
    #undef P
}

static void print_code(FILE *f, struct block *b)
{
    print_code_indented(f, b, 4);
}

static void print_worker(FILE *f, struct worker *w)
{
    fprintf(f, "Worker %s\n", w->name);
    for (int i = 0; i < w->inputs_len; ++i)
    {
        fprintf(f, "Input  %2d: ", i);
        print_variable(f, w->inputs[i]);
        fprintf(f, "\n");
    }
    for (int i = 0; i < w->outputs_len; ++i)
    {
        fprintf(f, "Output %2d: ", i);
        print_variable(f, w->outputs[i]);
        fprintf(f, "\n");
    }
    fprintf(f, "\n");
    print_code(f, w->body);
    fprintf(f, "\n");
    fprintf(f, "\n");
}

int export_program_text(struct program *program, const char *filename)
{
    FILE *f = fopen(filename, "w");
    for (int i = 0; i < program->workers_len; ++i)
    {
        print_worker(f, program->workers[i]);
    }
    fclose(f);
    return 0;
}

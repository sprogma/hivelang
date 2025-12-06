#include "ast.h"
#include "export.h"

#include "stdio.h"
#include "stdlib.h"


static void print_type(FILE *f, struct type *t)
{
    fprintf(f, "\"%lld\":{", t->id);
    fprintf(f, "\"type\":%d", t->type);
    switch (t->type)
    {
        case VAR_SCALAR:
            fprintf(f, ",");
            fprintf(f, "\"scalar\":\"%s\",", t->scalar.name);
            fprintf(f, "\"size\":%lld", size_of_type(t));
            break;
        case VAR_CLASS:
            fprintf(f, ",");
            fprintf(f, "\"class:\"%s\",", t->class.name);
            fprintf(f, "\"fields\":{");
            for (int i = 0; i < t->class.record->record.fields_len; ++i)
            {
                if (i != 0) { putc(',', f); }
                fprintf(f, "\"%s\":%lld", t->class.record->record.fields_name[i], 
                                          t->class.record->record.fields_type[i]->id);
            }
            fprintf(f, "}");
            break;
        case VAR_RECORD:
            fprintf(f, ",");
            fprintf(f, "\"record\":%s,", t->record.name);
            fprintf(f, "\"fields\":{");
            for (int i = 0; i < t->record.fields_len; ++i)
            {
                if (i != 0) { putc(',', f); }
                fprintf(f, "\"%s\":%lld", t->record.fields_name[i], 
                                          t->record.fields_type[i]->id);
            }
            fprintf(f, "}");
            break;
        case VAR_ARRAY:
            fprintf(f, ",");
            fprintf(f, "\"base\":%lld", t->array.base->id);
            break;
        case VAR_PIPE:
            fprintf(f, ",");
            fprintf(f, "\"base\":%lld", t->array.base->id);
            break;
    }
    fprintf(f, "}");
}


static void print_variable(FILE *f, struct variable *v)
{
    fprintf(f, "\"%lld\":%lld", v->id, v->type->id);
}


static void print_op(FILE *f, enum op_type t)
{
    switch (t)
    {
        case OP_ADD: fprintf(f, "\"add\""); break;
        case OP_SUB: fprintf(f, "\"sub\""); break;
        case OP_MUL: fprintf(f, "\"mul\""); break;
        case OP_DIV: fprintf(f, "\"div\""); break;
        case OP_MOD: fprintf(f, "\"mod\""); break;

        case OP_AND: fprintf(f, "\"and\""); break;
        case OP_NOT: fprintf(f, "\"not\""); break;
        case OP_OR: fprintf(f, "\"or\""); break;
        case OP_XOR: fprintf(f, "\"xor\""); break;

        case OP_BAND: fprintf(f, "\"band\""); break;
        case OP_BNOT: fprintf(f, "\"bnot\""); break;
        case OP_BOR: fprintf(f, "\"bor\""); break;
        case OP_BXOR: fprintf(f, "\"bxor\""); break;

        case OP_LT: fprintf(f, "\"lt\""); break;
        case OP_LE: fprintf(f, "\"le\""); break;
        case OP_GT: fprintf(f, "\"gt\""); break;
        case OP_GE: fprintf(f, "\"ge\""); break;
        case OP_EQ: fprintf(f, "\"eq\""); break;
        case OP_NE: fprintf(f, "\"ne\""); break;
    }
    return;
}


static void print_expression(FILE *f, struct expression *e)
{
    fprintf(f, "{");
    switch (e->type)
    {
        case EXPR_LITERAL_INT:
            fprintf(f, "\"int\":%lld", e->idata);
            break;
        case EXPR_LITERAL_STRING:
            fprintf(f, "\"string\":\"%s\"", (char *)e->pdata);
            break;
        case EXPR_LITERAL_ARRAY:
            fprintf(f, "\"array\":null,");
            fprintf(f, "\"type\":%lld,", ((struct type *)e->pdata)->id);
            fprintf(f, "\"size\":");
            print_expression(f, e->childs[0]);
            break;
        case EXPR_ARRAY_INDEX:
            fprintf(f, "\"index\":null,");
            fprintf(f, "\"from\":");
            print_expression(f, e->childs[0]);
            fprintf(f, ",");
            fprintf(f, "\"at\":");
            print_expression(f, e->childs[1]);
            break;
        case EXPR_VARIABLE:
            fprintf(f, "\"variable\":%lld", ((struct variable *)e->pdata)->id);
            break;
        case EXPR_QUERY:
            fprintf(f, "\"query\":"); print_expression(f, e->childs[0]);
            break;
        case EXPR_PUSH:
            fprintf(f, "\"push\":null,");
            fprintf(f, "\"to\":");
            print_expression(f, e->childs[0]);
            fprintf(f, ",");
            fprintf(f, "\"from\":");
            print_expression(f, e->childs[1]);
            break;
        case EXPR_OP:
            fprintf(f, "\"op\":");
            print_op(f, e->idata);
            fprintf(f, ",");
            fprintf(f, "\"childs\":[");
            if (e->childs_len == 1)
            {
                print_expression(f, e->childs[0]);
            }
            else if (e->childs_len == 2)
            {
                print_expression(f, e->childs[0]);
                fprintf(f, ",");
                print_expression(f, e->childs[1]);
            }
            fprintf(f, "]");
            break;
    }
    fprintf(f, "}");
}

static void print_code(FILE *f, struct block *b)
{
    fprintf(f, "{");
    fprintf(f, "\"locals\":{");
    for (int i = 0; i < b->locals_len; ++i)
    {
        if (i != 0) { putc(',', f); }
        print_variable(f, b->locals[i]);
    }
    fprintf(f, "},");
    fprintf(f, "\"code\":[");
    for (int i = 0; i < b->code_len; ++i)
    {
        if (i != 0) { putc(',', f); }
        switch (b->code[i]->type)
        {
            case STMT_BLOCK:
                print_code(f, b->code[i]->block);
                break;
            case STMT_LOOP:
                fprintf(f, "{");
                fprintf(f, "\"type\":\"loop\",");
                fprintf(f, "\"guard\":");
                print_expression(f, b->code[i]->loop.expr);
                fprintf(f, ",");
                fprintf(f, "\"body\":");
                print_code(f, b->code[i]->loop.block);
                fprintf(f, "}");
                break;
            case STMT_MATCH:
                fprintf(f, "{");
                fprintf(f, "\"type\":\"match\",");
                fprintf(f, "\"match\":");
                print_expression(f, b->code[i]->match.expr);
                fprintf(f, ",");
                fprintf(f, "\"cases\":[");
                for (int j = 0; j < b->code[i]->match.cases_len; ++j)
                {
                    if (j != 0) { putc(',', f); }
                    fprintf(f, "{");
                    if (b->code[i]->match.cases_default[j] != NULL)
                    {
                        fprintf(f, "\"default\":true,");
                        fprintf(f, "\"var\":%lld,", b->code[i]->match.cases_default[j]->id);
                    }
                    else
                    {
                        fprintf(f, "\"default\":false,");
                        fprintf(f, "\"case\":");
                        print_expression(f, b->code[i]->match.cases_literal[j]);
                        fprintf(f, ",");
                    }
                    if (b->code[i]->match.cases_guard[j] != NULL)
                    {
                        fprintf(f, "\"guard\":");
                        print_expression(f, b->code[i]->match.cases_guard[j]);
                        fprintf(f, ",");
                    }
                    fprintf(f, "\"body\":");
                    print_code(f, b->code[i]->match.cases_block[j]);
                    fprintf(f, "}");
                }
                fprintf(f, "]");
                fprintf(f, "}");
                break;
            case STMT_EXPRESSION:
                fprintf(f, "{");
                fprintf(f, "\"type\":\"expr\",");
                fprintf(f, "\"expr\":");
                print_expression(f, b->code[i]->expr);
                fprintf(f, "}");
                break;
            case STMT_WORKER:
                fprintf(f, "{");
                fprintf(f, "\"type\":\"call\",");
                fprintf(f, "\"worker\":%lld,", b->code[i]->worker.worker->id);
                fprintf(f, "\"inputs\":[");
                for (int j = 0; j < b->code[i]->worker.worker->inputs_len; ++j)
                {
                    if (j != 0) { putc(',', f); }
                    print_expression(f, b->code[i]->worker.inputs[j]);
                }
                fprintf(f, "],");
                fprintf(f, "\"outputs\":[");
                for (int j = 0; j < b->code[i]->worker.worker->outputs_len; ++j)
                {
                    if (j != 0) { putc(',', f); }
                    fprintf(f, "%lld", b->code[i]->worker.outputs[j]->id);
                }
                fprintf(f, "]");
                fprintf(f, "}");
                break;
            case STMT_BREAK:
                fprintf(f, "{");
                fprintf(f, "\"type\":\"break\"");
                fprintf(f, "}");
                break;
        }
    }
    fprintf(f, "]");
    fprintf(f, "}");
}

static void print_worker(FILE *f, struct worker *w)
{
    fprintf(f, "\"%lld\":{", w->id);
    fprintf(f, "\"inputs\":{");
    for (int i = 0; i < w->inputs_len; ++i)
    {
        if (i != 0) { putc(',', f); }
        print_variable(f, w->inputs[i]);
    }
    fprintf(f, "},");
    fprintf(f, "\"outputs\":{");
    for (int i = 0; i < w->outputs_len; ++i)
    {
        if (i != 0) { putc(',', f); }
        print_variable(f, w->outputs[i]);
    }
    fprintf(f, "},");
    fprintf(f, "\"code\":");
    print_code(f, w->body);
    fprintf(f, "}");
}

int export_program_json(struct program *program, const char *filename)
{
    /* export types */
    FILE *f = fopen(filename, "w");
    fprintf(f, "{");
    {
        fprintf(f, "\"types\":{");
        for (int i = 0; i < program->types_len; ++i)
        {
            if (i != 0) { putc(',', f); }
            print_type(f, program->types[i]);
        }
        fprintf(f, "}");
    }
    fprintf(f, ",");
    {
        fprintf(f, "\"workers\":{");
        for (int i = 0; i < program->workers_len; ++i)
        {
            if (i != 0) { putc(',', f); }
            print_worker(f, program->workers[i]);
        }
        fprintf(f, "}");
    }
    fprintf(f, "}");
    fclose(f);
    return 0;
}

#include "ast.h"
#include "export.h"

#include "stdio.h"
#include "string.h"
#include "stdlib.h"


// #define WRITE_64(ptr) fwrite(ptr, 1, 8, f)
#define WRITE_64(ptr) write64packed(ptr, f)
#define WRITE(ptr, count) fwrite(ptr, 1, count, f)


static void write64packed(void *ptr, FILE *f)
{
    // ONLY FOR LITTLE ENDIANS!
    uint64_t value = *(uint64_t *)ptr;
    if (value < 128)
    {
        uint8_t size = value | 0x80;
        WRITE(&size, 1);
    }
    else
    {
        uint8_t *data = ptr;
        int cnt = 0;
        while (cnt < 8 && data[7 - cnt] == 0)
        {
            cnt++;
        }
        uint8_t size = (8 - cnt) | 0x80;
        WRITE(&size, 1);
        WRITE(data, 8 - cnt);
    }
}


static void print_type(FILE *f, struct type *t)
{
    WRITE_64(&t->id);
    WRITE_64(&t->type);
    switch (t->type)
    {
        case VAR_SCALAR:
        {
            int64_t len = strlen(t->scalar.name);
            WRITE_64(&len);
            WRITE(t->scalar.name, len);
            len = size_of_type(t);
            WRITE_64(&len);
            break;
        }
        case VAR_CLASS:
        {
            int64_t len = strlen(t->class.name);
            WRITE_64(&len);
            WRITE(t->class.name, len);
            WRITE_64(&t->class.record->record.fields_len);
            for (int i = 0; i < t->class.record->record.fields_len; ++i)
            {
                WRITE_64(&t->class.record->record.fields_type[i]->id);
            }
            break;
        }
        case VAR_RECORD:
        {
            int64_t len = strlen(t->record.name);
            WRITE_64(&len);
            WRITE(t->record.name, len);
            WRITE_64(& t->record.fields_len);
            for (int i = 0; i <  t->record.fields_len; ++i)
            {
                WRITE_64(&t->record.fields_type[i]->id);
            }
            break;
        }
        case VAR_ARRAY:
            WRITE_64(&t->array.base->id);
            break;
        case VAR_PROMISE:
            WRITE_64(&t->promise.base->id);
            break;
        case VAR_PIPE:
            WRITE_64(&t->pipe.base->id);
            break;
    }
}


static void print_variable(FILE *f, struct variable *v)
{
    WRITE_64(&v->id);
    WRITE_64(&v->type->id);
}


static void print_op(FILE *f, enum op_type t)
{
    int64_t res = t;
    WRITE_64(&res);
}


static void print_expression(FILE *f, struct expression *e)
{
    WRITE_64(&e->type);
    switch (e->type)
    {
        case EXPR_LITERAL_INT:
            WRITE_64(&e->idata);
            break;
        case EXPR_LITERAL_STRING:
        {    
            int64_t len = strlen((char *)e->pdata);
            WRITE_64(&len);
            WRITE(e->pdata, len);
            break;
        }
        case EXPR_LITERAL_ARRAY:
            WRITE_64(&((struct type *)e->pdata)->id);
            print_expression(f, e->childs[0]);
            break;
        case EXPR_ARRAY_INDEX:
            print_expression(f, e->childs[0]);
            print_expression(f, e->childs[1]);
            break;
        case EXPR_VARIABLE:
            WRITE_64(&((struct variable *)e->pdata)->id);
            break;
        case EXPR_QUERY:
            print_expression(f, e->childs[0]);
            break;
        case EXPR_PUSH:
            print_expression(f, e->childs[0]);
            print_expression(f, e->childs[1]);
            break;
        case EXPR_OP:
            print_op(f, e->idata);
            if (e->childs_len == 1)
            {
                print_expression(f, e->childs[0]);
            }
            else if (e->childs_len == 2)
            {
                print_expression(f, e->childs[0]);
                print_expression(f, e->childs[1]);
            }
            break;
    }
}

static void print_code(FILE *f, struct block *b)
{
    WRITE_64(&b->locals_len);
    for (int i = 0; i < b->locals_len; ++i)
    {
        print_variable(f, b->locals[i]);
    }
    WRITE_64(&b->code_len);
    for (int i = 0; i < b->code_len; ++i)
    {
        WRITE_64(&b->code[i]->type);
        switch (b->code[i]->type)
        {
            case STMT_BLOCK:
                print_code(f, b->code[i]->block);
                break;
            case STMT_LOOP:
                print_expression(f, b->code[i]->loop.expr);
                print_code(f, b->code[i]->loop.block);
                break;
            case STMT_MATCH:
                print_expression(f, b->code[i]->match.expr);
                WRITE_64(&b->code[i]->match.cases_len);
                for (int j = 0; j < b->code[i]->match.cases_len; ++j)
                {
                    if (b->code[i]->match.cases_default[j] != NULL)
                    {
                        WRITE_64(&b->code[i]->match.cases_default[j]->id);
                    }
                    else
                    {
                        int64_t zero = 0;
                        WRITE_64(&zero);
                        print_expression(f, b->code[i]->match.cases_literal[j]);
                    }
                    if (b->code[i]->match.cases_guard[j] != NULL)
                    {
                        int64_t zero = 1;
                        WRITE_64(&zero);
                        print_expression(f, b->code[i]->match.cases_guard[j]);
                    }
                    else
                    {
                        int64_t zero = 0;
                        WRITE_64(&zero);
                    }
                    print_code(f, b->code[i]->match.cases_block[j]);
                }
                break;
            case STMT_EXPRESSION:
                print_expression(f, b->code[i]->expr);
                break;
            case STMT_WORKER:
                WRITE_64(&b->code[i]->worker.worker->id);
                WRITE_64(&b->code[i]->worker.worker->inputs_len);
                for (int j = 0; j < b->code[i]->worker.worker->inputs_len; ++j)
                {
                    print_expression(f, b->code[i]->worker.inputs[j]);
                }
                WRITE_64(&b->code[i]->worker.worker->outputs_len);
                for (int j = 0; j < b->code[i]->worker.worker->outputs_len; ++j)
                {
                    WRITE_64(&b->code[i]->worker.outputs[j]->id);
                }
                break;
            case STMT_BREAK:
                break;
        }
    }
}

static void print_worker(FILE *f, struct worker *w)
{
    WRITE_64(&w->id);
    WRITE_64(&w->inputs_len);
    for (int i = 0; i < w->inputs_len; ++i)
    {
        print_variable(f, w->inputs[i]);
    }
    WRITE_64(&w->outputs_len);
    for (int i = 0; i < w->outputs_len; ++i)
    {
        print_variable(f, w->outputs[i]);
    }
    print_code(f, w->body);
}

int export_program_bytes(struct program *program, const char *filename)
{
    /* export types */
    FILE *f = fopen(filename, "wb");
    
    WRITE_64(&program->types_len);
    for (int i = 0; i < program->types_len; ++i)
    {
        print_type(f, program->types[i]);
    }
    WRITE_64(&program->workers_len);
    for (int i = 0; i < program->workers_len; ++i)
    {
        print_worker(f, program->workers[i]);
    }
    
    int64_t size = ftell(f);
    printf("BYTE export generated [to file %s]. wrote %lld bytes\n", filename, size);
    fclose(f);
    return 0;
}

#include "ast.h"
#include "math.h"

#include "stdio.h"
#include "string.h"
#include "stdlib.h"


double expr_cost(struct program *p, struct expression *e)
{
    switch (e->type) {
        case EXPR_LITERAL_INT: return 0.25;
        case EXPR_LITERAL_STRING: return 1.0 + strlen(e->pdata) * 0.125;
        case EXPR_LITERAL_ARRAY: return 100.0 + size_of_type(e->pdata) * e->idata * 0.0125;
        case EXPR_ARRAY_INDEX: return 1.0 + expr_cost(p, e->childs[0]) + expr_cost(p, e->childs[1]);
        case EXPR_VARIABLE: return 1.0;
        case EXPR_QUERY: return (get_expr_type(p, e->childs[0])->type == VAR_ARRAY ? 1.0 : 500.0);
        case EXPR_PUSH: return 4.0;
        case EXPR_OP:
            switch(e->idata) {    
                case OP_ADD: return 0.5 + expr_cost(p, e->childs[0]) + expr_cost(p, e->childs[1]);
                case OP_SUB: return 0.5 + expr_cost(p, e->childs[0]) + expr_cost(p, e->childs[1]);
                case OP_MUL: return 1.5 + expr_cost(p, e->childs[0]) + expr_cost(p, e->childs[1]);
                case OP_DIV: return 10.0 + expr_cost(p, e->childs[0]) + expr_cost(p, e->childs[1]);
                case OP_MOD: return 10.0 + expr_cost(p, e->childs[0]) + expr_cost(p, e->childs[1]);
                case OP_AND: return 2.5 + expr_cost(p, e->childs[0]) + expr_cost(p, e->childs[1]);
                case OP_NOT: return 2.5 + expr_cost(p, e->childs[0]);
                case OP_OR: return 2.5 + expr_cost(p, e->childs[0]) + expr_cost(p, e->childs[1]);
                case OP_XOR: return 2.5 + expr_cost(p, e->childs[0]) + expr_cost(p, e->childs[1]);
                case OP_BAND: return 0.5 + expr_cost(p, e->childs[0]) + expr_cost(p, e->childs[1]);
                case OP_BNOT: return 0.5 + expr_cost(p, e->childs[0]);
                case OP_BOR: return 0.5 + expr_cost(p, e->childs[0]) + expr_cost(p, e->childs[1]);
                case OP_BXOR: return 0.5 + expr_cost(p, e->childs[0]) + expr_cost(p, e->childs[1]);
                case OP_LT: return 1.0 + expr_cost(p, e->childs[0]) + expr_cost(p, e->childs[1]);
                case OP_LE: return 1.0 + expr_cost(p, e->childs[0]) + expr_cost(p, e->childs[1]);
                case OP_GT: return 1.0 + expr_cost(p, e->childs[0]) + expr_cost(p, e->childs[1]);
                case OP_GE: return 1.0 + expr_cost(p, e->childs[0]) + expr_cost(p, e->childs[1]);
                case OP_EQ: return 1.0 + expr_cost(p, e->childs[0]) + expr_cost(p, e->childs[1]);
                case OP_NE: return 1.0 + expr_cost(p, e->childs[0]) + expr_cost(p, e->childs[1]);
            }
    }
    return 0.0;
}

double execution_cost(struct program *p, struct block *w)
{
    double res = 0.0;
    for (int i = 0; i < w->code_len; ++i)
    {
        switch (w->code[i]->type)
        {
            case STMT_BLOCK:
                w->code[i]->execution_cost = execution_cost(p, w->code[i]->block);
                break;
            case STMT_LOOP:
                /* think loop will be executed 100 times */
                w->code[i]->execution_cost = 100.0 * (
                    execution_cost(p, w->code[i]->loop.block) + 
                    expr_cost(p, w->code[i]->loop.expr)
                );
                break;
            case STMT_MATCH:
                {
                    double avr = 0.0;
                    for (int j = 0; j < w->code[i]->match.cases_len; ++j)
                    {
                        if (w->code[i]->match.cases_literal[j] != NULL)
                        {
                            avr += 2.0 * expr_cost(p, w->code[i]->match.cases_literal[j]);
                        }
                        if (w->code[i]->match.cases_guard[j] != NULL)
                        {
                            avr += 2.0 * expr_cost(p, w->code[i]->match.cases_guard[j]);
                        }
                        avr += execution_cost(p, w->code[i]->match.cases_block[j]);
                    }
                    avr /= w->code[i]->match.cases_len;
                    /* think match have little overhead + may be many branches will be checked */
                    w->code[i]->execution_cost = 2.5 + avr;
                }
                break;
            case STMT_EXPRESSION:
                w->code[i]->execution_cost = expr_cost(p, w->code[i]->expr);
                break;
            case STMT_WORKER:
                w->code[i]->execution_cost = 10.0 * log(1.0 + w->code[i]->worker.worker->execution_cost);
                break;
            case STMT_BREAK:
                w->code[i]->execution_cost = 1.0;
                break;
        }
        res += w->code[i]->execution_cost;
    }
    return res;
}


void optimize_program(struct program *program)
{
    for (int i = 0; i < program->workers_len; ++i)
    {
        program->workers[i]->multiplication = 0;
    }
    
    /* calculate execution costs of all workers */
    /* 100 rounds of calculations */
    for (int t = 0; t < 100; ++t)
    {
        for (int i = 0; i < program->workers_len; ++i)
        {
            program->workers[i]->execution_cost = execution_cost(program, program->workers[i]->body);
        }
    }

    // TODO: futher optimizations
}

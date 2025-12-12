#include "stdio.h"

#include "runtime.hpp"
#include "code.hpp"
#include "dynamic.hpp"
#include "context.hpp"
#include "type.hpp"


struct hive *create_hive(struct program *prg)
{
    struct hive *h = new struct hive; /*(struct hive *)calloc(1, sizeof(*h));

    new(&h->objects)std::unordered_map<object_id, struct object_origin>;
    new(&h->workers)std::unordered_map<worker_id, struct execution_worker *>;
    */
    
    h->program = prg;
    
    return h;
}


void allocate_memory_for_locals(struct context *ctx, struct block *code)
{    
    for (int i = 0; i < code->locals_len; ++i)
    {
        printf("REG LOCAL %lld += %lld\n", code->locals[i].id, ctx->next_address);
        ctx->offsets[code->locals[i].id] = ctx->next_address;
        ctx->locals[code->locals[i].id] = &code->locals[i];
        ctx->next_address += ctx->h->program->types[code->locals[i].type]->cached_size;
    }
}


char *generate_expression(char *buf, struct context *ctx, struct expression *e)
{
    switch (e->type)
    {
        case EXPR_LITERAL_INT:
            buf += sprintf(buf, "rax[0] = %lld;", e->idata);
            break;
        case EXPR_LITERAL_STRING:
            buf += sprintf(buf, "rax[0] = (long long)strdup(\"%s\");", (char *)e->pdata);
            break;
        case EXPR_LITERAL_ARRAY:
            buf = generate_expression(buf, ctx, e->childs[0]);
            buf += sprintf(buf, "rax[0] = (long long)malloc(%lld * rax[0]);\n", ctx->h->program->types[e->idata]->cached_size);
            break;
        case EXPR_ARRAY_INDEX:
        {
            buf = generate_expression(buf, ctx, e->childs[0]);
            buf += sprintf(buf, "push(rax);");
            buf = generate_expression(buf, ctx, e->childs[1]);
            buf += sprintf(buf, "pop(rbx);");
            // struct type *t0 = ctx->h->program->types[get_expr_type(ctx, e->childs[0])];
            // struct type *t1 = ctx->h->program->types[get_expr_type(ctx, e->childs[1])];
            // size_t size = get_type_size(ctx, t0->id);
            buf += sprintf(buf, "QueryObject(rbx[0], rax[0], rax);"); /* last arg - destination */
            break;
        }
        case EXPR_VARIABLE:
        {
            size_t offset = ctx->offsets[e->idata];
            size_t size = get_type_size(ctx, ctx->locals[e->idata]->type);
            buf += sprintf(buf, "memcpy(rax, (char *)locals + %lld, %lld);", offset, size);
            break;
        }
        case EXPR_QUERY:
        {
            buf = generate_expression(buf, ctx, e->childs[0]);
            struct type *t0 = ctx->h->program->types[get_expr_type(ctx, e->childs[0])];
            switch (t0->type)
            {
                case VAR_PIPE:
                    buf += sprintf(buf, "QueryObject(rax[0], 0, rax);");
                    break;
                case VAR_ARRAY:
                    buf += sprintf(buf, "QueryObject(rax[0], 0, rax);");
                    break;
                case VAR_PROMISE:
                    buf += sprintf(buf, "QueryObject(rax[0], 0, rax);");
                    break;
                default:
                    printf("Erro!\n");
                    exit(1);
            }
            break;
        }
        case EXPR_PUSH:
        {
            
            struct type *t0 = ctx->h->program->types[get_expr_type(ctx, e->childs[0])];
            
            switch (t0->type)
            {
                case VAR_CLASS:
                case VAR_RECORD:
                case VAR_SCALAR:
                case VAR_ARRAY:
                {
                    if (e->childs[0]->type == EXPR_VARIABLE)
                    {
                        buf = generate_expression(buf, ctx, e->childs[1]);
                        size_t size = get_type_size(ctx, t0->id);
                        size_t offset = ctx->offsets[(var_id)e->childs[0]->idata];
                        buf += sprintf(buf, "memcpy((char *)locals + %lld, rax, %lld);", offset, size);
                    }
                    else if (e->childs[0]->type == EXPR_ARRAY_INDEX)
                    {
                        buf = generate_expression(buf, ctx, e->childs[1]);
                        buf += sprintf(buf, "push(rax);");
                        /* load array address */
                        buf = generate_expression(buf, ctx, e->childs[0]->childs[0]);
                        buf += sprintf(buf, "push(rax);");
                        /* load index */
                        buf = generate_expression(buf, ctx, e->childs[0]->childs[1]);
                        buf += sprintf(buf, "pop(rbx);");
                        buf += sprintf(buf, "pop(rcx);");
                        buf += sprintf(buf, "SendObject(rbx[0], rax[0], rcx);");
                    }
                    else
                    {
                        printf("Error [not supported] push to %d\n", e->childs[0]->type);
                        exit(1);
                    }
                    break;
                }
                case VAR_PIPE:
                case VAR_PROMISE:
                {
                    buf = generate_expression(buf, ctx, e->childs[0]);
                    buf += sprintf(buf, "push(rax);");
                    buf = generate_expression(buf, ctx, e->childs[1]);
                    buf += sprintf(buf, "pop(rbx);");
                    buf += sprintf(buf, "SendObject(rbx[0], 0, rax);");
                    break;
                }
                default:
                    printf("ERROR!\n");
                    exit(1);
            }
            // buf += sprintf(buf, "\"push\":null,");
            // buf += sprintf(buf, "\"to\":");
            // buf = generate_expression(buf, ctx, e->childs[0]);
            // buf += sprintf(buf, ",");
            // buf += sprintf(buf, "\"from\":");
            // buf = generate_expression(buf, ctx, e->childs[1]);
            break;
        }
        case EXPR_OP:
        {
            if (e->childs_len == 1)
            {
                buf = generate_expression(buf, ctx, e->childs[0]);
            }
            else if (e->childs_len == 2)
            {
                buf = generate_expression(buf, ctx, e->childs[0]);
                buf += sprintf(buf, "push(rax);");
                buf = generate_expression(buf, ctx, e->childs[1]);
                buf += sprintf(buf, "pop(rbx);");
            }

            if (type_is_integer(ctx, get_expr_type(ctx, e->childs[0])))
            {
                switch ((enum op_type)e->idata)
                {
                    case OP_ADD: buf += sprintf(buf, "rax[0] = rbx[0] + rax[0];"); break;
                    case OP_SUB: buf += sprintf(buf, "rax[0] = rbx[0] - rax[0];"); break;
                    case OP_MUL: buf += sprintf(buf, "rax[0] = rbx[0] * rax[0];"); break;
                    case OP_DIV: buf += sprintf(buf, "rax[0] = rbx[0] / rax[0];"); break;
                    case OP_MOD: buf += sprintf(buf, "rax[0] = rbx[0] %% rax[0];"); break;

                    case OP_NOT: buf += sprintf(buf, "rax[0] = !rax[0];"); break;
                    case OP_BNOT: buf += sprintf(buf, "rax[0] = ~rax[0];"); break;
                    
                    case OP_AND: buf += sprintf(buf, "rax[0] = rbx[0] && rax[0];"); break; // TODO: right computation
                    case OP_OR: buf += sprintf(buf, "rax[0] = rbx[0] || rax[0];"); break; // TODO: right computation
                    case OP_XOR: buf += sprintf(buf, "rax[0] = rbx[0] ^^ rax[0];"); break;

                    case OP_BAND: buf += sprintf(buf, "rax[0] = rbx[0] & rax[0];"); break;
                    case OP_BOR: buf += sprintf(buf, "rax[0] = rbx[0] | rax[0];"); break;
                    case OP_BXOR: buf += sprintf(buf, "rax[0] = rbx[0] ^ rax[0];"); break;

                    case OP_LT: buf += sprintf(buf, "rax[0] = (rbx[0] < rax[0]);"); break;
                    case OP_LE: buf += sprintf(buf, "rax[0] = (rbx[0] <= rax[0]);"); break;
                    case OP_GT: buf += sprintf(buf, "rax[0] = (rbx[0] > rax[0]);"); break;
                    case OP_GE: buf += sprintf(buf, "rax[0] = (rbx[0] > rax[0]);"); break;
                    case OP_EQ: buf += sprintf(buf, "rax[0] = (rbx[0] == rax[0]);"); break;
                    case OP_NE: buf += sprintf(buf, "rax[0] = (rbx[0] != rax[0]);"); break;
                }
            }
            else
            {
                size_t size = get_type_size(ctx, get_expr_type(ctx, e->childs[0]));
                switch ((enum op_type)e->idata)
                {
                    case OP_EQ:
                        buf += sprintf(buf, "rax[0] = memcmp(rax, rbx, %lld) == 0;", size);
                        break;
                    case OP_NE:
                        buf += sprintf(buf, "rax[0] = memcmp(rax, rbx, %lld) != 0;", size);
                        break;
                    default:
                        printf("Error +-*/ on not supported type\n");
                        exit(1);
                }
            }
            break;
        }
    }
    return buf;
}



char *generate_code(char *buf, struct context *ctx, struct block *code)
{
    (void)ctx;

    for (int i = 0; i < code->code_len; ++i)
    {
        switch (code->code[i].type)
        {
            case STMT_BLOCK:
            {
                buf += sprintf(buf, "{");
                allocate_memory_for_locals(ctx, code->code[i].block);
                buf = generate_code(buf, ctx, code->code[i].block);
                buf += sprintf(buf, "}");
                break;
            }
            case STMT_LOOP:
            {
                buf += sprintf(buf, "while (1){");
                buf = generate_expression(buf, ctx, code->code[i].loop.expr);
                buf += sprintf(buf, "if (rax[0] == 0) { break; }");
                allocate_memory_for_locals(ctx, code->code[i].loop.block);
                buf = generate_code(buf, ctx, code->code[i].loop.block);
                buf += sprintf(buf, "}");
                break;
            }
            case STMT_MATCH:
                break;
            case STMT_EXPRESSION:
                buf = generate_expression(buf, ctx, code->code[i].expr);
            case STMT_WORKER:
                break;
            case STMT_BREAK:
                break;
        }
    }
    return buf;
}


void compile_worker(struct hive *h, worker_id wk)
{
    struct context ctx;

    ctx.ew = h->workers[wk] = (struct execution_worker *)calloc(1, sizeof(*h->workers[wk]));
    ctx.w = h->program->workers[wk];
    ctx.h = h;


    ctx.ew->time_executed = 0.0;
    ctx.ew->time_executed_prev = 0.0;
    new(&ctx.ew->calls)std::unordered_map<server_id, int64_t>;

    ctx.next_address = 0;

    /* for now, */
    /* gen C code, after that compile it, and use as worker */

    char *text = (char *)malloc(1024 * 64);
    char *end = text;

    end += sprintf(end, "long long stack[256][128] = {1};");
    end += sprintf(end, "int stack_ptr = 1;");
    end += sprintf(end, "void ___chkstk_ms(void) { }");
    end += sprintf(end, "void memset(void *a, int t, long long size) { ((void (*)(void *, int, long long))0x%p)(a, t, size); }", &memset);
    end += sprintf(end, "void memcpy(void *a, void *b, long long size) { ((void (*)(void *, void *, long long))0x%p)(a, b, size); }", &memcpy);
    end += sprintf(end, "void *malloc(long long size) { ((void *(*)(long long))0x%p)(size); }", &malloc);
    end += sprintf(end, "void pop(void *b) { memcpy(b, stack[--stack_ptr], sizeof(*stack)); }");
    end += sprintf(end, "void push(void *b) { memcpy(stack[stack_ptr++], b, sizeof(*stack)); }");
    end += sprintf(end, "void QueryObject(long long a, long long param, void *b) { ((void (*)(long long, long long, void *))0x%p)(a, param, b); }", &ExpandObject);
    end += sprintf(end, "void SendObject(long long a, long long param, void *b) { ((void (*)(long long, long long, void *))0x%p)(a, param, b); }", &SendObject);
    end += sprintf(end, "void worker(unsigned char *input){");
    end += sprintf(end, "char locals[1024] = {};");
    end += sprintf(end, "long long rax[128] = {};");
    end += sprintf(end, "long long rbx[128] = {};");
    end += sprintf(end, "long long rcx[128] = {};");
    /* distribute memory between locals of 1st level */
    allocate_memory_for_locals(&ctx, ctx.w->body);
    /* parse input into variables */
    {
        for (int i = 0; i < ctx.w->inputs_len; ++i)
        {
            var_id x = ctx.w->inputs[i];
            struct variable *var = NULL;
            for (int j = 0; j < ctx.w->body->locals_len; ++j)
            {
                if (ctx.w->body->locals[j].id == x)
                {
                    var = &ctx.w->body->locals[j];
                }
            }
            if (var != NULL)
            {
                /* copy arg to locals */
                int64_t size = ctx.h->program->types[var->type]->cached_size;
                int64_t offset = ctx.offsets[var->id];
                end += sprintf(end, "memcpy(locals + %lld, input + %lld, %lld);", ctx.offsets[x], offset, size);
            }
            else
            {
                printf("Error: cant find local variable %lld\n", x);
                ctx.w->compiled = NULL;
                return;
            }
        }
    }
    end = generate_code(end, &ctx, ctx.w->body);
    end += sprintf(end, "}\n");

    /* build */
    FILE *f = fopen("tmp/a.c", "w");
    if (f == NULL)
    {
        printf("Can't open tmp/a.c for write\n");
        ctx.w->compiled = NULL;
        return;
    }
    fwrite(text, 1, end - text, f);
    fclose(f);

    // system("clang-format tmp\\a.c");
    system("gcc -fPIC -ftree-loop-distribute-patterns -fno-builtin -ffreestanding -nostdlib -nostartfiles -nodefaultlibs -o tmp\\wk.exe tmp\\a.c -Wno-builtin-declaration-mismatch");
    system("objcopy -O binary tmp\\res.exe code.bin");
    system("pwsh -c \"[void]((objdump -t .\\tmp\\wk.exe | sls worker) -match \'(\\w+)\\s+\\w+\\s*$\'); $Matches[1]\" >tmp\\res");
    FILE *resf = fopen("tmp\\res", "r");
    void *start;
    fscanf(resf, "0x%p", &start);
    fclose(resf);
    
    uint8_t *bytes = (uint8_t *)malloc(16 * 1024);
    f = fopen("tmp/a.bin", "rb");
    if (f == NULL)
    {
        printf("Compilation failed [no result file found]\n");
        ctx.w->compiled = NULL;
        return;
    }
    int64_t len = fread(bytes, 1, 16 * 1024, f);
    fclose(f);
    
    ctx.w->compiled = (void (*)(uint8_t *))(bytes + (long long)start);
    ctx.w->compiled_len = len;
}

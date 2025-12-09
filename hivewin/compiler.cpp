#include "stdio.h"

#include "code.hpp"
#include "dynamic.hpp"


struct hive *create_hive(struct program *prg)
{
    struct hive *h = (struct hive *)calloc(1, sizeof(*h));

    h->program = prg;
    
    new(&h->objects)std::unordered_map<object_id, struct object_origin>;
    new(&h->workers)std::unordered_map<worker_id, struct execution_worker *>;
    
    return h;
}


char *compile_code(char *buf, struct hive *h, struct worker *w, struct block *code)
{
    (void)h;
    (void)w;
    (void)code;
    return buf;
}


void compile_worker(struct hive *h, worker_id wk)
{
    struct execution_worker *ew = h->workers[wk] = (struct execution_worker *)calloc(1, sizeof(*h->workers[wk]));
    struct worker *w = h->program->workers[wk] = (struct worker *)calloc(1, sizeof(*h->program->workers[wk]));

    ew->time_executed = 0.0;
    ew->time_executed_prev = 0.0;
    new(&ew->calls)std::unordered_map<server_id, int64_t>;

    /* for now, */
    /* gen C code, after that compile it, and use as worker */

    char *text = (char *)malloc(1024 * 64);
    char *end = compile_code(text, h, w, w->body);

    /* build */
    FILE *f = fopen("tmp/a.c", "w");
    fwrite(text, 1, end - text, f);
    fclose(f);
    
    uint8_t *bytes = (uint8_t *)malloc(16 * 1024);
    f = fopen("tmp/a.bin", "rb");
    int64_t len = fread(bytes, 1, 16 * 1024, f);
    fclose(f);
    
    w->compiled = (void (*)(uint8_t *))bytes;
    w->compiled_len = len;
}

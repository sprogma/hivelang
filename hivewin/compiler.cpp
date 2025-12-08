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


void compile_worker(struct hive *h, worker_id wk)
{
    struct execution_worker *ew = h->workers[wk] = (struct execution_worker *)calloc(1, sizeof(*h->workers[wk]));
    struct worker *w = h->program->workers[wk] = (struct worker *)calloc(1, sizeof(*h->program->workers[wk]));

    ew->time_executed = 0.0;
    ew->time_executed_prev = 0.0;
    new(&ew->calls)std::unordered_map<server_id, int64_t>;

    /* 1. gen C code, after that compile it, and use as worker */
    
    uint8_t *bytes = (uint8_t *)malloc(1024);
    w->compiled = (void (*)(uint8_t *))bytes;
}

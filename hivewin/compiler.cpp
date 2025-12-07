#include "stdio.h"

#include "code.hpp"
#include "dynamic.hpp"


struct hive *create_hive(struct program *prg)
{
    struct hive *h = (struct hive *)calloc(1, sizeof(*h));

    h->program = prg;
    
    new(&h->objects)std::unordered_map<int64_t, struct object_origin>;
    new(&h->workers)std::unordered_map<int64_t, struct execution_worker *>;
    
    return h;
}


void compile_worker(struct hive *h, worker_id wk)
{
    struct execution_worker *w = h->workers[wk] = (struct execution_worker *)malloc(sizeof(*h->workers[wk]));

    w->time_executed = 0.0;
    w->time_executed_prev = 0.0;
    new(&w->calls)std::unordered_map<server_id, int64_t>;

    /* 1. gen C code, after that compile it, and use as worker */
    

    uint8_t *bytes;
}

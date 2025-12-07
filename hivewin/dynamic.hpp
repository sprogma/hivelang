#ifndef DYNAMIC_H
#define DYNAMIC_H

#include "inttypes.h"
#include "code.hpp"
#include <unordered_map>

typedef int64_t server_id;
typedef int64_t object_id;

union value
{
    object_id object;
    void *raw_data;
};

struct object_origin
{
    /* server to request object from */
    /* if equal to localhost - then ID without first bit is pointer on object! */
    uint32_t server_ip;
    uint32_t server_port;
};

struct execution_worker
{
    worker_id id;    
    /* statistics */
    double time_executed;
    double time_executed_prev;
    std::unordered_map<server_id, int64_t> calls;
};

struct hive
{
    std::unordered_map<int64_t, struct object_origin> objects;
    std::unordered_map<int64_t, struct execution_worker *>workers;
    struct program *program;
};


struct hive *create_hive(struct program *prg);
void compile_worker(struct hive *h, worker_id wk);


#endif

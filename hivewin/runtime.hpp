#ifndef RUNTIME_H
#define RUNTIME_H


#include "inttypes.h"

#include "dynamic.hpp"


extern "C"
{

void RunWorker(int64_t id, union value *input);

void TransferObject(object_id object, void *data);

union value GetObject(object_id object, int64_t parameter);

struct query_results
{
    int64_t workers_count;
    double waiting_time; /* total time workers waited */
    double execution_time; /* time workers executed */
    double total_time; /* time workers executed */
};

struct query_results QueryHive();

// TODO: realizate connection
// void Connect(int32_t ip, int32_t port);
}

#endif

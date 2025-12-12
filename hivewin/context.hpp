#ifndef CONTEXT_H
#define CONTEXT_H


#include <unordered_map>


struct context
{
    std::unordered_map<int64_t, int64_t> offsets;
    std::unordered_map<int64_t, struct variable *> locals;
    int64_t next_address;
    struct hive *h;
    struct execution_worker *ew;
    struct worker *w;
};


#endif

#ifndef LANG_H
#define LANG_H

#include "inttypes.h"

enum type_flags
{
    TYPE_POINTER=0x0001,
};

struct type
{
    int64_t flags;
    int64_t size;
};

enum worker_flags
{
    TYPE_POINTER=0x0001,
};

struct 
{
    int64_t flags;
    struct pipe *inputs;
    struct pipe *outputs;
} worker;

#endif

#include "stdio.h"

#include "code.hpp"
#include "dynamic.hpp"
#include "runtime.hpp"

#include <map>
#include <vector>
#include <unordered_map>


extern "C"
{

struct pipe
{
    
};

union value *create_pipe()
{
    return calloc(1, sizeof(struct pipe));
}

struct hive *h;


void RunWorker(int64_t id, union value *input)
{
    
}

void TransferObject(object_id object, void *data);

union value GetObject(object_id object, int64_t parameter);

struct query_results QueryHive();

}


int main()
{
    uint8_t *code = (uint8_t *)malloc(1024 * 64);
    FILE *f = fopen("a.bin", "r");
    code[fread(code, 1, 1024 * 64 - 1, f)] = 0;
    fclose(f);
    
    struct program *prg = program_from_bytes(code);
    h = create_hive(prg);
    
    int last;

    for (auto i : prg->workers)
    {
        compile_worker(h, i.first);
        last = i.first;
    }

    union value *input = create_pipe();

    RunWorker(last, input);

    printf("Program %p\n", prg);
    
    return 0;
}

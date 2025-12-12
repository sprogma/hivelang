#include "stdio.h"
#include "windows.h"

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
    union value *buffer;
    int64_t begin;
    int64_t end;
};



struct hive *h;




object_id register_object(void *data)
{
    object_id id = h->next_obj_id++;
    h->objects[id].server_ip = 0;
    h->objects[id].server_port = 0;
    h->objects[id].data = data;
    return id;
}


union value create_pipe()
{
    return (union value){.object = register_object(calloc(1, sizeof(struct pipe)))};
}



void RunWorker(worker_id id, uint8_t *data)
{
    struct execution_worker *ew = h->workers[id];
    struct worker *w = h->program->workers[id];

    LARGE_INTEGER frequency;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    
    printf("Starting execution of %lld\n", id);

    if (w->compiled == NULL)
    {
        printf("Error: run of not compiled worker\n");
        exit(1);
    }

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);

    w->compiled(data);

    QueryPerformanceCounter(&end);

    double time = (double) (end.QuadPart - start.QuadPart) / frequency.QuadPart;
    ew->time_executed += time;
    printf("Worker %lld executed for %f s.\n", id, time);
}

void TransferObject(object_id object, void *data)
{
    (void)object;
    (void)data;
}

void ExpandObject(object_id object, int64_t parameter, void *dest)
{
    printf("Expand obj\n");
    (void)object;
    (void)parameter;
    (void)dest;
}

struct query_results QueryHive()
{
    return {};
}


void SendObject(object_id object, int64_t parameter, void *data)
{
    printf("Send obj\n");
    (void)object;
    (void)parameter;
    (void)data;
}

}


int main()
{
    uint8_t *code = (uint8_t *)malloc(1024 * 64);
    FILE *f = fopen("a.bin", "r");
    code[fread(code, 1, 1024 * 64 - 1, f)] = 0;
    fclose(f);
    
    struct program *prg = program_from_bytes(code);
    h = create_hive(prg);
    

    worker_id last;
    for (auto i : prg->workers)
    {
        compile_worker(h, i.first);
        last = i.first;
    }

    union value input = create_pipe();

    uint8_t *data = (uint8_t *)malloc(8);
    memcpy(data, &input, 8);

    RunWorker(last, data);

    printf("Program %p\n", prg);
    
    return 0;
}

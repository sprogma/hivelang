#include "stdio.h"
#include "stdlib.h"
#include "ast.h"
#include "export.h"

int main()
{
    FILE *f = fopen("a.test", "r");
    char *s = malloc(1024 * 16);
    s[fread(s, 1, 1024 * 16, f)] = 0;
    fclose(f);

    struct program *res = parse(s);
    printf("Parsed!\n");

    if (res == NULL)
    {
        return 1;
    }

    optimize_program(res);

    export_program_text(res);
    export_program_json(res, "a.json");
    export_program_bytes(res, "a.bin");
    
    return 0;
}


#include "stdio.h"
#include "stdlib.h"
#include "ast.h"
#include "program_build.h"

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

    build_program(res);
    
    return 0;
}


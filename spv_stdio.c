#include "spv.h"

#include <stdio.h>
#include <stdlib.h>

int spv_load(const char* file, struct spv* spv)
{
    FILE* f;
    f = fopen(file, "rb");
    fseek(f, 0, SEEK_END);
    spv->sz = ftell(f);
    rewind(f);
    spv->code = malloc(spv->sz);
    fread(spv->code, 1, spv->sz, f);
    fclose(f);
    return 1;
}

void spv_free(struct spv* spv)
{
    free(spv->code); 
}

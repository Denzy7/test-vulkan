#include "spv.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#include <sys/stat.h>
#endif

int _spv_inexedir(const char* file, char* resolv, const size_t resolv_len)
{
#if __linux__ 
    struct stat statbuf;
    char* rpath;
#endif

#if __linux__
    rpath = realpath("/proc/self/exe", NULL);
    *strrchr(rpath, '/') = 0;
    snprintf(resolv, resolv_len, "%s/%s", rpath, file);
    free(rpath);
    if(stat(resolv, &statbuf) == 0)
        return 1;
#endif

    return 0;
}

int spv_load(const char* file, struct spv* spv)
{
    FILE* f;
    char resolv[1024];
    f = fopen(file, "rb");
    if(f == NULL)
    {
        if(_spv_inexedir(file, resolv, sizeof(resolv)))
            f = fopen(resolv, "rb");
        else
            return 0;
    }

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

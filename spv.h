#ifndef SPV_H
#define SPV_H

#include <stddef.h>
#include <stdint.h>
struct spv
{
    uint32_t* code;
    size_t sz;
};

int spv_load(const char* file, struct spv* spv);
void spv_free(struct spv* spv);

#endif

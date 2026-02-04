#include "mf/memory.h"

void *mfmemcpy(void *dest, const void *src, unsigned long len)
{
        return __builtin_memcpy(dest, src, len);
}

void *mfmemset(void *m, char value, unsigned long count)
{
        return __builtin_memset(m, value, count);
}

char *arena_alloc(arena *a, int size, int count, int align)
{
        int padding = (long)a->base % align;
        if (a->base + padding + (size * count) > a->end) {
                // we are super fucked here
                // fprintf(stderr, "out of memory: %s\n", a->name);
                return 0;
        }

        char *memory = a->base + padding;
        a->base += padding + (size * count);
        mfmemset(memory, 0, size * count);
        return memory;
}

char *arena_steal(arena *a, int size, int count)
{
        if (a->base + (size * count) > a->end) {
                // we are super fucked here
                return 0;
        }
        char *memory = a->base;
        a->base += (size * count);
        mfmemset(memory, 0, size * count);
        return memory;
}

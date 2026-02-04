#ifndef mf_memory
#define mf_memory

#define kibibyte (1024)
#define mebibyte (1024 * kibibyte)
#define gigabyte (1024 * mebibyte)

typedef struct arena {
        const char *name;
        char *base;
        char *end;
        int sz;
} arena;

char *arena_alloc(arena *a, int size, int count, int align);
char *arena_steal(arena *a, int size, int count);
#define make_arena(m) ((arena){__FUNCTION__, m, m + sizeof(m), sizeof(m)})
#define make_arena_ptr(ptr, sz) ((arena){__FUNCTION__, ptr, ptr + sz, sz})
#define make(a, T, c) ((T *)arena_alloc(a, sizeof(T), c, _Alignof(T)))
#define steal(a, T, c) ((T *)arena_steal(a, sizeof(T), c))
#define reserve(a, T, c)                                                       \
        ((arena){__FUNCTION__, a.base + (sizeof(T) * c), a.end, a.sz})
#define bytes_free(a) (a.end - a.base)
#define bytes_used(a) (a.sz - bytes_free(a))

void *mfmemcpy(void *dest, const void *src, unsigned long len);
void *mfmemset(void *m, char value, unsigned long count);

#endif

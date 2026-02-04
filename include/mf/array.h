#ifndef mf_array
#define mf_array

#define ARRAY_DEF(n, T) typedef struct arr_##n { \
        T *memory; \
        int len; \
} arr_##n;

typedef struct array {
        void *memory;
        int len;
} array;

#define NULL_ARRAY ((array){.len = 0, .memory = 0})
#define arr_idx(arr, T, i) (((T *)arr.memory)[i])

#endif

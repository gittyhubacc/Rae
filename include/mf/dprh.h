#ifndef mf_dprh
#define mf_dprh

#include "memory.h"
#include "string.h"

typedef struct dprh_entry {
        string key;
        void *value;
} dprh_entry;

typedef struct dprh_tbl {
        int len;
        int exp;
        dprh_entry *addr;
} dprh_tbl;

unsigned int dprh_hash(string s);
int dprh_index(unsigned int hash, int exp, int idx);

dprh_tbl dprh_tbl_make(arena *a, int exp);
int dprh_tbl_get(dprh_tbl *tbl, string key, void **value);
int dprh_tbl_set(dprh_tbl *tbl, string key, void *value);
void dprh_tbl_clear(dprh_tbl *tbl);

#endif

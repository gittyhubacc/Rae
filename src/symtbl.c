#include "symtbl.h"

void symtbl_init(arena *a, symtbl *tbl, int exp)
{
        for (int i = 0; i < SYMTBL_SCOPES; i++) {
                tbl->stack[i] = dprh_tbl_make(a, exp);
        }
        tbl->head = 0;
}

int symtbl_set(symtbl *tbl, string key, void *data)
{
        int rv = dprh_tbl_set(&tbl->stack[tbl->head], key, data);
        // printf("setting %.*s = %p: %i\n", key.len, key.addr, data, rv);
        return rv;
}

int symtbl_get(symtbl *tbl, string key, void **data)
{
        int cur = tbl->head;
        while (cur >= 0) {
                if (dprh_tbl_get(&tbl->stack[cur--], key, data)) {
                        // printf("getting %.*s = %p: 1\n", key.len, key.addr,
                        // *data);
                        return 1;
                }
        }
        // printf("getting %.*s: 0\n", key.len, key.addr);
        return 0;
}

void symtbl_scope_enter(symtbl *tbl) { tbl->head++; }

void symtbl_scope_leave(symtbl *tbl) { tbl->head--; }

void symtbl_scope_clear(symtbl *tbl) { dprh_tbl_clear(&tbl->stack[tbl->head]); }

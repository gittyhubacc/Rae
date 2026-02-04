#ifndef rae_symtbl_h
#define rae_symtbl_h

#include <mf/dprh.h>

#define SYMTBL_SCOPES (0x8)
typedef struct symtbl {
        int head;
        dprh_tbl stack[SYMTBL_SCOPES];
} symtbl;

void symtbl_init(arena *a, symtbl *tbl, int exp);

int symtbl_set(symtbl *tbl, string key, void *data);

int symtbl_get(symtbl *tbl, string key, void **data);

void symtbl_scope_enter(symtbl *tbl);

void symtbl_scope_leave(symtbl *tbl);

void symtbl_scope_clear(symtbl *tbl);

#endif

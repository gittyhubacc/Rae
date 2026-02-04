#include "mf/dprh.h"
#include <string.h>

#define P (257)        // set for 8bit ascii
#define M (2147483647) // this doesn't have to change
unsigned int dprh_hash(string s)
{
        int p = 1;
        unsigned int hash = 0;
        for (int i = 0; i < s.len; i++) {
                hash = (hash + (s.addr[i] - 'a' + 1) * p) % M;
                p = (P * p) % M;
        }
        return hash + 1;
}
#undef P
#undef M

int dprh_index(unsigned int hash, int exp, int idx)
{
        unsigned int mask = (1 << exp) - 1;
        unsigned int step = (hash >> (32 - exp)) | 1;
        return (idx + step) & mask;
}

dprh_tbl dprh_tbl_make(arena *a, int exp)
{
        dprh_tbl tbl;
        tbl.len = 0;
        tbl.exp = exp;
        tbl.addr = make(a, dprh_entry, 1 << tbl.exp);
        return tbl;
}

// will emit the entry for this key, or the first new entry
// will return 1 if matched key, 0 if empty entry
static int find_entry(dprh_tbl *tbl, string key, dprh_entry **entry)
{
        dprh_entry *cursor;
        int empty;
        int match;
        int hash = dprh_hash(key);
        int idx = dprh_index(hash, tbl->exp, hash);

        for (int seatbelt = 0; seatbelt < (1 << tbl->exp) + 5; seatbelt++) {
                cursor = &tbl->addr[idx];

                empty = !cursor->key.addr;
                match = streq(key, cursor->key);
                if (empty || match) {
                        // we found an empty or matching node
                        *entry = cursor;
                        return match;
                }
                idx = dprh_index(hash, tbl->exp, idx);
        }

        // we hit the emergency seatbelt, failure
        return 0;
}

int dprh_tbl_get(dprh_tbl *tbl, string key, void **value)
{
        dprh_entry *entry;
        int rv = find_entry(tbl, key, &entry);
        *value = entry->value;
        return rv;
}

int dprh_tbl_set(dprh_tbl *tbl, string key, void *value)
{
        if (tbl->len + 1 >= (1 << tbl->exp)) {
                // out of memory, cannot insert
                return -1;
        }

        dprh_entry *entry;
        find_entry(tbl, key, &entry);
        entry->value = value;
        entry->key = key;
        return 0;
}

void dprh_tbl_clear(dprh_tbl *tbl)
{
        for (int i = 0; i < (1 << tbl->exp); i++) {
                tbl->addr[i].value = 0;
                tbl->addr[i].key.len = 0;
                tbl->addr[i].key.addr = 0;
        }
}

#ifndef mf_string
#define mf_string

#include "memory.h"

typedef struct string {
        char *addr;
        int len;
} string;

typedef struct string_arr {
        int len;
        string *addr;
} string_arr;

#define S(s) ((string){s, sizeof(s) - 1})
#define S_NULL ((string){0, 0})

string itos(arena *a, int i);
string itosb(arena *a, int i, int base);
int stoi(string s);
float stof(string s);
int streq(string left, string right);
int indexof(string s, char c, int skip);
string concat(arena *a, string left, string right);
string string_mask(arena *a, char *c_str);

extern string epsilon;

#endif

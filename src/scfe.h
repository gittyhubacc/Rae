#ifndef scfe_h
#define scfe_h

#include <mf/list.h>
#include <mf/memory.h>
#include <mf/string.h>

typedef struct scfe_tkn {
        int type;
        string sdata;
        int line_num;
        int col_pos;
} scfe_tkn;

typedef struct scfe_tkn_arr {
        int len;
        scfe_tkn *addr;
} scfe_tkn_arr;

typedef struct _ptnode {
        int rule;
        int child_cnt;
        scfe_tkn label;
        struct _ptnode *parent;
        struct _ptnode *children;
} ptnode;

typedef struct scfe_nfa_arrow {
        int state;
        int input;
        int dest;
} scfe_nfa_arrow;

typedef struct scfe_nfa_delta {
        int rule_cnt;
        scfe_nfa_arrow *rules;
} scfe_nfa_delta;

typedef struct scfe_nfa {
        int start;
        int accept;
        scfe_nfa_delta delta;
} scfe_nfa;

typedef struct scfe_nfa_arr {
        int len;
        scfe_nfa *addr;
} scfe_nfa_arr;

scfe_nfa scfe_nfa_union(arena *a, scfe_nfa s, scfe_nfa r);

scfe_nfa scfe_nfa_concat(arena *a, scfe_nfa r, scfe_nfa s);

scfe_nfa scfe_nfa_close(arena *a, scfe_nfa r);

int scfe_nfa_membership(arena *a, scfe_nfa n, string input);

typedef struct scfe_pda_rule {
        scfe_tkn head;
        scfe_tkn_arr body;
} scfe_pda_rule;

typedef struct scfe_pda_rule_arr {
        int len;
        scfe_pda_rule *addr;
} scfe_pda_rule_arr;

typedef struct scfe_pda_id {
        int i;
        int rule;
        scfe_tkn head;
        scfe_tkn_arr body;
        scfe_tkn_arr look_ahead;
} scfe_pda_id;

typedef struct scfe_pda_state {
        list *ids; // list of scfe_pda_id*
} scfe_pda_state;

#define SCFE_PARSER_STACK_SZ (32)
typedef struct scfe_parser {
        int pt_i;
        int state_i;
        scfe_pda_state state_stack[SCFE_PARSER_STACK_SZ];
        ptnode *pt_stack[SCFE_PARSER_STACK_SZ];
} scfe_parser;

typedef struct scfe_grammar {
        string_arr terminals;
        scfe_pda_rule_arr nonterminals;
} scfe_grammar;

ptnode *scfe_parse_tree(arena *a, arena tmp, scfe_grammar *g,
                             string input);
ptnode *scfe_parse(arena *a, arena tmp, scfe_grammar *g,
                        scfe_tkn_arr tokens);
scfe_nfa scfe_regex(arena *a, arena tmp, string re);

#endif

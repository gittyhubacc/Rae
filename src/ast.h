#ifndef ast_h
#define ast_h

#include <mf/string.h>
#include <mf/array.h>

#include "symtbl.h"
#include "scfe.h"

typedef enum {
	AST_E_INT,
	AST_E_FLOAT,
	AST_E_STRING,
	AST_E_CALL,
	AST_E_VAR,
	AST_E_CONS,
	AST_E_MUL,
	AST_E_DIV,
	AST_E_ADD,
	AST_E_SUB,
	AST_E_MOD,
	AST_E_REF,
	AST_E_INDEX,
	AST_E_MEMACC,
	AST_E_DEREF,
	AST_E_DEREF_MEMACC,
	AST_E_EQ,
	AST_E_NEQ,
	AST_E_LT,
	AST_E_LTE,
	AST_E_GT,
	AST_E_GTE,
	AST_E_AND,
	AST_E_OR,
	AST_E_CAST,
	AST_S_RETURN,
	AST_S_IF,
	AST_S_IFELSE,
	AST_S_WHILE,
	AST_S_DECL,
	AST_S_BLOCK,
	AST_S_ASSIGN,
	AST_S_CALL,
	AST_PROC_DEF,
	AST_PROC_IMPORT,
	AST_STRUCT_DEF,
	AST_MODULE,
	AST_SEAM,
} astnode_kind;

typedef enum {
	TNK_i8 = 1 << 0,
	TNK_i32 = 1 << 1,
	TNK_i64 = 1 << 2,
	TNK_f32 = 1 << 3,
	TNK_f64 = 1 << 4,
	TNK_UNIT = 1 << 5,
	TNK_PTR = 1 << 6,
	TNK_PROC = 1 << 7,
	TNK_ARRAY = 1 << 8,
	TNK_STRUCT = 1 << 9,
} typenode_kind;

#define TNK_INTEGER (TNK_i8 | TNK_i32 | TNK_i64)
#define TNK_FLOAT (TNK_f32 | TNK_f64)
#define TNK_INDEXABLE (TNK_ARRAY | TNK_PTR)

typedef struct typenode typenode;

struct proc_def_arg {
	typenode *type;
	string name;
};

ARRAY_DEF(arg, struct proc_def_arg *);

struct tdat_ptr {
	struct typenode *subject;
};

struct tdat_proc {
	arr_arg args;
	typenode *return_type;
};

struct tdat_array {
	int count;
	typenode *subject;
};

struct tdat_struct_mem {
	string name;
	typenode *type;
};

ARRAY_DEF(mem, struct tdat_struct_mem *)


typedef struct {
	string ns;
	string id;
} eligible_id;

struct tdat_struct {
	eligible_id name;
	arr_mem members;
};

typedef struct typenode {
	typenode_kind kind;
	union {
		struct tdat_ptr ptr;
		struct tdat_proc proc;
		struct tdat_array array;
		struct tdat_struct strct;
	};
} typenode;

typedef struct astnode astnode;

ARRAY_DEF(ast, astnode *);

struct adat_i8 {
	char byte;
};

struct adat_i32 {
	int value;
};

struct adat_i64 {
	long value;
};

struct adat_f32 {
	float value;
};

struct adat_f64 {
	double value;
};

struct adat_bin_op {
	astnode *left;
	astnode *right;
};

struct adat_var {
	string ns;
	string id;
};

struct adat_acc {
	astnode *strct;
	string name;
	int idx;
};

struct adat_index {
	astnode *array;
	astnode *idx;
};

struct adat_ref {
	astnode *expr; // maybe should be restricted to a variable?
};

struct adat_deref {
	astnode *ptr; // we dereference pointers... right?
};

struct adat_rstmt {
	astnode *value;
};

struct adat_dstmt {
	string name;
	typenode *type;
	astnode *expr;
};

struct adat_astmt {
	astnode *lval;
	astnode *expr;
};

struct adat_cstmt {
	astnode *fn_call;
};

struct adat_wstmt {
	astnode *cond;
	arr_ast body;
};

struct adat_bstmt {
	arr_ast body;
};

struct adat_istmt {
	astnode *cond;
	arr_ast branch_t;
	arr_ast branch_f;
};

struct adat_proc_def {
	int exported;
	int implicit_ret;
	eligible_id name;
	arr_arg args;
	arr_ast stmts;
	typenode *return_type;
};

struct adat_proc_imp {
	eligible_id name;
	arr_arg args;
	typenode *return_type;
};

struct adat_proc_call {
	eligible_id name;
	arr_ast arguments;
};

struct adat_strct {
	eligible_id name;
};

struct adat_module {
	arr_ast stmts; // top level statements
};

struct adat_cast {
	astnode *expr;
};

struct adat_string {
	string data;
};


typedef struct astnode {
	astnode_kind kind;
	typenode *type;
	union {
		struct adat_i8 i8;
		struct adat_i32 i32;
		struct adat_i64 i64;
		struct adat_f32 f32;
		struct adat_f64 f64;
		struct adat_var var;
		struct adat_acc acc;
		struct adat_acc drfacc;
		struct adat_index index;;
		struct adat_ref ref;
		struct adat_deref deref;
		struct adat_bin_op bin_op;
		struct adat_rstmt rstmt;
		struct adat_dstmt dstmt;
		struct adat_astmt astmt;
		struct adat_cstmt cstmt;
		struct adat_wstmt wstmt;
		struct adat_istmt istmt;
		struct adat_bstmt bstmt;
		struct adat_proc_def proc_def;
		struct adat_proc_imp proc_imp;
		struct adat_proc_call call;
		struct adat_module module;
		struct adat_strct strct;
		struct adat_cast cast;
		struct adat_string string;
	};
} astnode;

struct ns {
	string name;
	dprh_tbl proc;
	dprh_tbl attr;
	dprh_tbl strct;
	dprh_tbl aspct;
};

typedef struct {
	list *df_ns;
	string ctx_ns;
	dprh_tbl namspaces; 	// string -> struct ns *
	symtbl *types; 		// string -> typenode
} semctx;

typedef struct attr_arg_decl {
	eligible_id attr;
	string var;
} attr_arg_decl;

struct tattr {
	string regex;
};

struct nattr {
	int argc;
	attr_arg_decl **argv;
};

typedef struct attr_rule {
	eligible_id name;
	int is_term; // is terminal/nonterminal
	int is_expr; // is expression/statement
	union {
		struct tattr term;
		struct nattr ntrm;
	};
	ptnode *body; // raw pt from when we parsed the attr def
} attr_rule;

typedef enum {
	ADV_BEFORE,
	ADV_AFTER
} advice_verb;

typedef struct {
	eligible_id src;
	advice_verb verb;
	eligible_id dest;
} advice_t;

typedef struct {
	int len;
	advice_t **addr;
	eligible_id name;
} aspect_t;

// pregrammar has lists of attr_rule
typedef struct pregrammar {
	int expr;
	int tm_count;
	list *tm_rules;
	int nt_count;
	list *nt_rules;
	dprh_tbl rule_bag;
        dprh_tbl name_idx_tbl;//= dprh_tbl_make(a, 6);
} pregrammar;

// takes parse tree, returns meaningful type-checked ast
semctx *semctx_make(arena *a);
astnode *sem_engine(arena *a, arena tmp, semctx *ctx, ptnode *pt);

extern int typenode_eq(typenode *left, typenode *right);
extern astnode *promote_type(arena *a, typenode *tn, astnode *ast);
extern astnode *create_cast(arena *a, typenode *type, astnode *expr);

#endif

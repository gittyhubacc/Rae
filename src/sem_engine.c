#include "ast.h"
#include "grammar.h"
#include "errfmt.h"
#include <string.h>

static struct ns *ns_make(arena *a, string name)
{
	struct ns *ns = make(a, struct ns, 1);
	ns->name = name;
	ns->proc = dprh_tbl_make(a, 5);
	ns->attr = dprh_tbl_make(a, 5);
	ns->strct = dprh_tbl_make(a, 5);
	ns->aspct = dprh_tbl_make(a, 5);
	return ns;
}

static string qualified_id_mangle(arena *a, string raw)
{
	string copy;
	copy.len = raw.len;
	copy.addr = make(a, char, copy.len);
	for (int i = 0; i < raw.len; i++) {
		copy.addr[i] = raw.addr[i];
		if (copy.addr[i] == ':') {
			copy.addr[i] = '_';
		}
	}
	return copy;
}

static eligible_id qualified_id_parse(string raw)
{
	eligible_id id;
	id.ns.len = 0;
	id.ns.addr = raw.addr;
	while (id.ns.addr[id.ns.len] != ':') {
		id.ns.len++;
	}
	id.id.len = raw.len - id.ns.len - 1;
	id.id.addr = raw.addr + id.ns.len + 1;
	return id;
}

#define STD_HEXCODE "std:0x!!"
semctx *semctx_make(arena *a)
{
	semctx *ctx = make(a, semctx, 1);
	ctx->ctx_ns = S_NULL;

	// gamma, typing context
	ctx->types = make(a, symtbl, 1);
	symtbl_init(a, ctx->types, 5);

	// collection of name spaces
	ctx->namspaces = dprh_tbl_make(a, 5);

	// create std namespace, populate with initial types n that
	string std_name = S("std");
	struct ns *std = make(a, struct ns, 1); 
	std->name = std_name;
	std->proc = dprh_tbl_make(a, 5);
	std->attr = dprh_tbl_make(a, 9);
	std->strct = dprh_tbl_make(a, 5);
	std->aspct = dprh_tbl_make(a, 5);
	dprh_tbl_set(&ctx->namspaces, std_name, std);

	typenode *unit = make(a, typenode, 1);
	unit->kind = TNK_UNIT;
	dprh_tbl_set(&std->strct, S("unit"), unit);

	typenode *i8 = make(a, typenode, 1);
	i8->kind = TNK_i8;
	dprh_tbl_set(&std->strct, S("i8"), i8);

	typenode *i32 = make(a, typenode, 1);
	i32->kind = TNK_i32;
	dprh_tbl_set(&std->strct, S("i32"), i32);

	typenode *i64 = make(a, typenode, 1);
	i64->kind = TNK_i64;
	dprh_tbl_set(&std->strct, S("i64"), i64);

	typenode *f32 = make(a, typenode, 1);
	f32->kind = TNK_f32;
	dprh_tbl_set(&std->strct, S("f32"), f32);

	typenode *f64 = make(a, typenode, 1);
	f64->kind = TNK_f64;
	dprh_tbl_set(&std->strct, S("f64"), f64);

	string str_name = S("string");
	eligible_id string_id = {
		.id = str_name,
		.ns = std_name
	};
	typenode *str = make(a, typenode, 1);
	str->kind = TNK_STRUCT;
	str->strct.name = string_id;
	str->strct.members.len = 2;
	str->strct.members.memory = make(a, struct tdat_struct_mem *, 2);
	str->strct.members.memory[0] = make(a, struct tdat_struct_mem, 1);
	str->strct.members.memory[0]->name = S("len");
	str->strct.members.memory[0]->type = i32;
	str->strct.members.memory[1] = make(a, struct tdat_struct_mem, 1);
	str->strct.members.memory[1]->name = S("addr");
	str->strct.members.memory[1]->type = make(a, typenode, 1);
	str->strct.members.memory[1]->type->kind = TNK_PTR;
	str->strct.members.memory[1]->type->ptr.subject = i8;
	dprh_tbl_set(&std->strct, str_name, str);

	list *node = make(a, list, 1);
	attr_rule *lit_int = make(a, attr_rule, 1);
	lit_int->name = qualified_id_parse(S("std:lit_int"));
	lit_int->is_term = 1;
	lit_int->term.regex = S("0+([1-9].[0-9]*)");
	node->data = lit_int;
	node->next = NULL;
	node->last = node;
	dprh_tbl_set(&std->attr, lit_int->name.id, node);

	node = make(a, list, 1);
	attr_rule *lit_float = make(a, attr_rule, 1);
	lit_float->name = qualified_id_parse(S("std:lit_float"));
	lit_float->is_term = 1;
	lit_float->term.regex = S("(0+([1-9].[0-9]*)).(~+\\..([0-9]*))");
	node->data = lit_float;
	node->next = NULL;
	node->last = node;
	dprh_tbl_set(&std->attr, lit_float->name.id, node);

	node = make(a, list, 1);
	attr_rule *lit_string = make(a, attr_rule, 1);
	lit_string->name = qualified_id_parse(S("std:lit_string"));
	lit_string->is_term = 1;
	lit_string->term.regex = S("\".([\\-!]+[#-\\])*.(~+\")");
	node->data = lit_string;
	node->next = NULL;
	node->last = node;
	dprh_tbl_set(&std->attr, lit_string->name.id, node);

	node = make(a, list, 1);
	attr_rule *alphanum = make(a, attr_rule, 1);
	alphanum->name = qualified_id_parse(S("std:alphanumeric"));
	alphanum->is_term = 1;
	alphanum->term.regex = S("[a-z]+[A-Z]+[0-9]");
	node->data = alphanum;
	node->next = NULL;
	node->last = node;
	dprh_tbl_set(&std->attr, alphanum->name.id, node);

	node = make(a, list, 1);
	attr_rule *alphanum2 = make(a, attr_rule, 1);
	alphanum2->name = qualified_id_parse(S("std:alphanumstr"));
	alphanum2->is_term = 1;
	alphanum2->term.regex = S("([a-z]+[A-Z]+[0-9]).([a-z]+[A-Z]+[0-9])*");
	node->data = alphanum2;
	node->next = NULL;
	node->last = node;
	dprh_tbl_set(&std->attr, alphanum2->name.id, node);

	node = make(a, list, 1);
	attr_rule *punctuation = make(a, attr_rule, 1);
	punctuation->name = qualified_id_parse(S("std:punctuation"));
	punctuation->is_term = 1;
	punctuation->term.regex = S(",+\\.+!+?+:");
	node->data = punctuation;
	node->next = NULL;
	node->last = node;
	dprh_tbl_set(&std->attr, punctuation->name.id, node);

	node = make(a, list, 1);
	attr_rule *glob = make(a, attr_rule, 1);
	glob->name = qualified_id_parse(S("std:glob"));
	glob->is_term = 1;
	//glob->term.regex = S("([\\-;]+=+[?-\\]).([\\-;]+=+[?-\\])*");
	glob->term.regex = S("([a-z]+[A-Z]+[0-9]+\\ +,+\\.+!+?).([a-z]+[A-Z]+[0-9]+\\ +,+\\.+!+?)*");
	node->data = glob;
	node->next = NULL;
	node->last = node;
	dprh_tbl_set(&std->attr, glob->name.id, node);

	// ascii codes
	
	attr_rule *code;
	char *buffer = make(a, char, strlen(STD_HEXCODE) + 1);
	strcpy(buffer, STD_HEXCODE);
	char digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
	string codename;
	codename.len = strlen(STD_HEXCODE);
	string regex;
	regex.len = 2;
	for (int i = 0x00; i <= 0xFF; i++) {
		code = make(a, attr_rule, 1);
		buffer[6] = digits[i >> 4];
		buffer[7] = digits[i & 0x0F];
		codename.addr = make(a, char, codename.len + 1);
		strcpy(codename.addr, buffer);

		code->name = qualified_id_parse(codename);
		code->is_term = 1;
		regex.addr = make(a, char, 2);
		regex.addr[0] = '\\';
		regex.addr[1] = i;
		code->term.regex = regex;

		node = make(a, list, 1);
		node->data = code;
		node->next = NULL;
		node->last = node;
		dprh_tbl_set(&std->attr, code->name.id, node);
	}


	/*
	list *dfns_root = make(a, list, 1);
	dfns_root->last = dfns_root;
	dfns_root->next = NULL;
	dfns_root->data = std;

	ctx->df_ns = dfns_root;
	*/
	ctx->df_ns = NULL;

	return ctx;
}
static eligible_id eligible_id_make(ptnode *pt)
{
	eligible_id id;

	if (pt->children[0].label.type == TTOK_ID_PLAIN) {
		id.ns = S_NULL;
		id.id = pt->children[0].label.sdata;
		return id;
	}
	return qualified_id_parse(pt->children[0].label.sdata);
}

int typenode_eq(typenode *left, typenode *right)
{
	if (left->kind != right->kind) {
		return 0;
	} else if (left == right) {
		return 1;
	}
	switch (left->kind) {
	case TNK_i8:
	case TNK_i32:
	case TNK_i64:
	case TNK_f32:
	case TNK_f64:
	case TNK_UNIT:
		return 1;
	case TNK_PTR:
		return typenode_eq(left->ptr.subject, right->ptr.subject);
	case TNK_PROC: {
		arr_arg args = left->proc.args;
		for (int i = 0; i < args.len; i++) {
			if (!typenode_eq(args.memory[i]->type, args.memory[i]->type)) {
				return 0;
			}
		}
		return typenode_eq(left->proc.return_type, right->proc.return_type);
       }
	case TNK_ARRAY:
		return left->array.count == right->array.count 
			&& typenode_eq(left->array.subject, right->array.subject);
	case TNK_STRUCT:
		if (left->strct.members.len != right->strct.members.len) {
			return 0;
		}
		for (int i = 0; i < left->strct.members.len; i++) {
			typenode *tl = left->strct.members.memory[i]->type;
			typenode *tr = right->strct.members.memory[i]->type;
			if (!typenode_eq(tl, tr)) {
				return 0;
			}
		}
		return 1;
	}

	return 0;
}

static astnode *syn_lval(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	typenode *tn;
	astnode *lval = make(a, astnode, 1);

	switch (pt->rule) {
	case SYN_LVAL_ID:
		lval->kind = AST_E_VAR;
		lval->var.id = pt->children[0].label.sdata;
		if (!symtbl_get(ctx->types, lval->var.id, (void **)&lval->type)) {
			FATAL_UNDEF_VAR(lval->var.id);
		}
		break;
	case SYN_LVAL_DEREF:
		lval->kind = AST_E_DEREF;
		lval->deref.ptr = syn_lval(a, tmp, ctx, &pt->children[1]);
		lval->type = lval->deref.ptr->type->ptr.subject;
		break;
	case SYN_LVAL_MEMACC:
		lval->kind = AST_E_MEMACC;
		lval->acc.name = pt->children[2].label.sdata;
		lval->acc.strct = syn_lval(a, tmp, ctx, &pt->children[0]);

		lval->type = NULL;
		tn = lval->acc.strct->type;
		if (tn->kind != TNK_STRUCT) {
			FATAL_BAD_ACC(lval->acc.name);
		}
		for (int i = 0; i < tn->strct.members.len; i++) {
			if (streq(tn->strct.members.memory[i]->name, lval->acc.name)) {
				lval->acc.idx = i;
				lval->type = tn->strct.members.memory[i]->type;
			}
		}
		if (lval->type == NULL) {
			FATAL_BAD_MEMBER(lval->acc.name);
		}
		break;
	case SYN_LVAL_DEREF_MEMACC:
		lval->kind = AST_E_DEREF_MEMACC;
		lval->drfacc.name = pt->children[2].label.sdata;
		lval->drfacc.strct = syn_lval(a, tmp, ctx, &pt->children[0]);

		lval->type = NULL;
		tn = lval->drfacc.strct->type;
		typenode *strct_type = tn->ptr.subject;
		if (tn->kind != TNK_PTR || strct_type->kind != TNK_STRUCT) {
			FATAL_BAD_ACC(lval->drfacc.name);
		}
		for (int i = 0; i < strct_type->strct.members.len; i++) {
			struct tdat_struct_mem *member;
			member = strct_type->strct.members.memory[i];
			if (streq(member->name, lval->drfacc.name)) {
				lval->type = member->type;
				lval->drfacc.idx = i;
				break;
			}
		}
		if (lval->type == NULL) {
			FATAL_BAD_MEMBER(lval->drfacc.name);
		}
		break;
	case SYN_LVAL_INDEX:
		lval->kind = AST_E_INDEX;
		lval->index.idx = sem_engine(a, tmp, ctx, &pt->children[2]);
		lval->index.array = syn_lval(a, tmp, ctx, &pt->children[0]);
		lval->type = lval->index.array->type->array.subject;
		if (lval->index.array->type->kind != TNK_ARRAY) {
			if (lval->index.array->type->kind != TNK_PTR) {
				FATAL_BAD_INDEX();
			}
			lval->type = lval->index.array->type->ptr.subject;
		}
		break;
	default:
		FATAL_UNSUP_LVAL(pt->rule);
		break;
	}

	return lval;
}


static astnode *syn_prim_int(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);
	ast->kind = AST_E_INT;
	ast->type = make(a, typenode, 1);
	ast->type->kind = TNK_i32;
	ast->i32.value = stoi(pt->children[0].label.sdata);
	if (ast->i32.value <= 0xFF) {
		ast->type->kind = TNK_i8;
		ast->i8.byte = ast->i32.value;
	}
	return ast;
}

static astnode *syn_prim_float(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);
	ast->kind = AST_E_FLOAT;
	ast->type = make(a, typenode, 1);
	ast->type->kind = TNK_f32;
	ast->f32.value = stof(pt->children[0].label.sdata);
	return ast;
}

static list *syn_elist(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	list *node = make(a, list, 1);
	if (pt->rule == SYN_ELIST_ROOT) {
		node->data = sem_engine(a, tmp, ctx, &pt->children[0]);
		node->last = node;
		node->next = NULL;
		return node;
	}
	list *head = syn_elist(a, tmp, ctx, &pt->children[0]);
	node->data = sem_engine(a, tmp, ctx, &pt->children[2]);
	list_append(&head, node);
	return head;
}

static arr_ast *expr_list_to_array(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	arr_ast *arr = make(a, arr_ast, 1);
	list *exprs = syn_elist(a, tmp, ctx, pt);
	int count = 0;
	list *cursor = exprs;
	while (cursor) {
		count++;
		cursor = cursor->next;
	}

	arr->len = count;
	arr->memory = make(a, astnode *, count);
	count = 0;
	cursor = exprs;
	while (cursor) {
		arr->memory[count++] = cursor->data;
		cursor = cursor->next;
	}

	return arr;
}

static astnode *syn_prim_papp(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);
	ast->kind = AST_E_CALL;

	struct ns *ns;
	int switched = 0;
	eligible_id name = eligible_id_make(&pt->children[0]);
	if (name.ns.addr == NULL || name.ns.len < 1) {
		name.ns = ctx->ctx_ns;
		switched = 1;
	} 
	if (!dprh_tbl_get(&ctx->namspaces, name.ns, (void **)&ns)) {
		FATAL_BAD_NS(name.ns);
	}

	typenode *fntn;
	if (!dprh_tbl_get(&ns->proc, name.id, (void **)&fntn)) {
		int found = 0;
		if (switched) {
			list *cursor = ctx->df_ns;
			while (cursor) {
				ns = cursor->data;
				if (dprh_tbl_get(&ns->proc, name.id, (void **)&fntn)) {
					name.ns = ns->name;
					found = 1;
					break;
				}
				cursor = cursor->next;
			}
		}
		if (!found) {
			FATAL_UNDEF_VAR(name.id);
		}
	}

	// we need to make sure all our arguments have the correct types
	ast->call.name = name;
	ast->call.arguments.len = 0;
	ast->call.arguments.memory = 0;
	if (pt->rule == SYN_PROC_APP_ARGS) {
		ast->call.arguments = *expr_list_to_array(a, tmp, ctx, &pt->children[2]);
		if (ast->call.arguments.len != fntn->proc.args.len) {
			FATAL_CALL_BADLEN(name.id);
		}
		for (int i = 0; i < fntn->proc.args.len; i++) {
			typenode *expected = fntn->proc.args.memory[i]->type;
			typenode *actual = ast->call.arguments.memory[i]->type;
			if (!typenode_eq(expected, actual)) {
				ast->call.arguments.memory[i] = promote_type(a, expected, ast->call.arguments.memory[i]);
				if (!ast->call.arguments.memory[i]) {
					FATAL_CALL_BADTYPE(name.id, i);
				}
			}
		}
	}

	// we are well typed :^)
	ast->type = fntn->proc.return_type;

	return ast;
}

static astnode *syn_prim_deref(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);
	ast->kind = AST_E_DEREF;
	ast->deref.ptr = sem_engine(a, tmp, ctx, &pt->children[1]);
	ast->type = ast->deref.ptr->type->ptr.subject; // we're our subject's subject.....
	return ast;
}


static astnode *syn_var_id(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_E_VAR;

	eligible_id id = eligible_id_make(&pt->children[0]);
        if (!symtbl_get(ctx->types, id.id, (void **)&ast->type)) {
		FATAL_UNDEF_VAR(id.id);
	}

	ast->var.ns = id.ns;
	ast->var.id = id.id;

	return ast;
}

static astnode *syn_var_memacc(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_E_MEMACC;

	ast->acc.name = pt->children[2].label.sdata;
	ast->acc.strct = sem_engine(a, tmp, ctx, &pt->children[0]);
	if (ast->acc.strct->type->kind != TNK_STRUCT) {
		FATAL_BAD_ACC(ast->acc.name);
	}
	ast->acc.idx = -1;
	typenode *tn = ast->acc.strct->type;
	for (int i = 0; i < tn->strct.members.len; i++) {
		string mem_name = tn->strct.members.memory[i]->name;
		typenode *mem_type = tn->strct.members.memory[i]->type; 
		if (streq(mem_name, ast->acc.name)) {
			ast->type = mem_type;
			ast->acc.idx = i;
			break;
		}
	}
	if (ast->acc.idx < 0) {
		FATAL_BAD_MEMBER(ast->acc.name);
	}

	return ast;
}

static astnode *syn_var_index(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_E_INDEX;
	ast->index.idx = sem_engine(a, tmp, ctx, &pt->children[2]);
	ast->index.array = sem_engine(a, tmp, ctx, &pt->children[0]);
	if (!(ast->index.array->type->kind & TNK_INDEXABLE)) {
		FATAL_BAD_INDEX();
	}
	ast->type = ast->index.array->type->kind == TNK_ARRAY
		? ast->index.array->type->array.subject
		: ast->index.array->type->ptr.subject;

	return ast;
}

static astnode *syn_var_deref_memacc(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_E_DEREF_MEMACC;
	ast->drfacc.name = pt->children[2].label.sdata;
	ast->drfacc.strct = sem_engine(a, tmp, ctx, &pt->children[0]);

	ast->type = NULL;
	typenode *tn = ast->drfacc.strct->type;
	typenode *strct_type = tn->ptr.subject;
	if (tn->kind != TNK_PTR || strct_type->kind != TNK_STRUCT) {
		FATAL_BAD_ACC(ast->drfacc.name);
	}
	for (int i = 0; i < strct_type->strct.members.len; i++) {
		struct tdat_struct_mem *member;
		member = strct_type->strct.members.memory[i];
		if (streq(member->name, ast->drfacc.name)) {
			ast->type = member->type;
			ast->drfacc.idx = i;
			break;
		}
	}
	if (ast->type == NULL) {
		FATAL_BAD_MEMBER(ast->drfacc.name);
	}

	return ast;
}

// we need some sort of "type_promote" to make types play nice and make friends
static typenode_kind type_coalesce(typenode *left, typenode *right)
{
	if (left->kind & TNK_INTEGER && right->kind & TNK_INTEGER) {
		return left->kind >= right->kind ? left->kind : right->kind;
	} else if (left->kind & TNK_FLOAT && right->kind & TNK_FLOAT) {
		return left->kind >= right->kind ? left->kind : right->kind;
	}

	FATAL_GENERIC_UNSUPPORTED("failed type coalesce");
}

astnode *create_cast(arena *a, typenode *type, astnode *expr)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_E_CAST;
	ast->type = type;
	ast->cast.expr = expr;

	return ast;
}

static astnode *syn_bin_op(arena *a, arena tmp, char *name, astnode_kind kind, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = kind;

	astnode *left = sem_engine(a, tmp, ctx, &pt->children[0]);
	astnode *right = sem_engine(a, tmp, ctx, &pt->children[2]);

	if (left->type->kind == TNK_PTR && right->type->kind & TNK_INTEGER) {
		ast->type = left->type;
	} else if (right->type->kind == TNK_PTR && left->type->kind & TNK_INTEGER) {
		ast->type = right->type;
	} else {
		ast->type = make(a, typenode, 1);
		ast->type->kind = type_coalesce(left->type, right->type);
	}
	if (ast->type->kind < 0) {
		FATAL_BAD_TYPES(name, left->type->kind, right->type->kind);
	}

	ast->bin_op.left = left;
	ast->bin_op.right = right;

	return ast;
}

static astnode *syn_expr_lt(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_E_LT;
	ast->type = make(a, typenode, 1);
	ast->type->kind = TNK_i32;
	ast->bin_op.left = sem_engine(a, tmp, ctx, &pt->children[0]);
	ast->bin_op.right = sem_engine(a, tmp, ctx, &pt->children[2]);

	return ast;
}

static astnode *syn_expr_gt(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_E_GT;
	ast->type = make(a, typenode, 1);
	ast->type->kind = TNK_i32;
	ast->bin_op.left = sem_engine(a, tmp, ctx, &pt->children[0]);
	ast->bin_op.right = sem_engine(a, tmp, ctx, &pt->children[2]);

	return ast;
}

static astnode *syn_expr_lte(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_E_LTE;
	ast->type = make(a, typenode, 1);
	ast->type->kind = TNK_i32;
	ast->bin_op.left = sem_engine(a, tmp, ctx, &pt->children[0]);
	ast->bin_op.right = sem_engine(a, tmp, ctx, &pt->children[2]);

	return ast;
}

static astnode *syn_expr_gte(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_E_GTE;
	ast->type = make(a, typenode, 1);
	ast->type->kind = TNK_i32;
	ast->bin_op.left = sem_engine(a, tmp, ctx, &pt->children[0]);
	ast->bin_op.right = sem_engine(a, tmp, ctx, &pt->children[2]);

	return ast;
}

static astnode *syn_expr_eq(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_E_EQ;
	ast->type = make(a, typenode, 1);
	ast->type->kind = TNK_i32;
	ast->bin_op.left = sem_engine(a, tmp, ctx, &pt->children[0]);
	ast->bin_op.right = sem_engine(a, tmp, ctx, &pt->children[2]);

	return ast;
}

static astnode *syn_expr_neq(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_E_NEQ;
	ast->type = make(a, typenode, 1);
	ast->type->kind = TNK_i32;
	ast->bin_op.left = sem_engine(a, tmp, ctx, &pt->children[0]);
	ast->bin_op.right = sem_engine(a, tmp, ctx, &pt->children[2]);

	return ast;
}

static astnode *syn_expr_string(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_E_STRING;
	int sansQuote = pt->label.type == -47 ? 0 : 2;
	//printf("sansquote: %i\n", sansQuote);
	ast->string.data.len = pt->children[0].label.sdata.len - sansQuote;
	//printf("with '%.*s'\n", pt->children[0].label.sdata.len, pt->children[0].label.sdata.addr);
	ast->string.data.addr = make(a, char, ast->string.data.len);
	char ch;
	int i = 0;
	int bump = pt->label.type == -47 ? 0 : 1;
	int j = bump; // this is a hack related to string cat
	while (j < pt->children[0].label.sdata.len - bump) {
		ch = pt->children[0].label.sdata.addr[j++];
		if (j < pt->children[0].label.sdata.len - 1 && ch == '\\') {
			switch(pt->children[0].label.sdata.addr[j]) {
			case 'n':
				j++;
				ch = '\n';
				break;
			}
		}
		ast->string.data.addr[i++] = ch;
	}
	ast->string.data.len = i;
	ast->type = make(a, typenode, 1);
	ast->type->kind = TNK_PTR;

	struct ns *ns;
	if (!dprh_tbl_get(&ctx->namspaces, S("std"), (void **)&ns)) {
		FATAL_BAD_NS(S("std"));
		// include the stdlib!
	}

	if (!dprh_tbl_get(&ns->strct, S("string"), (void **)&ast->type->ptr.subject)) {
		FATAL_UNDEF_VAR(S("string"));
		// include the stdlib!
	}

	return ast;
}

static astnode *syn_expr_cat(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);
	ast->kind = AST_E_STRING;
	ast->type = make(a, typenode, 1);
	ast->type->kind = TNK_PTR;

	struct ns *ns;
	if (!dprh_tbl_get(&ctx->namspaces, S("std"), (void **)&ns)) {
		FATAL_BAD_NS(S("std"));
		// include the stdlib!
	}

	if (!dprh_tbl_get(&ns->strct, S("string"), (void **)&ast->type->ptr.subject)) {
		FATAL_UNDEF_VAR(S("string"));
		// include the stdlib!
	}

	astnode *left = sem_engine(a, tmp, ctx, &pt->children[0]);
	if (left->kind != AST_E_STRING) {
		FATAL_GENERIC_UNSUPPORTED("using cat with non-strings");
	}
	//printf("found '%.*s'\n", left->string.data.len, left->string.data.addr);

	astnode *right = sem_engine(a, tmp, ctx, &pt->children[2]);
	if (right->kind != AST_E_STRING) {
		FATAL_GENERIC_UNSUPPORTED("using cat with non-strings");
	}
	//printf("found '%.*s'\n", right->string.data.len, right->string.data.addr);

	ast->string.data.len = left->string.data.len + right->string.data.len;
	ast->string.data.addr = make(a, char, ast->string.data.len);
	int i = 0;
	for (int j = 0; j < left->string.data.len; j++) {
		ast->string.data.addr[i++] = left->string.data.addr[j];
	}
	for (int j = 0; j < right->string.data.len; j++) {
		ast->string.data.addr[i++] = right->string.data.addr[j];
	}

	//printf("produced '%.*s'\n", ast->string.data.len, ast->string.data.addr);
	return ast;
}

static astnode *syn_expr_ref(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_E_REF;
	ast->ref.expr = sem_engine(a, tmp, ctx, &pt->children[1]);
	ast->type = make(a, typenode, 1);
	ast->type->kind = TNK_PTR;
	ast->type->ptr.subject = ast->ref.expr->type;

	return ast;
}

static typenode *typenode_make(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	typenode *tn = make(a, typenode, 1);

	switch (pt->rule) {
	case SYN_TYPEXPR_ID: {
		struct ns *ns;
		eligible_id id = eligible_id_make(&pt->children[0]);
		if (id.ns.addr == NULL || id.ns.len < 1) {
			id.ns = ctx->ctx_ns;
		} 
		if (id.ns.addr && !dprh_tbl_get(&ctx->namspaces, id.ns, (void **)&ns)) {
			FATAL_BAD_NS(id.ns);
		}
		if (!id.ns.addr || !dprh_tbl_get(&ns->strct, id.id, (void **)&tn)) {
			int found = 0;
			list *cursor = ctx->df_ns;
			while (cursor) {
				ns = cursor->data;
				if (dprh_tbl_get(&ns->strct, id.id, (void **)&tn)) {
					found = 1;
					break;
				}
				cursor = cursor->next;
			}
			if (!found) {
				FATAL_UNDEF_TYPE(id.id);
			}
		}
		break;
	}
	case SYN_TYPEXPR_PTR:
		tn->kind = TNK_PTR;
		tn->ptr.subject = typenode_make(a, tmp, ctx, &pt->children[1]);
		break;
	case SYN_TYPEXPR_ARR:
		tn->kind = TNK_ARRAY;
		tn->array.count = stoi(pt->children[2].label.sdata);
		tn->array.subject = typenode_make(a, tmp, ctx, &pt->children[0]);
		break;
	default:
		FATAL_UNSUP_TYPE(pt->rule);
	}

	return tn;
}

static astnode *syn_expr_cast(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_E_CAST;
	ast->cast.expr = sem_engine(a, tmp, ctx, &pt->children[1]);
	ast->type = typenode_make(a, tmp, ctx, &pt->children[3]);

	return ast;
}

static astnode *syn_rstmt_expr(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_S_RETURN;
	ast->type = make(a, typenode, 1);
	ast->type->kind = TNK_UNIT;
	ast->rstmt.value = sem_engine(a, tmp, ctx, &pt->children[1]);

	return ast;
}

static astnode *syn_dstmt(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_S_DECL;
	ast->type = make(a, typenode, 1);
	ast->type->kind = TNK_UNIT;

	ast->dstmt.name = pt->children[2].label.sdata;
	ast->dstmt.type = typenode_make(a, tmp, ctx, &pt->children[1]);

	if (pt->rule == SYN_DSTMT_EXPR) {
		ast->dstmt.expr = sem_engine(a, tmp, ctx, &pt->children[4]);
	}

	symtbl_set(ctx->types, ast->dstmt.name, ast->dstmt.type);

	return ast;
}

static astnode *syn_cstmt(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_S_CALL;
	ast->type = make(a, typenode, 1);
	ast->type->kind = TNK_UNIT;

	ast->cstmt.fn_call = syn_prim_papp(a, tmp, ctx, &pt->children[1]);

	return ast;
}

static list *syn_slist(arena *a, arena tmp, semctx *ctx, ptnode *pt);

#define IS_BLOCK(x) (x == SYN_BLOCK_SLIST || x == SYN_BLOCK_SLIST_RET || x == SYN_BLOCK_RET)
static arr_ast *block_to_array(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	arr_ast *arr;

	if (pt->rule == SYN_BLOCK_RET) {
		arr = make(a, arr_ast, 1);
		arr->len = 1;
		arr->memory = make(a, astnode *, 1);
		arr->memory[0] = sem_engine(a, tmp, ctx, &pt->children[0]);
		return arr;
	}

	int count;
	if (IS_BLOCK(pt->children[0].rule)) {
		arr = block_to_array(a, tmp, ctx, &pt->children[0]);
	} else {
		count = pt->child_cnt > 1 ? 1 : 0; // room for return statement
		list *stmts = syn_slist(a, tmp, ctx, &pt->children[0]);
		list *cursor = stmts;
		while (cursor) {
			count++;
			cursor = cursor->next;
		}

		arr = make(a, arr_ast, 1);
		arr->len = count;
		arr->memory = make(a, astnode *, count);
		count = 0;
		cursor = stmts;
		while (cursor) {
			arr->memory[count++] = cursor->data;
			cursor = cursor->next;
		}

	}

	astnode *ret_stmt = pt->child_cnt > 1 
		? sem_engine(a, tmp, ctx, &pt->children[1])
		: NULL;
	if (ret_stmt != NULL) {
		arr->memory[count] = ret_stmt;
	}

	return arr;
}


static astnode *syn_wstmt(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_S_WHILE;
	ast->type = make(a, typenode, 1);
	ast->type->kind = TNK_UNIT;

	ast->wstmt.cond = sem_engine(a, tmp, ctx, &pt->children[1]);
	ast->wstmt.body = *block_to_array(a, tmp, ctx, &pt->children[3]);

	return ast;
}

static astnode *syn_istmt_if(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_S_IF;
	ast->type = make(a, typenode, 1);
	ast->type->kind = TNK_UNIT;

	ast->istmt.cond = sem_engine(a, tmp, ctx, &pt->children[1]);
	ast->istmt.branch_t = *block_to_array(a, tmp, ctx, &pt->children[3]);

	return ast;
}

static astnode *syn_istmt_else(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_S_IFELSE;
	ast->type = make(a, typenode, 1);
	ast->type->kind = TNK_UNIT;

	ast->istmt.cond = sem_engine(a, tmp, ctx, &pt->children[1]);
	ast->istmt.branch_t = *block_to_array(a, tmp, ctx, &pt->children[3]);
	ast->istmt.branch_f = *block_to_array(a, tmp, ctx, &pt->children[5]);

	return ast;
}

astnode *promote_type(arena *a, typenode *tn, astnode *ast)
{
	if (tn->kind & TNK_INTEGER 
		&& ast->type->kind & TNK_INTEGER
		&& tn->kind >= ast->type->kind
	) {
		return create_cast(a, tn, ast);
	} else if (tn->kind & TNK_FLOAT
		&& ast->type->kind & TNK_FLOAT
		&& tn->kind >= ast->type->kind
	) {
		return create_cast(a, tn, ast);
	} else if (tn->kind == TNK_PTR
		&& ast->type->kind & TNK_INTEGER
	) {
		return create_cast(a, tn, ast);
	}


	return NULL;
}

static astnode *syn_astmt(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_S_ASSIGN;
	ast->type = make(a, typenode, 1);
	ast->type->kind = TNK_UNIT;

	ast->astmt.lval = syn_lval(a, tmp, ctx, &pt->children[0]);
	ast->astmt.expr = sem_engine(a, tmp, ctx, &pt->children[2]);

	if (!typenode_eq(ast->astmt.expr->type, ast->astmt.lval->type)) {
		ast->astmt.expr = promote_type(a, ast->astmt.lval->type, ast->astmt.expr);
		if (!ast->astmt.expr) {
			FATAL_ASSIGN_BADTYPE(S("some lval idk man"));
		}
	}

	return ast;
}

static struct proc_def_arg *proc_def_arg_make(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	struct proc_def_arg *arg = make(a, struct proc_def_arg, 1);
	arg->name = pt->children[1].label.sdata;
	arg->type = typenode_make(a, tmp, ctx, &pt->children[0]);
	return arg;
}

static list *proc_arg_decls_read(arena *a, arena *tmp, semctx *ctx, ptnode *pt)
{
	list *node = make(tmp, list, 1);
	if (pt->rule == SYN_PADS_ROOT) {
		node->data = proc_def_arg_make(a, *tmp, ctx, &pt->children[0]);
		node->next = NULL;
		node->last = node;
		return node;
	}
	list *head = proc_arg_decls_read(a, tmp, ctx, &pt->children[0]);
	node->data = proc_def_arg_make(a, *tmp, ctx, &pt->children[2]);
	list_append(&head, node);
	return head;
}

static arr_arg proc_args_read(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	arr_arg arr;
	arr.len = 0;
	arr.memory = 0;
	if (pt->rule == SYN_PARGS_EMPTY) {
		return arr;
	}

	list *arg_list = proc_arg_decls_read(a, &tmp, ctx, &pt->children[1]);
	list *cursor = arg_list;
	int count = 0;
	while (cursor) {
		count = count + 1;
		cursor = cursor->next;
	}

	arr.len = count;
	arr.memory = make(a, struct proc_def_arg *, arr.len);
	count = 0;
	cursor = arg_list;
	while (cursor) {
		arr.memory[count++] = cursor->data;
		cursor = cursor->next;
	}

	return arr;
}

static list *syn_slist(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	if (pt->child_cnt == 1) {
		list *node = make(a, list, 1);
		node->next = NULL;
		node->last = node;
		node->data = sem_engine(a, tmp, ctx, &pt->children[0]);
		return node;
	}
	list *left = syn_slist(a, tmp, ctx, &pt->children[0]);
	list *right = syn_slist(a, tmp, ctx, &pt->children[1]);
	left->last->next = right;
	left->last = right->last;
	return left;
}

static astnode *syn_proc_imp(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_PROC_IMPORT;
	ast->type = make(a, typenode, 1);
	ast->type->kind = TNK_UNIT;

	struct ns *ns;
	eligible_id name = qualified_id_parse(pt->children[2].label.sdata);
	if (!dprh_tbl_get(&ctx->namspaces, name.ns, (void **)&ns)) {
		ns = ns_make(a, name.ns);
		dprh_tbl_set(&ctx->namspaces, name.ns, ns);
	}
	ast->proc_imp.name = name;

	typenode *fntype = make(a, typenode, 1);
	fntype->proc.args = proc_args_read(a, tmp, ctx, &pt->children[4]);
	fntype->proc.return_type = typenode_make(a, tmp, ctx, &pt->children[3]);

	ast->proc_imp.args = fntype->proc.args;
	ast->proc_imp.return_type = fntype->proc.return_type;

	dprh_tbl_set(&ns->proc, name.id, fntype);

	return ast;
}

static astnode *syn_proc_def(arena *a, arena tmp, semctx *ctx, ptnode *pt, int exported)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_PROC_DEF;
	ast->type = make(a, typenode, 1);
	ast->type->kind = TNK_UNIT;

	// does the name space exist? if not, create it
	struct ns *ns;
	string qualified_id = pt->children[1 + exported].label.sdata;
	eligible_id name = qualified_id_parse(qualified_id);
	if (!dprh_tbl_get(&ctx->namspaces, name.ns, (void **)&ns)) {
		ns = ns_make(a, name.ns);
		dprh_tbl_set(&ctx->namspaces, name.ns, ns);
	}
	string super_ns = ctx->ctx_ns; // technically always null i think
	ctx->ctx_ns = name.ns; // set this namespace as our contextual namespace

	// process argument list
	typenode *fntype = make(a, typenode, 1);
	fntype->proc.args = proc_args_read(a, tmp, ctx, &pt->children[3 + exported]);
	fntype->proc.return_type = typenode_make(a, tmp, ctx, &pt->children[2 + exported]);

	symtbl_scope_enter(ctx->types);
	for (int i = 0; i < fntype->proc.args.len; i++) {
		struct proc_def_arg *arg = fntype->proc.args.memory[i];
		symtbl_set(ctx->types, arg->name, arg->type);
	}

	ast->proc_def.name = name;
	ast->proc_def.exported = exported;
	ast->proc_def.args = fntype->proc.args;
	ast->proc_def.return_type = fntype->proc.return_type;

	arr_ast *stmts = block_to_array(a, tmp, ctx, &pt->children[4 + exported]);
	ast->proc_def.stmts = *stmts;
	ast->proc_def.implicit_ret = stmts->memory[stmts->len - 1]->kind != AST_S_RETURN;

	symtbl_scope_leave(ctx->types);

	dprh_tbl_set(&ns->proc, name.id, fntype);
	ctx->ctx_ns = super_ns;

	return ast;
}

static attr_arg_decl *syn_had(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	attr_arg_decl *decl = make(a, attr_arg_decl, 1);
	if (pt->rule == SYN_HAD_SINGLE) {
		decl->attr = eligible_id_make(&pt->children[0]);
		decl->var = S_NULL;
	} else if (pt->rule == SYN_HAD_DOUBLE) {
		decl->attr = eligible_id_make(&pt->children[0]);
		decl->var = pt->children[1].label.sdata;
	} else {
		FATAL_PLUMBING();
	}
	return decl;
}

static list *syn_hads(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	list *node = make(a, list, 1);
	if (pt->rule == SYN_HADS_ROOT) {
		node->data = syn_had(a, tmp, ctx, &pt->children[0]);
		node->next = NULL;
		node->last = node;
		return node;
	}
	list *head = syn_hads(a, tmp, ctx, &pt->children[0]);
	node->data = syn_had(a, tmp, ctx, &pt->children[2]);
	list_append(&head, node);
	return head;
}

static astnode *syn_hom_def(arena *a, arena tmp, semctx *ctx, ptnode *pt, int expr)
{
	attr_rule *attr = make(a, attr_rule, 1);

	attr->is_term = 0;
	attr->is_expr = expr;
	attr->name = qualified_id_parse(pt->children[1].label.sdata);
	attr->body = &pt->children[5];

	int argc = 0;
	list *args = syn_hads(a, tmp, ctx, &pt->children[3]);

	list *cursor = args;
	while (cursor) {
		argc++;
		cursor = cursor->next;
	}

	attr->ntrm.argc = argc;
	attr->ntrm.argv = make(a, attr_arg_decl *, argc);

	argc = 0;
	cursor = args;
	while (cursor) {
		attr->ntrm.argv[argc++] = cursor->data;
		cursor = cursor->next;
	}

	struct ns *ns;
	if (!dprh_tbl_get(&ctx->namspaces, attr->name.ns, (void **)&ns)) {
		ns = ns_make(a, attr->name.ns); // create namespace if it does not yet exist
		dprh_tbl_set(&ctx->namspaces, attr->name.ns, ns);
	}

	list *attrs = NULL;
	list *node = make(a, list, 1);
	node->data = attr;
	
	dprh_tbl_get(&ns->attr, attr->name.id, (void **)&attrs);
	list_append(&attrs, node);
	dprh_tbl_set(&ns->attr, attr->name.id, attrs);

	return NULL;
}

static struct tdat_struct_mem *syn_struct_mem(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	struct tdat_struct_mem *mem = make(a, struct tdat_struct_mem, 1);
	mem->name = pt->children[1].label.sdata;
	mem->type = typenode_make(a, tmp, ctx, &pt->children[0]);
	return mem;
}

static list *syn_struct_mem_list(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	list *node = make(a, list, 1);
	if (pt->rule == SYN_SMLIST_ROOT) {
		node->data = syn_struct_mem(a, tmp, ctx, &pt->children[0]);
		node->next = NULL;
		node->last = node;
		return node;
	}
	node->data = syn_struct_mem(a, tmp, ctx, &pt->children[1]);
	list *head = syn_struct_mem_list(a, tmp, ctx, &pt->children[0]);
	list_append(&head, node);
	return head;
}

static char *eligible_cat(char *buffer, eligible_id id)
{
	int cursor = 0;
	if (id.ns.addr && id.ns.len > 0) {
		for (int i = 0; i < id.ns.len; i++) {
			buffer[cursor++] = id.ns.addr[i];
		}
		buffer[cursor++] = '_';
	}
	for (int i = 0; i < id.id.len; i++) {
		buffer[cursor++] = id.id.addr[i];
	}
	buffer[cursor++] = '\0';
	return buffer;
}

static void attr_rule_close(arena *a, eligible_id name, pregrammar *pregram, semctx *ctx)
{
	list *rules;
	struct ns *ns;
	if (!dprh_tbl_get(&ctx->namspaces, name.ns, (void **)&ns)) {
		FATAL_BAD_NS(name.ns);
	}
	if (!dprh_tbl_get(&ns->attr, name.id, (void **)&rules)) {
		FATAL_BAD_RULE(name.id);
	}

	// check if we're already in
	void *some;
	string cat;
	cat.len = name.ns.len + name.id.len + 2;
	char *buffer = make(a, char, cat.len);
	cat.addr = buffer;
	eligible_cat(cat.addr, name);
	//printf("checking %.*s\n", cat.len, cat.addr);
	if (dprh_tbl_get(&pregram->rule_bag, cat, &some)) {
		//printf("found\n");
		return;
	}
	//printf("not found\n");
	
	list *cursor = rules;
	while (cursor) {
		attr_rule *attr = cursor->data;
		list *node = make(a, list, 1);
		node->data = attr;

		// check if terminal or nonterminal
		if (attr->is_term) {
			//printf("found term: %.*s\n", cat.len, cat.addr);
			pregram->tm_count++;
			list_append(&pregram->tm_rules, node);
			dprh_tbl_set(&pregram->rule_bag, cat, attr);
			return; // we don't alias terminals the same way as nonterminals
		}

		for (int i = 0; i < attr->ntrm.argc; i++) {
			attr_arg_decl *arg = attr->ntrm.argv[i];
			if (!streq(name.id, arg->attr.id) || !streq(name.ns, arg->attr.ns)) {
				attr_rule_close(a, arg->attr, pregram, ctx);
			}
		}

		// after processing arguments
		pregram->nt_count++;
		list_append(&pregram->nt_rules, node);
		dprh_tbl_set(&pregram->rule_bag, cat, attr);

		cursor = cursor->next;
	}
}

static pregrammar *pregrammar_make(arena *a, eligible_id name, semctx *ctx)
{
	pregrammar *pg = make(a, pregrammar, 1);

	pg->tm_count = 0;
	pg->tm_rules = NULL;
	pg->nt_count = 0;
	pg->nt_rules = NULL;
	pg->rule_bag = dprh_tbl_make(a, 6);
	pg->name_idx_tbl = dprh_tbl_make(a, 6);

	attr_rule_close(a, name, pg, ctx);
	
	return pg;
}

static scfe_grammar *grammar_make(arena *a, pregrammar *pg)
{
        scfe_grammar *G = make(a, scfe_grammar, 1);
        G->terminals.len = pg->tm_count;
        G->terminals.addr = make(a, string, pg->tm_count);
        G->nonterminals.len = pg->nt_count + 1;
        G->nonterminals.addr = make(a, scfe_pda_rule, pg->nt_count + 1);

        //dprh_tbl name_idx_tbl = dprh_tbl_make(a, 6);
        dprh_tbl name_idx_tbl = pg->name_idx_tbl;

        int i = 0;
        attr_rule *attr;
        list *cursor = pg->tm_rules;
        while (cursor) {
                attr = cursor->data;
                G->terminals.addr[i++] = attr->term.regex; // i is set +1 here

		string name;
		name.len = attr->name.ns.len + attr->name.id.len + 2;
		name.addr = make(a, char, name.len);
		eligible_cat(name.addr, attr->name);

                dprh_tbl_set(&name_idx_tbl, name, (void *)(long)i);
		//printf("++ setting %.*s as %d!\n", name.len, name.addr, i);
                cursor = cursor->next;
        }

        // this is the implicit OVER symbol
        int over_id = i++;
        int nt_id = i; // this is saved from when terminals ended, the first
                       // nonterminal "number" is the next after over, which is
                       // next after last terminal
        // this is the first nonterminal, it's the start symbol for the
        // augmented rule
        int start_id = nt_id++;

        i = 0;
        int index;
	int cur_id;
        cursor = pg->nt_rules;
        while (cursor) {
                attr = cursor->data;

		string name;
		name.len = attr->name.ns.len + attr->name.id.len + 2;
		name.addr = make(a, char, name.len);
		eligible_cat(name.addr, attr->name);

		if (!dprh_tbl_get(&name_idx_tbl, name, (void **)&cur_id)) {
			cur_id = nt_id++;
			//printf("xx setting %.*s as %d!\n", name.len, name.addr, cur_id);
		} else {
			cur_id--;
			//printf("xx found %.*s as %d!\n", name.len, name.addr, cur_id);
		}
                G->nonterminals.addr[i].head.type = cur_id;
                G->nonterminals.addr[i].body.len = attr->ntrm.argc;
                G->nonterminals.addr[i].body.addr = make(a, scfe_tkn, attr->ntrm.argc);
                for (int argi = 0; argi < attr->ntrm.argc; argi++) {
                        // what number represents this rule/type?
                        // (we have the name)
			string args;
			eligible_id arg = attr->ntrm.argv[argi]->attr;
			args.len = arg.ns.len + arg.id.len + 2;
			args.addr = make(a, char, args.len);
			eligible_cat(args.addr, arg);

                        dprh_tbl_get(&name_idx_tbl, args, (void **)&index);
                        if (index == 0) {
				FATAL_LEGACY_ZI(args);
                        }
                        G->nonterminals.addr[i].body.addr[argi].type = index - 1;
                }
                dprh_tbl_set(&name_idx_tbl, name, (void *)(long)cur_id + 1);
		//printf("saved nonterminal %.*s as %d!\n", name.len, name.addr, cur_id);
                cursor = cursor->next;
                i++;
        }

        // augment here!
        G->nonterminals.addr[i].head.type = start_id;
        G->nonterminals.addr[i].body.len = 2;
        G->nonterminals.addr[i].body.addr = make(a, scfe_tkn, 2);
        G->nonterminals.addr[i].body.addr[0].type = G->nonterminals.addr[i - 1].head.type;
        G->nonterminals.addr[i].body.addr[1].type = over_id;

        return G;
}

static ptnode *pt1_from_int(arena *a, string sdat)
{
        ptnode *node = make(a, ptnode, 1);
        node->rule = SYN_PRIM_INT;
        node->label.type = -22;
        node->label.sdata = S_NULL;
        node->child_cnt = 1;
        node->children = make(a, ptnode, 1);
        node->children[0].rule = -1;
        node->children[0].label.type = TTOK_LIT_INT;
        node->children[0].label.sdata = sdat;
        node->children[0].child_cnt = 0;
        node->children[0].children = 0;
        return node;
}

static ptnode *pt1_from_float(arena *a, string sdat)
{
        ptnode *node = make(a, ptnode, 1);
        node->rule = SYN_PRIM_FLOAT;
        node->label.type = -22;
        node->label.sdata = S_NULL;
        node->child_cnt = 1;
        node->children = make(a, ptnode, 1);
        node->children[0].rule = -1;
        node->children[0].label.type = TTOK_LIT_FLOAT;
        node->children[0].label.sdata = sdat;
        node->children[0].child_cnt = 0;
        node->children[0].children = 0;
        return node;
}

static ptnode *pt1_from_string(arena *a, string sdat)
{
        ptnode *node = make(a, ptnode, 1);
        node->rule = SYN_EXPR_STRING;
        node->label.type = -47;
        node->label.sdata = S_NULL;
        node->child_cnt = 1;
        node->children = make(a, ptnode, 1);
        node->children[0].rule = -1;
        node->children[0].label.type = TTOK_LIT_STRING;
        node->children[0].child_cnt = 0;
        node->children[0].children = 0;
	node->children[0].label.sdata.len = sdat.len;
	node->children[0].label.sdata.addr = make(a, char, sdat.len);
	for (int i = 0; i < sdat.len; i++) {
		node->children[0].label.sdata.addr[i] = sdat.addr[i];
	}
	//printf("made node from '%.*s'\n", sdat.len, sdat.addr);
        return node;
}

static void debug_pt(int lvl, ptnode *pt)
{
	if (lvl > 200) {
		FATAL_GENERIC_UNSUPPORTED("big pt");
	}
	printf("%i: rule %i (%p)\n", lvl, pt->rule, pt);
	for (int i = 0;  i < pt->child_cnt; i++) {
		debug_pt(lvl + 1, &pt->children[i]);
		printf(",\n");
	}
}

static ptnode *pt_copy(arena *a, ptnode *node)
{
	ptnode *clone = make(a, ptnode, 1);
	clone->rule = node->rule;
	clone->child_cnt = node->child_cnt;
	clone->label = node->label;
	clone->children = make(a, ptnode, clone->child_cnt);
	for (int i = 0; i < clone->child_cnt; i++) {
		clone->children[i] = *pt_copy(a, &node->children[i]);
	}
	return clone;
}

static attr_rule *attr_copy(arena *a, attr_rule *attr)
{
	attr_rule *clone = make(a, attr_rule, 1);

	clone->body = pt_copy(a, attr->body);
	clone->is_expr = attr->is_expr;
	clone->is_term = attr->is_term;
	clone->name = attr->name;
	clone->term = attr->term;
	clone->ntrm = attr->ntrm;

	return clone;
}
	
static int is_std_hexcode(eligible_id var)
{
       if (!streq(var.ns, S("std"))) {
               return 0;
       }
       if (var.id.len == 4 && var.id.addr[0] == '0' && var.id.addr[1] == 'x') {
               return 1;
       }
       return 0;
}


static ptnode *synthesis(arena *a, ptnode *pt0, ptnode *pt1, semctx *ctx, attr_rule **rules)
{
	switch (pt1->rule) {
	case SYN_PRIM_SEAM: {
		int idx = -1;
		eligible_id rule;
		string invoking = pt1->children[1].label.sdata;
		for (int i = 0; i < rules[pt0->rule]->ntrm.argc; i++) {
			if (streq(invoking, rules[pt0->rule]->ntrm.argv[i]->var)) {
				/*
				printf("found expr '%.*s' as '%.*s:%.*s'\n", 
					pt1->children[1].label.sdata.len, pt1->children[1].label.sdata.addr,
					rules[pt0->rule]->ntrm.argv[i]->attr.ns.len,
					rules[pt0->rule]->ntrm.argv[i]->attr.ns.addr,
					rules[pt0->rule]->ntrm.argv[i]->attr.id.len,
					rules[pt0->rule]->ntrm.argv[i]->attr.id.addr
					);
				*/
				rule = rules[pt0->rule]->ntrm.argv[i]->attr;
				idx = i;
				break;
			}
		}
		if (idx < 0) {
			printf("couldnt find '%.*s'\n", invoking.len, invoking.addr);
			FATAL_GENERIC_UNSUPPORTED("negative idx 0");
		}
		struct ns *ns;
		list *argrules;
		if (!dprh_tbl_get(&ctx->namspaces, rule.ns, (void **)&ns)) {
			FATAL_BAD_NS(rule.ns);
		}
		if (!dprh_tbl_get(&ns->attr, rule.id, (void **)&argrules)) {
			FATAL_BAD_RULE(rule.id);
		}

		attr_rule *argrule = argrules->data;
		if (!argrule->is_term) {
			ptnode *child = &pt0->children[idx];
			ptnode *tile = pt_copy(a, rules[child->rule]->body);
			return synthesis(a, child, tile, ctx, rules);
		} else if (streq(argrule->name.id, S("lit_int"))) {
			return pt1_from_int(a, pt0->children[idx].label.sdata);
		} else if (streq(argrule->name.id, S("lit_float"))) {
			return pt1_from_float(a, pt0->children[idx].label.sdata);
		} else if (streq(argrule->name.id, S("lit_string"))) {
			return pt1_from_string(a, pt0->children[idx].label.sdata);
		} else if (streq(argrule->name.id, S("alphanumeric"))) {
			return pt1_from_string(a, pt0->children[idx].label.sdata);
		} else if (streq(argrule->name.id, S("punctuation"))) {
			return pt1_from_string(a, pt0->children[idx].label.sdata);
		} else if (streq(argrule->name.id, S("glob"))) {
			//printf("processing glob '%.*s'\n", pt0->children[idx].label.sdata.len, pt0->children[idx].label.sdata.addr);
			return pt1_from_string(a, pt0->children[idx].label.sdata);
		} else if (is_std_hexcode(argrule->name)) {
			return pt1_from_string(a, pt0->children[idx].label.sdata);
		}
		FATAL_GENERIC_UNSUPPORTED("missing transition for terminal");
	}
	case SYN_STMT_ESTMT: {
		//printf("found stmt '%.*s'\n", pt1->children[2].label.sdata.len, pt1->children[2].label.sdata.addr);
		int idx = -1;
		string invoking = pt1->children[2].label.sdata;
		for (int i = 0; i < rules[pt0->rule]->ntrm.argc; i++) {
			if (streq(invoking, rules[pt0->rule]->ntrm.argv[i]->var)) {
				idx = i;
				break;
			}
		}
		if (idx < 0) {
			//printf("%.*s\n", invoking.len, invoking.addr);
			FATAL_GENERIC_UNSUPPORTED("negative idx 1");
		}

		// do the synthesis and then decide
		ptnode *child = &pt0->children[idx];
		ptnode *bstmt = make(a, ptnode, 1);
		ptnode *tile = pt_copy(a, rules[child->rule]->body);
		ptnode *product = synthesis(a, child, tile, ctx, rules);
		bstmt->rule = SYN_STMT_BSTMT;
		bstmt->child_cnt = 1;
		bstmt->children = make(a, ptnode, 1);
		bstmt->children[0].rule = SYN_BSTMT_BLOCK;
		bstmt->children[0].child_cnt = 3;
		bstmt->children[0].children = make(a, ptnode, 3);
		bstmt->children[0].children[0].rule = -1;
		bstmt->children[0].children[0].label.type = TTOK_KW_DO;

		bstmt->children[0].children[1] = *product;

		bstmt->children[0].children[2].rule = -1;
		bstmt->children[0].children[2].label.type = TTOK_KW_END;

		return bstmt;
	}
	default: {
		for (int i = 0; i < pt1->child_cnt; i++) {
			pt1->children[i] = *synthesis(a, pt0, &pt1->children[i], ctx, rules);
		}
		if (pt1->child_cnt == 1 
			&& pt1->rule != SYN_ELIST_ROOT
			&& (pt1->children[0].rule == SYN_EXPR_STRING || pt1->children[0].rule == SYN_EXPR_CAT)) {
			return pt1->children;
		}
		return pt1;
	}
	}
}


static astnode *ast_block_make(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *block = make(a, astnode, 1);
	block->kind = AST_S_BLOCK;
	block->type = make(a, typenode, 1);
	block->type->kind = TNK_UNIT;
	block->bstmt.body = *block_to_array(a, tmp, ctx, pt);
	return block;
}

static list *syn_weaves(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	eligible_id *id;
	list *node = make(a, list, 1);
	if (pt->rule == SYN_WLIST_ROOT) {
		id = make(a, eligible_id, 1);
		*id = eligible_id_make(&pt->children[0].children[1]);
		node->data = id;
		node->next = NULL;
		node->last = node;
		return node;
	}

	list *head = syn_weaves(a, tmp, ctx, &pt->children[0]);
	id = make(a, eligible_id, 1);
	*id = eligible_id_make(&pt->children[1].children[1]);
	node->data = id;
	list_append(&head, node);

	return head;
}

static void weave_lists(arena *a, arena tmp, semctx *ctx, 
	pregrammar *pg, attr_rule **rules, list *src_list, list *dst_list, advice_verb verb)
{
	if (src_list->next || dst_list->next) {
		FATAL_GENERIC_UNSUPPORTED("weaving with non singletons");
	}
	attr_rule *src = src_list->data;
	attr_rule *dst = dst_list->data;
	long dst_idx;
	string name;
	name.len = dst->name.ns.len + dst->name.id.len + 2;
	name.addr = make(a, char, name.len);
	eligible_cat(name.addr, dst->name);
	//printf("trying %.*s\n", name.len, name.addr);
	if (!dprh_tbl_get(&pg->name_idx_tbl, name, (void **)&dst_idx)) {
		FATAL_GENERIC_UNSUPPORTED("corrupted name_idx_tbl?");
	}
	dst_idx -= 1; // offset
	dst_idx -= 2; // start and over tokens
	dst_idx -= pg->tm_count; // terminal tokens
	//printf("got %i\n", rules[dst_idx]->body->label.type);
	if (rules[dst_idx]->body->label.type != NTOK_BLOCK) {
		FATAL_GENERIC_UNSUPPORTED("weaving into expression attrs");
	}
	if (src->body->label.type != NTOK_BLOCK) {
		FATAL_GENERIC_UNSUPPORTED("weaving with expression attrs");
	}

	ptnode *source = pt_copy(a, src->body);
	ptnode *target = rules[dst_idx]->body;
	switch(target->rule) {
	case SYN_BLOCK_SLIST:
		if (source->rule == SYN_BLOCK_SLIST) {
			ptnode *root = make(a, ptnode, 1);
			root->rule = SYN_BLOCK_SLIST;
			root->label.type = NTOK_BLOCK;
			root->child_cnt = 1;
			root->children = make(a, ptnode, 1);
			root->children[0].rule = SYN_SLIST_DOUBLE;
			root->children[0].child_cnt = 2;
			root->children[0].children = make(a, ptnode, 2);
			if (verb == ADV_BEFORE) {
				root->children[0].children[0] = source->children[0];
				root->children[0].children[1] = target->children[0];
			} else if (verb == ADV_AFTER) {
				root->children[0].children[0] = target->children[0];
				root->children[0].children[1] = source->children[0];
			}
			//printf("setting %p <- %p\n", rules[dst_idx]->body, root);
			rules[dst_idx]->body = root;
		} else if (source->rule == SYN_BLOCK_RET) {
			if (verb == ADV_BEFORE) {
				rules[dst_idx]->body = pt_copy(a, source);
			}
			ptnode *root = make(a, ptnode, 1);
			root->rule = SYN_BLOCK_SLIST_RET;
			root->label.type = NTOK_BLOCK;
			root->child_cnt = 2;
			root->children = make(a, ptnode, 2);
			root->children[0] = target->children[0];
			root->children[1] = source->children[0];
			rules[dst_idx]->body = root;
		} else if (source->rule == SYN_BLOCK_SLIST_RET) {
			FATAL_GENERIC_UNSUPPORTED("cannot after return");
		} else {
			printf("%i\n", source->rule);
			FATAL_GENERIC_UNSUPPORTED("weird source->rule");
		}
		break;
	case SYN_BLOCK_RET:
		FATAL_GENERIC_UNSUPPORTED("cannot after return");
		break;
	case SYN_BLOCK_SLIST_RET:
		FATAL_GENERIC_UNSUPPORTED("not written yet ;)");
		break;
	}
}

static void weave_aspect(arena *a, arena tmp, semctx *ctx,
	pregrammar *pg, attr_rule **rules, eligible_id *aspect_name)
{
	string aspect_ns = aspect_name->ns;
	if (aspect_ns.len < 1 || !aspect_ns.addr) {
		aspect_ns = ctx->ctx_ns;
	}

	struct ns *ns;
	aspect_t *aspect;
	if (!dprh_tbl_get(&ctx->namspaces, aspect_ns, (void **)&ns)) {
		FATAL_BAD_NS(aspect_ns);
	}
	if (!dprh_tbl_get(&ns->aspct, aspect_name->id, (void **)&aspect)) {
		FATAL_BAD_ASPECT(aspect_name->id);
	}
	for (int i = 0; i < aspect->len; i++) {
		advice_t *advice = aspect->addr[i];
		string ns_name = advice->src.ns;
		if (ns_name.len < 0 || !ns_name.addr) {
			ns_name = ctx->ctx_ns;
		}
		if (!dprh_tbl_get(&ctx->namspaces, ns_name, (void **)&ns)) {
			FATAL_BAD_NS(ns_name);
		}
		list *src_list;
		if (!dprh_tbl_get(&ns->attr, advice->src.id, (void **)&src_list)) {
			FATAL_BAD_RULE(advice->src.id);
		}
		printf("%.*s:%.*s: has rule %i\n",
			advice->src.ns.len, advice->src.ns.addr,
			advice->src.id.len, advice->src.id.addr,
			((attr_rule *)src_list->data)->body->rule);
		ns_name = advice->dest.ns;
		if (ns_name.len < 0 || !ns_name.addr) {
			ns_name = ctx->ctx_ns;
		}
		if (!dprh_tbl_get(&ctx->namspaces, ns_name, (void **)&ns)) {
			FATAL_BAD_NS(ns_name);
		}
		list *dst_list;
		if (!dprh_tbl_get(&ns->attr, advice->dest.id, (void **)&dst_list)) {
			FATAL_BAD_RULE(advice->dest.id);
		}
		// cats tiles from ctx onto tiles in rules
		weave_lists(a, tmp, ctx, pg, rules, src_list, dst_list, advice->verb);
	}
}

static astnode *syn_prim_happ_weave(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	eligible_id name = eligible_id_make(&pt->children[0]);
	string s0 = pt->children[3].label.sdata;
	s0.addr += 1;
	s0.len -= 2;

	pregrammar *pg = pregrammar_make(a, name, ctx);

	int i = 0;
	list *cursor = pg->nt_rules;
	attr_rule *rules[pg->nt_count];
	while (cursor) {
		rules[i++] = attr_copy(a, cursor->data);
		//printf("%i::: %.*s\n", i - 1, rules[i - 1]->name.id.len, rules[i-1]->name.id.addr);
		cursor = cursor->next;
	}

	scfe_grammar *g0 = grammar_make(a, pg);

	string super_ns = ctx->ctx_ns;
	ctx->ctx_ns = name.ns;

	list *aspects = syn_weaves(a, tmp, ctx, &pt->children[1]);
	list *acursor = aspects;
	eligible_id *id;
	while (acursor) {
		id = acursor->data;
		weave_aspect(a, tmp, ctx, pg, rules, id);
		acursor = acursor->next;
	}

	ctx->ctx_ns = super_ns;

	ptnode *pt0 = scfe_parse_tree(a, tmp, g0, s0);
	ptnode *tile = pt_copy(a, rules[pt0->rule]->body);
	ptnode *pt1 = synthesis(a, pt0, tile, ctx, rules);

	if (rules[pt0->rule]->is_expr) {
		return sem_engine(a, tmp, ctx, pt1);
	}

	return ast_block_make(a, tmp, ctx, pt1);
}
static astnode *syn_prim_happ(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	if (pt->rule == SYN_WEAVE_APP) {
		return syn_prim_happ_weave(a, tmp, ctx, pt);
	}

	// use rules created by hom_defs to construct grammars
	// parse argument to this application with that grammar
	// return the results of calling sem_engine on that pt from this function
	eligible_id name = eligible_id_make(&pt->children[0]);
	string s0 = pt->children[2].label.sdata;
	s0.addr += 1;
	s0.len -= 2;

	pregrammar *pg = pregrammar_make(a, name, ctx);

	int i = 0;
	list *cursor = pg->nt_rules;
	attr_rule *rules[pg->nt_count];
	while (cursor) {
		rules[i++] = cursor->data;
		cursor = cursor->next;
	}

	scfe_grammar *g0 = grammar_make(a, pg);
	ptnode *pt0 = scfe_parse_tree(a, tmp, g0, s0);
	ptnode *tile = pt_copy(a, rules[pt0->rule]->body);
	ptnode *pt1 = synthesis(a, pt0, tile, ctx, rules);

	if (rules[pt0->rule]->is_expr) {
		return sem_engine(a, tmp, ctx, pt1);
	}

	return ast_block_make(a, tmp, ctx, pt1);
}

static astnode *syn_bstmt_block(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	return ast_block_make(a, tmp, ctx, &pt->children[1]);
}

static astnode *syn_struct_def(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_STRUCT_DEF;
	ast->type = make(a, typenode, 1);
	ast->type->kind = TNK_STRUCT;

	ast->strct.name = qualified_id_parse(pt->children[1].label.sdata);
	ast->type->strct.name = ast->strct.name;

	struct ns *ns;
	if (!dprh_tbl_get(&ctx->namspaces, ast->strct.name.ns, (void **)&ns)) {
		ns = ns_make(a, ast->strct.name.ns); // create namespace if it does not yet exist
		dprh_tbl_set(&ctx->namspaces, ast->strct.name.ns, ns);
	}
	dprh_tbl_set(&ns->strct, ast->strct.name.id, ast->type);
	string super_ns = ctx->ctx_ns;
	ctx->ctx_ns = ast->strct.name.ns;

	// process in context of namespace
	int count = 0;
	list *mems = syn_struct_mem_list(a, tmp, ctx, &pt->children[2]);
	list *cursor = mems;
	while (cursor) {
		count++;
		cursor = cursor->next;
	}
	ast->type->strct.members.len = count;
	ast->type->strct.members.memory = make(a, struct tdat_struct_mem *, count);

	count = 0;
	cursor = mems;
	while (cursor) {
		ast->type->strct.members.memory[count++] = cursor->data;
		cursor = cursor->next;
	}

	ctx->ctx_ns = super_ns;

	return ast;
}

static advice_t *advice_make(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	advice_t *advice = make(a, advice_t, 1);

	advice->src = eligible_id_make(&pt->children[0]);
	advice->dest = eligible_id_make(&pt->children[2]);
	switch (pt->rule) {
	case SYN_ADVICE_BEFORE:
		advice->verb = ADV_BEFORE;
		break;
	case SYN_ADVICE_AFTER:
		advice->verb = ADV_AFTER;
		break;
	}

	return advice;
}

static list *syn_advice_list(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	list *node = make(a, list, 1);
	if (pt->rule == SYN_ALIST_ROOT) {
		node->data = advice_make(a, tmp, ctx, &pt->children[0]);
		node->last = node;
		node->next = NULL;
		return node;
	}
	list *head = syn_advice_list(a, tmp, ctx, &pt->children[0]);
	node->data = advice_make(a, tmp, ctx, &pt->children[1]);
	list_append(&head, node);

	return head;
}

static astnode *syn_aspect_def(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	aspect_t *aspect = make(a, aspect_t, 1);
	aspect->name = qualified_id_parse(pt->children[1].label.sdata);
	list *advice = syn_advice_list(a, tmp, ctx, &pt->children[2]);
	list *cursor = advice;
	int count = 0;
	while (cursor) {
		count++;
		cursor = cursor->next;
	}
	aspect->len = count;
	aspect->addr = make(a, advice_t *, aspect->len);

	count = 0;
	cursor = advice;
	while (cursor) {
		aspect->addr[count++] = cursor->data;
		cursor = cursor->next;
	}

	struct ns *ns;
	if (!dprh_tbl_get(&ctx->namspaces, aspect->name.ns, (void **)&ns)) {
		ns = ns_make(a, aspect->name.ns); // create namespace if it does not yet exist
		dprh_tbl_set(&ctx->namspaces, aspect->name.ns, ns);
	}
	dprh_tbl_set(&ns->aspct, aspect->name.id, aspect);

	return NULL;
}

static astnode *syn_using_id(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	// bring the namespace into the default namespace
	struct ns *ns;
	string name = pt->children[1].label.sdata;
	if (!dprh_tbl_get(&ctx->namspaces, name, (void **)&ns)) {
		FATAL_BAD_NS(name);
	}

	list *node = make(a, list, 1);
	node->data = ns;
	list_append(&ctx->df_ns, node);

	return NULL;
}

static list *chunk_to_list(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	if (pt->rule != SYN_CHUNK_DOUBLE) {
		list *node = make(a, list, 1);
		node->data = sem_engine(a, tmp, ctx, pt);
		node->last = node;
		node->next = NULL;
		return node;
	}

	list *aux;
	list *left = chunk_to_list(a, tmp, ctx, &pt->children[0]);
	list *right = chunk_to_list(a, tmp, ctx, &pt->children[1]);
	switch ((((left->data != NULL) << 1) | (right->data != NULL))) {
	case 0:
		aux = make(a, list, 1);
		aux->data = NULL;
		return aux;
	case 1:
		return right;
	case 2:
		return left;
	case 3:
		left->last->next = right;
		left->last = right->last;

		return left;
	}
	FATAL_BAD_CHUNK();
}

static astnode *syn_chunk_dbl(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	astnode *ast = make(a, astnode, 1);

	ast->kind = AST_MODULE;
	ast->type = make(a, typenode, 1);
	ast->type->kind = TNK_UNIT;

	int count = 0;
	list *stmts = chunk_to_list(a, tmp, ctx, pt);
	list *cursor = stmts;
	while (cursor) {
		count++;
		cursor = cursor->next;
	}
	ast->module.stmts.len = count;
	ast->module.stmts.memory = make(a, astnode *, count);

	count = 0;
	cursor = stmts;
	while (cursor) {
		ast->module.stmts.memory[count++] = cursor->data;
		cursor = cursor->next;
	}

	return ast;
}

astnode *sem_engine(arena *a, arena tmp, semctx *ctx, ptnode *pt)
{
	switch (pt->rule) {
	case SYN_PRIM_GROUP:
		return sem_engine(a, tmp, ctx, &pt->children[1]);
	case SYN_PRIM_INT:
		return syn_prim_int(a, tmp, ctx, pt);
	case SYN_PRIM_FLOAT:
		return syn_prim_float(a, tmp, ctx, pt);
	case SYN_PRIM_PAPP:
		return syn_prim_papp(a, tmp, ctx, &pt->children[0]);
	case SYN_VAR_ID:
		return syn_var_id(a, tmp, ctx, pt);
	case SYN_VAR_MEMACC:
		return syn_var_memacc(a, tmp, ctx, pt);
	case SYN_VAR_INDEX:
		return syn_var_index(a, tmp, ctx, pt);
	case SYN_VAR_DEREF_MEMACC:
		return syn_var_deref_memacc(a, tmp, ctx, pt);
	case SYN_PRIM_VAR:
		return sem_engine(a, tmp, ctx, &pt->children[0]);
	case SYN_PRIM_DEREF:
		return syn_prim_deref(a, tmp, ctx, pt);

	case SYN_SECN_PRIM:
		return sem_engine(a, tmp, ctx, &pt->children[0]);
	case SYN_SECN_MUL:
		return syn_bin_op(a, tmp, "mul", AST_E_MUL, ctx, pt);
	case SYN_SECN_DIV:
		return syn_bin_op(a, tmp, "div", AST_E_DIV, ctx, pt);

	case SYN_TERT_SECN:
		return sem_engine(a, tmp, ctx, &pt->children[0]);
	case SYN_TERT_ADD:
		return syn_bin_op(a, tmp, "add", AST_E_ADD, ctx, pt);
	case SYN_TERT_SUB:
		return syn_bin_op(a, tmp, "sub", AST_E_SUB, ctx, pt);
	case SYN_TERT_MOD:
		return syn_bin_op(a, tmp, "mod", AST_E_MOD, ctx, pt);

	case SYN_EXPR_TERT:
		return sem_engine(a, tmp, ctx, &pt->children[0]);
	case SYN_EXPR_LT:
		return syn_expr_lt(a, tmp, ctx, pt);
	case SYN_EXPR_LTE:
		return syn_expr_lte(a, tmp, ctx, pt);
	case SYN_EXPR_GT:
		return syn_expr_gt(a, tmp, ctx, pt);
	case SYN_EXPR_GTE:
		return syn_expr_gte(a, tmp, ctx, pt);
	case SYN_EXPR_EQ:
		return syn_expr_eq(a, tmp, ctx, pt);
	case SYN_EXPR_NEQ:
		return syn_expr_neq(a, tmp, ctx, pt);
	case SYN_EXPR_STRING:
		return syn_expr_string(a, tmp, ctx, pt);
	case SYN_EXPR_CAT:
		return syn_expr_cat(a, tmp, ctx, pt);
	case SYN_EXPR_REF:
		return syn_expr_ref(a, tmp, ctx, pt);
	case SYN_EXPR_CAST:
		return syn_expr_cast(a, tmp, ctx, &pt->children[0]);
	
	case SYN_RSTMT_EXPR:
		return syn_rstmt_expr(a, tmp, ctx, pt);
	case SYN_STMT_DSTMT:
		return sem_engine(a, tmp, ctx, &pt->children[0]);
	case SYN_DSTMT_EXPR:
	case SYN_DSTMT_ID:
		return syn_dstmt(a, tmp, ctx, pt);
	case SYN_STMT_ASTMT:
		return sem_engine(a, tmp, ctx, &pt->children[0]);
	case SYN_ASTMT_EXPR:
		return syn_astmt(a, tmp, ctx, pt);
	case SYN_STMT_CSTMT:
		return sem_engine(a, tmp, ctx, &pt->children[0]);
	case SYN_CSTMT_EXPR:
		return syn_cstmt(a, tmp, ctx, pt);
	case SYN_STMT_WSTMT:
		return sem_engine(a, tmp, ctx, &pt->children[0]);
	case SYN_WSTMT_EXPR:
		return syn_wstmt(a, tmp, ctx, pt);
	case SYN_ISTMT_IF:
		return syn_istmt_if(a, tmp, ctx, pt);
	case SYN_ISTMT_ELSE:
		return syn_istmt_else(a, tmp, ctx, pt);
	case SYN_STMT_ISTMT:
		return sem_engine(a, tmp, ctx, &pt->children[0]);
	case SYN_SSTMT_EXPR:
		return syn_prim_happ(a, tmp, ctx, &pt->children[1]);
	case SYN_STMT_SSTMT:
		return sem_engine(a, tmp, ctx, &pt->children[0]);
	case SYN_BSTMT_BLOCK:
		return syn_bstmt_block(a, tmp, ctx, pt);
	case SYN_STMT_BSTMT:
		return sem_engine(a, tmp, ctx, &pt->children[0]);

	case SYN_PROC_IMP:
		return syn_proc_imp(a, tmp, ctx, pt);

	case SYN_PROC_EDEF:
		return syn_proc_def(a, tmp, ctx, pt, 1);
	case SYN_PROC_SDEF:
		return syn_proc_def(a, tmp, ctx, pt, 0);

	case SYN_HOM_DEF_EXPR:
		return syn_hom_def(a, tmp, ctx, pt, 1);
	case SYN_HOM_DEF_STMT:
		return syn_hom_def(a, tmp, ctx, pt, 0);
	
	case SYN_PRIM_HAPP:
		return syn_prim_happ(a, tmp, ctx, &pt->children[0]);

	case SYN_STRUCT_DEF:
		return syn_struct_def(a, tmp, ctx, pt);
	
	case SYN_ASPECT_DEF:
		return syn_aspect_def(a, tmp, ctx, pt);

	case SYN_USING_ID:
		return syn_using_id(a, tmp, ctx, pt);

	case SYN_CHUNK_USING:
		return sem_engine(a, tmp, ctx, &pt->children[0]);
	case SYN_CHUNK_IMP:
		return sem_engine(a, tmp, ctx, &pt->children[0]);
	case SYN_CHUNK_SDEF:
		return sem_engine(a, tmp, ctx, &pt->children[0]);
	case SYN_CHUNK_PDEF:
		return sem_engine(a, tmp, ctx, &pt->children[0]);
	case SYN_CHUNK_HDEF:
		return sem_engine(a, tmp, ctx, &pt->children[0]);
	case SYN_CHUNK_ADEF:
		return sem_engine(a, tmp, ctx, &pt->children[0]);
	case SYN_CHUNK_DOUBLE:
		return syn_chunk_dbl(a, tmp, ctx, pt);

	default:
		FATAL_UNDEF_SYN_RULE(pt->rule);
	}
	return NULL;
}

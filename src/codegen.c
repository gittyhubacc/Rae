#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/Transforms/PassBuilder.h>
#include <string.h>
#include "codegen.h"
#include "errfmt.h"

cgctx *cgctx_make(arena *a, cgopt opt)
{
	cgctx *ctx = make(a, cgctx, 1);
	ctx->opt = opt;
	ctx->udtypes = dprh_tbl_make(a, 5);

        LLVMInitializeAllTargetInfos();
        LLVMInitializeAllTargets();
        LLVMInitializeAllTargetMCs();
        LLVMInitializeAllAsmPrinters();

        char *errmsg;
        LLVMTargetRef target;
	char *target_triple;
	switch (opt.target) {
	case LELNUX64:
		target_triple = LLVMGetDefaultTargetTriple();
		break;
	case WASM32:
		target_triple = "wasm32-unknown-unknown";
		break;
	}
        if (LLVMGetTargetFromTriple(target_triple, &target, &errmsg) != 0) {
		FATAL_LLVM_ERR(errmsg);
        }

        ctx->target = LLVMCreateTargetMachine(
		target, target_triple, 
		"generic", "", 
		LLVMCodeGenLevelDefault,
		LLVMRelocDefault, 
		LLVMCodeModelDefault);
        LLVMTargetDataRef layout = LLVMCreateTargetDataLayout(ctx->target);
        char *data_layout = LLVMCopyStringRepOfTargetData(layout);

        symtbl_init(a, &ctx->symtbl, 6);
        ctx->llvm = LLVMContextCreate();
        ctx->module = LLVMModuleCreateWithNameInContext("obj_rae", ctx->llvm);
        LLVMSetTarget(ctx->module, target_triple);
        LLVMSetDataLayout(ctx->module, data_layout);
        LLVMDisposeMessage(data_layout);
        ctx->builder = LLVMCreateBuilderInContext(ctx->llvm);

	return ctx;
}

static void *_codegen(arena *a, arena tmp, astnode *ast, cgctx *ctx);

static char *eligible_c_str(char *buffer, eligible_id id)
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

static LLVMTypeRef typeref_make(arena *a, cgctx *ctx, typenode *tn)
{
	switch (tn->kind) {
	case TNK_UNIT:
		return LLVMVoidTypeInContext(ctx->llvm);
	case TNK_i8:
		return LLVMInt8TypeInContext(ctx->llvm);
	case TNK_i32:
		return LLVMInt32TypeInContext(ctx->llvm);
	case TNK_i64:
		return LLVMInt64TypeInContext(ctx->llvm);
	case TNK_f32:
		return LLVMFloatTypeInContext(ctx->llvm);
	case TNK_f64:
		return LLVMDoubleTypeInContext(ctx->llvm);
	case TNK_PTR: 
		return LLVMPointerTypeInContext(ctx->llvm, 0);
	case TNK_STRUCT: {
		char c_str[tn->strct.name.ns.len + tn->strct.name.id.len + 1];
		eligible_c_str(c_str, tn->strct.name);
		string combo = string_mask(a, c_str);
		LLVMTypeRef type;

		if (!dprh_tbl_get(&ctx->udtypes, combo, (void **)&type)) {
			FATAL_UNDEF_TYPE(combo);
		}

		return type;
	}
	case TNK_ARRAY: {
		LLVMTypeRef type = typeref_make(a, ctx, tn->array.subject);
		return LLVMArrayType2(type, tn->array.count);
	}
	default:
		FATAL_BAD_TYPE("codegen", tn->kind);
	}
}

static void *ast_e_int(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	LLVMTypeRef type = typeref_make(a, ctx, ast->type);
	long value = ast->type->kind == TNK_i32 
		? ast->i32.value 
		: ast->i64.value;
	return LLVMConstInt(type, value, 0);
}

static void *ast_e_float(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	LLVMTypeRef type = typeref_make(a, ctx, ast->type);
	double value = ast->type->kind == TNK_f32 
		? ast->f32.value 
		: ast->f64.value;
	return LLVMConstReal(type, value);
}

static void *ast_e_call(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	string ns = ast->call.name.ns;
	int argc = ast->call.arguments.len;
	char c_str[ast->call.name.id.len + ast->call.name.ns.len + 1];
	eligible_c_str(c_str, ast->call.name);

	LLVMValueRef argv[argc];
	for (int i = 0; i < argc; i++) {
		argv[i] = _codegen(a, tmp, ast->call.arguments.memory[i], ctx);
	}

	LLVMValueRef fn = LLVMGetNamedFunction(ctx->module, c_str);
	LLVMTypeRef type = LLVMGlobalGetValueType(fn);
	char *name = (LLVMGetTypeKind(type) == LLVMVoidTypeKind) ? "call" : "";
	return LLVMBuildCall2(ctx->builder, type, fn, argv, argc, name);
}

static void *ast_e_mul(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	LLVMValueRef left;
	LLVMValueRef right;
	astnode *ast_tmp;
	if (!typenode_eq(ast->bin_op.left->type, ast->bin_op.right->type)) {
		ast_tmp = promote_type(a, ast->bin_op.left->type, ast->bin_op.right);
		if (ast_tmp) {
			ast->bin_op.right = ast_tmp;
		} else {
			ast_tmp = promote_type(a, ast->bin_op.right->type, ast->bin_op.left);
			if (ast_tmp) {
				ast->bin_op.left = ast_tmp;
			} else {
				FATAL_BAD_TYPE("mul", ast->bin_op.left->type->kind);
			}
		}
	}
	if (ast->bin_op.left->type->kind & TNK_INTEGER) {
		left = _codegen(a, tmp, ast->bin_op.left, ctx);
		right = _codegen(a, tmp, ast->bin_op.right, ctx);
		return LLVMBuildMul(ctx->builder, left, right, "imul");
	}
	if (ast->bin_op.left->type->kind & TNK_FLOAT) {
		left = _codegen(a, tmp, ast->bin_op.left, ctx);
		right = _codegen(a, tmp, ast->bin_op.right, ctx);
		return LLVMBuildFMul(ctx->builder, left, right, "fmul");
	}
	FATAL_BAD_TYPE("mul", ast->bin_op.left->type->kind);
}

static void *ast_e_div(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	LLVMValueRef left;
	LLVMValueRef right;
	astnode *ast_tmp;
	if (!typenode_eq(ast->bin_op.left->type, ast->bin_op.right->type)) {
		ast_tmp = promote_type(a, ast->bin_op.left->type, ast->bin_op.right);
		if (ast_tmp) {
			ast->bin_op.right = ast_tmp;
		} else {
			ast_tmp = promote_type(a, ast->bin_op.right->type, ast->bin_op.left);
			if (ast_tmp) {
				ast->bin_op.left = ast_tmp;
			} else {
				FATAL_BAD_TYPE("div", ast->bin_op.left->type->kind);
			}
		}
	}
	if (ast->bin_op.left->type->kind & TNK_INTEGER) {
		left = _codegen(a, tmp, ast->bin_op.left, ctx);
		right = _codegen(a, tmp, ast->bin_op.right, ctx);
		return LLVMBuildSDiv(ctx->builder, left, right, "idiv");
	}
	if (ast->bin_op.left->type->kind & TNK_FLOAT) {
		left = _codegen(a, tmp, ast->bin_op.left, ctx);
		right = _codegen(a, tmp, ast->bin_op.right, ctx);
		return LLVMBuildFDiv(ctx->builder, left, right, "fdiv");
	}
	FATAL_BAD_TYPE("div", ast->bin_op.left->type->kind);
}

static LLVMValueRef ast_to_addr(arena *a, arena tmp, astnode *ast, cgctx *ctx);

static void *ast_e_add(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	LLVMValueRef left;
	LLVMValueRef right;
	astnode *ast_tmp;
	if (!typenode_eq(ast->bin_op.left->type, ast->bin_op.right->type)) {
		if (ast->bin_op.left->type->kind == TNK_PTR && ast->bin_op.right->type->kind & TNK_INTEGER) {
			LLVMValueRef base;
			astnode *idxable = ast->bin_op.left;

			// prebase is the address that holds the pointer
			LLVMValueRef prebase = ast_to_addr(a, tmp, idxable, ctx);
			// base_type is the type of the pointer
			LLVMTypeRef base_type = typeref_make(a, ctx, idxable->type);
			// base is the pointer
			base = LLVMBuildLoad2(ctx->builder, base_type, prebase, "rae");

			LLVMValueRef indices[] = {
				_codegen(a, tmp, ast->bin_op.right, ctx)
			};
			LLVMTypeRef base_subject = typeref_make(a, ctx, ast->bin_op.left->type->ptr.subject);
			return LLVMBuildGEP2(ctx->builder, base_subject, base, indices, 1, "ptradd");
		} else if (ast->bin_op.right->type->kind == TNK_PTR && ast->bin_op.left->type->kind & TNK_INTEGER) {
			LLVMValueRef base;
			astnode *idxable = ast->bin_op.right;

			// prebase is the address that holds the pointer
			LLVMValueRef prebase = ast_to_addr(a, tmp, idxable, ctx);
			// base_type is the type of the pointer
			LLVMTypeRef base_type = typeref_make(a, ctx, idxable->type);
			// base is the pointer
			base = LLVMBuildLoad2(ctx->builder, base_type, prebase, "rae");

			LLVMValueRef indices[] = {
				_codegen(a, tmp, ast->bin_op.left, ctx)
			};
			LLVMTypeRef base_subject = typeref_make(a, ctx, ast->bin_op.right->type->ptr.subject);
			return LLVMBuildGEP2(ctx->builder, base_subject, base, indices, 1, "ptradd");
		}
		
		ast_tmp = promote_type(a, ast->bin_op.left->type, ast->bin_op.right);
		if (ast_tmp) {
			ast->bin_op.right = ast_tmp;
		} else {
			ast_tmp = promote_type(a, ast->bin_op.right->type, ast->bin_op.left);
			if (ast_tmp) {
				ast->bin_op.left = ast_tmp;
			} else {
				FATAL_BAD_TYPE("add", ast->bin_op.left->type->kind);
			}
		}
	}
	if (ast->bin_op.left->type->kind & TNK_INTEGER) {
		left = _codegen(a, tmp, ast->bin_op.left, ctx);
		right = _codegen(a, tmp, ast->bin_op.right, ctx);
		return LLVMBuildAdd(ctx->builder, left, right, "iadd");
	}
	if (ast->bin_op.left->type->kind & TNK_FLOAT) {
		left = _codegen(a, tmp, ast->bin_op.left, ctx);
		right = _codegen(a, tmp, ast->bin_op.right, ctx);
		return LLVMBuildFAdd(ctx->builder, left, right, "fadd");
	}
	FATAL_BAD_TYPE("add", ast->bin_op.left->type->kind);
}

static void *ast_e_sub(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	LLVMValueRef left;
	LLVMValueRef right;
	astnode *ast_tmp;
	if (!typenode_eq(ast->bin_op.left->type, ast->bin_op.right->type)) {
		ast_tmp = promote_type(a, ast->bin_op.left->type, ast->bin_op.right);
		if (ast_tmp) {
			ast->bin_op.right = ast_tmp;
		} else {
			ast_tmp = promote_type(a, ast->bin_op.right->type, ast->bin_op.left);
			if (ast_tmp) {
				ast->bin_op.left = ast_tmp;
			} else {
				FATAL_BAD_TYPE("sub", ast->bin_op.left->type->kind);
			}
		}
	}
	if (ast->bin_op.left->type->kind & TNK_INTEGER) {
		left = _codegen(a, tmp, ast->bin_op.left, ctx);
		right = _codegen(a, tmp, ast->bin_op.right, ctx);
		return LLVMBuildSub(ctx->builder, left, right, "isub");
	}
	if (ast->bin_op.left->type->kind & TNK_FLOAT) {
		left = _codegen(a, tmp, ast->bin_op.left, ctx);
		right = _codegen(a, tmp, ast->bin_op.right, ctx);
		return LLVMBuildFSub(ctx->builder, left, right, "fsub");
	}
	FATAL_BAD_TYPE("sub", ast->bin_op.left->type->kind);
}

static void *ast_e_mod(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	LLVMValueRef left;
	LLVMValueRef right;
	astnode *ast_tmp;
	if (!typenode_eq(ast->bin_op.left->type, ast->bin_op.right->type)) {
		ast_tmp = promote_type(a, ast->bin_op.left->type, ast->bin_op.right);
		if (ast_tmp) {
			ast->bin_op.right = ast_tmp;
		} else {
			ast_tmp = promote_type(a, ast->bin_op.right->type, ast->bin_op.left);
			if (ast_tmp) {
				ast->bin_op.left = ast_tmp;
			} else {
				FATAL_BAD_TYPE("remainder", ast->bin_op.left->type->kind);
			}
		}
	}
	if (ast->bin_op.left->type->kind & TNK_INTEGER) {
		left = _codegen(a, tmp, ast->bin_op.left, ctx);
		right = _codegen(a, tmp, ast->bin_op.right, ctx);
		return LLVMBuildSRem(ctx->builder, left, right, "srem");
	}
	if (ast->bin_op.left->type->kind & TNK_FLOAT) {
		left = _codegen(a, tmp, ast->bin_op.left, ctx);
		right = _codegen(a, tmp, ast->bin_op.right, ctx);
		return LLVMBuildFRem(ctx->builder, left, right, "frem");
	}
	FATAL_BAD_TYPE("remainder", ast->bin_op.left->type->kind);
}

static LLVMValueRef ast_e_var_to_addr(astnode *ast, cgctx *ctx)
{
	LLVMValueRef value;
	if (!symtbl_get(&ctx->symtbl, ast->var.id, (void **)&value)) {
		FATAL_UNDEF_VAR(ast->var.id);
	}
	return value;
}

static LLVMValueRef ast_e_deref_to_addr(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	LLVMValueRef addr = ast_to_addr(a, tmp, ast->deref.ptr, ctx);
	LLVMTypeRef type = typeref_make(a, ctx, ast->deref.ptr->type);
	return LLVMBuildLoad2(ctx->builder, type, addr, "deref");
}

static LLVMValueRef ast_e_index_to_addr(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	LLVMValueRef base;
	astnode *idxable = ast->index.array;
	if (idxable->type->kind == TNK_PTR) {
		// prebase is the address that holds the pointer
		LLVMValueRef prebase = ast_to_addr(a, tmp, idxable, ctx);
		// base_type is the type of the pointer
		LLVMTypeRef base_type = typeref_make(a, ctx, idxable->type);
		// base is the pointer
		base = LLVMBuildLoad2(ctx->builder, base_type, prebase, "rae");
	} else if (idxable->type->kind == TNK_ARRAY) {
		// base is the pointer
		base = ast_to_addr(a, tmp, idxable, ctx);
	} else {
		FATAL_NONINDEXABLE();
	}

	int idx_cnt = 1;
	LLVMValueRef indices[] = {
		_codegen(a, tmp, ast->index.idx, ctx)
	};

	// base_subject is the type that the pointer points to
	LLVMTypeRef base_subject = typeref_make(a, ctx, ast->type);
	return LLVMBuildGEP2(ctx->builder, base_subject, base, indices, idx_cnt, "idx");
}

static LLVMValueRef ast_e_memacc_to_addr(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	astnode *idxable = ast->acc.strct;
	LLVMValueRef base = ast_to_addr(a, tmp, idxable, ctx);

	int idx_cnt = 1;
	LLVMTypeRef i32 = LLVMInt32TypeInContext(ctx->llvm);
	LLVMValueRef indices[] = {
		LLVMConstInt(i32, ast->acc.idx, 0)
	};

	char c_str[ast->acc.name.len + 1];
	memcpy(c_str, ast->acc.name.addr, ast->acc.name.len);
	c_str[ast->acc.name.len] = '\0';

	LLVMTypeRef base_subject = typeref_make(a, ctx, ast->type);
	return LLVMBuildGEP2(ctx->builder, base_subject, base, indices, idx_cnt, c_str);
}

static LLVMValueRef ast_e_deref_memacc_to_addr(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	LLVMValueRef prebase = ast_to_addr(a, tmp, ast->acc.strct, ctx);
	LLVMTypeRef base_type = typeref_make(a, ctx, ast->acc.strct->type);
	LLVMValueRef base = LLVMBuildLoad2(ctx->builder, base_type, prebase, "keys");

	int idx_cnt = 1;
	LLVMTypeRef i32 = LLVMInt32TypeInContext(ctx->llvm);
	LLVMValueRef indices[] = {
		LLVMConstInt(i32, ast->acc.idx, 0)
	};

	char c_str[ast->acc.name.len + 1];
	memcpy(c_str, ast->acc.name.addr, ast->acc.name.len);
	c_str[ast->acc.name.len] = '\0';

	LLVMTypeRef base_subject = typeref_make(a, ctx, ast->type);
	return LLVMBuildGEP2(ctx->builder, base_subject, base, indices, idx_cnt, c_str);
}


static LLVMValueRef ast_to_addr(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	switch (ast->kind) {
	case AST_E_VAR:
		return ast_e_var_to_addr(ast, ctx);
	case AST_E_DEREF:
		return ast_e_deref_to_addr(a, tmp, ast, ctx);
	case AST_E_INDEX:
		return ast_e_index_to_addr(a, tmp, ast, ctx);
	case AST_E_MEMACC: 
		return ast_e_memacc_to_addr(a, tmp, ast, ctx);
	case AST_E_DEREF_MEMACC: 
		return ast_e_deref_memacc_to_addr(a, tmp, ast, ctx);
	default:
		FATAL_UNSUP_VAR(ast->kind);
	}
	return NULL;
}

static void *ast_e_var(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	LLVMValueRef value = ast_to_addr(a, tmp, ast, ctx);
	LLVMTypeRef type = typeref_make(a, ctx, ast->type);

	char c_str[ast->var.id.len + 1];
	memcpy(c_str, ast->var.id.addr, ast->var.id.len);
	c_str[ast->var.id.len] = '\0';

	return LLVMBuildLoad2(ctx->builder, type, value, c_str);
}

static void *ast_e_memacc(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	LLVMValueRef value = ast_to_addr(a, tmp, ast, ctx);
	LLVMTypeRef type = typeref_make(a, ctx, ast->type);

	char c_str[ast->acc.name.len + 1];
	memcpy(c_str, ast->acc.name.addr, ast->acc.name.len);
	c_str[ast->acc.name.len] = '\0';

	return LLVMBuildLoad2(ctx->builder, type, value, c_str);
}

static void *ast_e_ref(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	return ast_to_addr(a, tmp, ast->ref.expr, ctx);
}

static void *ast_e_deref(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	LLVMValueRef value = ast_to_addr(a, tmp, ast, ctx);
	LLVMTypeRef type = typeref_make(a, ctx, ast->type);

	char c_str[ast->acc.name.len + 1];
	memcpy(c_str, ast->acc.name.addr, ast->acc.name.len);
	c_str[ast->acc.name.len] = '\0';

	return LLVMBuildLoad2(ctx->builder, type, value, c_str);
}

static void *ast_e_lt(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	if (!typenode_eq(ast->bin_op.left->type, ast->bin_op.right->type)) {
		astnode *ast_tmp = promote_type(a, ast->bin_op.left->type, ast->bin_op.right);
		if (ast_tmp) {
			ast->bin_op.right = ast_tmp;
		} else {
			ast_tmp = promote_type(a, ast->bin_op.right->type, ast->bin_op.left);
			if (ast_tmp) {
				ast->bin_op.left = ast_tmp;
			} else {
				FATAL_BAD_TYPE("mul", ast->bin_op.left->type->kind);
			}
		}
	}
	if (ast->bin_op.left->type->kind & TNK_INTEGER) {
		LLVMValueRef left = _codegen(a, tmp, ast->bin_op.left, ctx);
		LLVMValueRef right = _codegen(a, tmp, ast->bin_op.right, ctx);
		return LLVMBuildICmp(ctx->builder, LLVMIntSLT, left, right, "ilt");
	} else if (ast->bin_op.left->type->kind & TNK_FLOAT) {
		LLVMValueRef left = _codegen(a, tmp, ast->bin_op.left, ctx);
		LLVMValueRef right = _codegen(a, tmp, ast->bin_op.right, ctx);
		return LLVMBuildFCmp(ctx->builder, LLVMRealOLT, left, right, "flt");
	}
	FATAL_BAD_CMP();
}

static void *ast_e_lte(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	if (!typenode_eq(ast->bin_op.left->type, ast->bin_op.right->type)) {
		astnode *ast_tmp = promote_type(a, ast->bin_op.left->type, ast->bin_op.right);
		if (ast_tmp) {
			ast->bin_op.right = ast_tmp;
		} else {
			ast_tmp = promote_type(a, ast->bin_op.right->type, ast->bin_op.left);
			if (ast_tmp) {
				ast->bin_op.left = ast_tmp;
			} else {
				FATAL_BAD_TYPE("mul", ast->bin_op.left->type->kind);
			}
		}
	}
	if (ast->bin_op.left->type->kind & TNK_INTEGER) {
		LLVMValueRef left = _codegen(a, tmp, ast->bin_op.left, ctx);
		LLVMValueRef right = _codegen(a, tmp, ast->bin_op.right, ctx);
		return LLVMBuildICmp(ctx->builder, LLVMIntSLE, left, right, "ile");
	} else if (ast->bin_op.left->type->kind & TNK_FLOAT) {
		LLVMValueRef left = _codegen(a, tmp, ast->bin_op.left, ctx);
		LLVMValueRef right = _codegen(a, tmp, ast->bin_op.right, ctx);
		return LLVMBuildFCmp(ctx->builder, LLVMRealOLE, left, right, "fle");
	}
	FATAL_BAD_CMP();
}

static void *ast_e_gt(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	if (!typenode_eq(ast->bin_op.left->type, ast->bin_op.right->type)) {
		astnode *ast_tmp = promote_type(a, ast->bin_op.left->type, ast->bin_op.right);
		if (ast_tmp) {
			ast->bin_op.right = ast_tmp;
		} else {
			ast_tmp = promote_type(a, ast->bin_op.right->type, ast->bin_op.left);
			if (ast_tmp) {
				ast->bin_op.left = ast_tmp;
			} else {
				FATAL_BAD_TYPE("mul", ast->bin_op.left->type->kind);
			}
		}
	}
	if (ast->bin_op.left->type->kind & TNK_INTEGER) {
		LLVMValueRef left = _codegen(a, tmp, ast->bin_op.left, ctx);
		LLVMValueRef right = _codegen(a, tmp, ast->bin_op.right, ctx);
		return LLVMBuildICmp(ctx->builder, LLVMIntSGT, left, right, "igt");
	} else if (ast->bin_op.left->type->kind & TNK_FLOAT) {
		LLVMValueRef left = _codegen(a, tmp, ast->bin_op.left, ctx);
		LLVMValueRef right = _codegen(a, tmp, ast->bin_op.right, ctx);
		return LLVMBuildFCmp(ctx->builder, LLVMRealOGT, left, right, "fgt");
	}
	FATAL_BAD_CMP();
}

static void *ast_e_gte(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	if (!typenode_eq(ast->bin_op.left->type, ast->bin_op.right->type)) {
		astnode *ast_tmp = promote_type(a, ast->bin_op.left->type, ast->bin_op.right);
		if (ast_tmp) {
			ast->bin_op.right = ast_tmp;
		} else {
			ast_tmp = promote_type(a, ast->bin_op.right->type, ast->bin_op.left);
			if (ast_tmp) {
				ast->bin_op.left = ast_tmp;
			} else {
				FATAL_BAD_TYPE("mul", ast->bin_op.left->type->kind);
			}
		}
	}
	if (ast->bin_op.left->type->kind & TNK_INTEGER) {
		LLVMValueRef left = _codegen(a, tmp, ast->bin_op.left, ctx);
		LLVMValueRef right = _codegen(a, tmp, ast->bin_op.right, ctx);
		return LLVMBuildICmp(ctx->builder, LLVMIntSGE, left, right, "ige");
	} else if (ast->bin_op.left->type->kind & TNK_FLOAT) {
		LLVMValueRef left = _codegen(a, tmp, ast->bin_op.left, ctx);
		LLVMValueRef right = _codegen(a, tmp, ast->bin_op.right, ctx);
		return LLVMBuildFCmp(ctx->builder, LLVMRealOGE, left, right, "fge");
	}
	FATAL_BAD_CMP();
}

static void *ast_e_eq(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	if (!typenode_eq(ast->bin_op.left->type, ast->bin_op.right->type)) {
		astnode *ast_tmp = promote_type(a, ast->bin_op.left->type, ast->bin_op.right);
		if (ast_tmp) {
			ast->bin_op.right = ast_tmp;
		} else {
			ast_tmp = promote_type(a, ast->bin_op.right->type, ast->bin_op.left);
			if (ast_tmp) {
				ast->bin_op.left = ast_tmp;
			} else {
				FATAL_BAD_TYPE("eq", ast->bin_op.left->type->kind);
			}
		}
	}
	if (ast->bin_op.left->type->kind & TNK_INTEGER) {
		LLVMValueRef left = _codegen(a, tmp, ast->bin_op.left, ctx);
		LLVMValueRef right = _codegen(a, tmp, ast->bin_op.right, ctx);
		return LLVMBuildICmp(ctx->builder, LLVMIntEQ, left, right, "ieq");
	} else if (ast->bin_op.left->type->kind & TNK_FLOAT) {
		LLVMValueRef left = _codegen(a, tmp, ast->bin_op.left, ctx);
		LLVMValueRef right = _codegen(a, tmp, ast->bin_op.right, ctx);
		return LLVMBuildFCmp(ctx->builder, LLVMRealOEQ, left, right, "feq");
	}
	FATAL_BAD_CMP();
}

static void *ast_e_neq(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	if (!typenode_eq(ast->bin_op.left->type, ast->bin_op.right->type)) {
		astnode *ast_tmp = promote_type(a, ast->bin_op.left->type, ast->bin_op.right);
		if (ast_tmp) {
			ast->bin_op.right = ast_tmp;
		} else {
			ast_tmp = promote_type(a, ast->bin_op.right->type, ast->bin_op.left);
			if (ast_tmp) {
				ast->bin_op.left = ast_tmp;
			} else {
				FATAL_BAD_TYPE("mul", ast->bin_op.left->type->kind);
			}
		}
	}
	if (ast->bin_op.left->type->kind & TNK_INTEGER) {
		LLVMValueRef left = _codegen(a, tmp, ast->bin_op.left, ctx);
		LLVMValueRef right = _codegen(a, tmp, ast->bin_op.right, ctx);
		return LLVMBuildICmp(ctx->builder, LLVMIntNE, left, right, "ine");
	} else if (ast->bin_op.left->type->kind & TNK_FLOAT) {
		LLVMValueRef left = _codegen(a, tmp, ast->bin_op.left, ctx);
		LLVMValueRef right = _codegen(a, tmp, ast->bin_op.right, ctx);
		return LLVMBuildFCmp(ctx->builder, LLVMRealONE, left, right, "fne");
	}
	FATAL_BAD_CMP();
}

static void *ast_e_cast(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	if (ast->type->kind & TNK_INTEGER && ast->cast.expr->type->kind & TNK_INTEGER) {
		LLVMTypeRef type_dest = typeref_make(a, ctx, ast->type);
		int width_dest = LLVMGetIntTypeWidth(type_dest);
		LLVMTypeRef type_src = typeref_make(a, ctx, ast->cast.expr->type);
		int width_src = LLVMGetIntTypeWidth(type_src);
		LLVMValueRef expr = _codegen(a, tmp, ast->cast.expr, ctx);
		if (width_dest < width_src) {
			return LLVMBuildTrunc(ctx->builder, expr, type_dest, "trunc");
		}
		return LLVMBuildSExt(ctx->builder, expr, type_dest, "sext");
	}
	if (ast->type->kind & TNK_FLOAT && ast->cast.expr->type->kind & TNK_INTEGER) {
		LLVMTypeRef dest = typeref_make(a, ctx, ast->type);
		LLVMValueRef expr = _codegen(a, tmp, ast->cast.expr, ctx);
		return LLVMBuildFPToSI(ctx->builder, expr, dest, "fptosi");
	}
	return _codegen(a, tmp, ast->cast.expr, ctx);
}

static void *ast_e_string(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	LLVMTypeRef i8 = LLVMInt8TypeInContext(ctx->llvm);
	LLVMTypeRef buffer_type = LLVMArrayType2(i8, ast->string.data.len);
	LLVMValueRef buffer = LLVMAddGlobal(ctx->module, buffer_type, "ctsb");
	LLVMValueRef buffer_init = LLVMConstStringInContext(ctx->llvm, ast->string.data.addr, ast->string.data.len, 1);
	LLVMSetInitializer(buffer, buffer_init);

	//printf("creating compile time constant: '%.*s'(%i)\n", ast->string.data.len, ast->string.data.addr, ast->string.data.len);

	LLVMTypeRef i32 = LLVMInt32TypeInContext(ctx->llvm);
	LLVMTypeRef str_type = typeref_make(a, ctx, ast->type->ptr.subject);
	LLVMValueRef str = LLVMAddGlobal(ctx->module, str_type, "ctss");
	LLVMValueRef members[2] = {
		LLVMConstInt(i32, ast->string.data.len, 0),
		buffer
	};
	LLVMValueRef str_init = LLVMConstNamedStruct(str_type, members, 2);
	LLVMSetInitializer(str, str_init);

	return str;
}

static void *ast_s_return(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	astnode *cast = create_cast(a, ctx->ret_tn, ast->rstmt.value);
	LLVMValueRef value = _codegen(a, tmp, cast, ctx);
	LLVMBuildRet(ctx->builder, value);
	return NULL;
}

static void *ast_s_block(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	for (int i = 0; i < ast->bstmt.body.len; i++) {
		_codegen(a, tmp, ast->bstmt.body.memory[i], ctx);
	}
	return NULL;
}

static void *ast_s_decl(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	char c_str[ast->dstmt.name.len + 1];
	memcpy(c_str, ast->dstmt.name.addr, ast->dstmt.name.len);
	c_str[ast->dstmt.name.len] = '\0';
	LLVMTypeRef type = typeref_make(a, ctx, ast->dstmt.type);

	LLVMBasicBlockRef curblock = LLVMGetInsertBlock(ctx->builder);
	LLVMValueRef parent = LLVMGetBasicBlockParent(curblock);
	LLVMBasicBlockRef entryblock = LLVMGetEntryBasicBlock(parent);
	LLVMValueRef start = LLVMGetFirstInstruction(entryblock);
	LLVMBuilderRef builder = LLVMCreateBuilderInContext(ctx->llvm);
	if (start) {
		LLVMPositionBuilderBefore(builder, start);
	} else {
		LLVMPositionBuilderAtEnd(builder, entryblock);
	}
	LLVMValueRef variable = LLVMBuildAlloca(builder, type, c_str);
	if (ast->dstmt.expr) {
		LLVMValueRef value = _codegen(a, tmp, ast->dstmt.expr, ctx);
		LLVMBuildStore(ctx->builder, value, variable);
	}
	symtbl_set(&ctx->symtbl, ast->dstmt.name, variable);

	return NULL;
}


static void *ast_s_assign(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	LLVMValueRef lval = ast_to_addr(a, tmp, ast->astmt.lval, ctx);
	LLVMValueRef rval = _codegen(a, tmp, ast->astmt.expr, ctx);
	LLVMBuildStore(ctx->builder, rval, lval);
	return NULL;
}

static void *ast_s_call(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	_codegen(a, tmp, ast->cstmt.fn_call, ctx);
	return NULL;
}

static void *ast_s_while(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	LLVMBasicBlockRef curblk = LLVMGetInsertBlock(ctx->builder);
	LLVMValueRef parent = LLVMGetBasicBlockParent(curblk);
	LLVMBasicBlockRef loophdr = LLVMAppendBasicBlockInContext(ctx->llvm, parent, "whdr");
	LLVMBasicBlockRef loopbdy = LLVMCreateBasicBlockInContext(ctx->llvm, "wbdy");
	LLVMBasicBlockRef loopend = LLVMCreateBasicBlockInContext(ctx->llvm, "wend");

	LLVMBuildBr(ctx->builder, loophdr);
	LLVMPositionBuilderAtEnd(ctx->builder, loophdr);
	// TODO: cannot be made INTO i64 or something
	if (!(ast->wstmt.cond->type->kind & TNK_INTEGER)) {
		FATAL_BAD_CONDITIONAL();
	}
	LLVMValueRef cond = _codegen(a, tmp, ast->wstmt.cond, ctx);
	LLVMBuildCondBr(ctx->builder, cond, loopbdy, loopend);

	LLVMAppendExistingBasicBlock(parent, loopbdy);
	LLVMPositionBuilderAtEnd(ctx->builder, loopbdy);
	for (int i = 0; i < ast->wstmt.body.len; i++) {
		_codegen(a, tmp, ast->wstmt.body.memory[i], ctx);
	}

	loopbdy = LLVMGetInsertBlock(ctx->builder);
	LLVMValueRef lastinst = LLVMGetLastInstruction(loopbdy);
	if (!lastinst || !LLVMIsATerminatorInst(lastinst)) {
		LLVMBuildBr(ctx->builder, loophdr);
	}

	LLVMAppendExistingBasicBlock(parent, loopend);
	LLVMPositionBuilderAtEnd(ctx->builder, loopend);
	return NULL;
}

static void *ast_s_if(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	LLVMValueRef cond = _codegen(a, tmp, ast->istmt.cond, ctx);
	LLVMTypeRef cond_type = LLVMTypeOf(cond);
	if (LLVMGetTypeKind(cond_type) != LLVMIntegerTypeKind) {
		FATAL_GENERIC_UNSUPPORTED("non integer conditional");
	}
	LLVMTypeRef type_dest = LLVMInt1TypeInContext(ctx->llvm);
	cond = LLVMBuildTrunc(ctx->builder, cond, type_dest, "iftrunc");

	LLVMBasicBlockRef curblk = LLVMGetInsertBlock(ctx->builder);
	LLVMValueRef parent = LLVMGetBasicBlockParent(curblk);
	LLVMBasicBlockRef thenb = LLVMAppendBasicBlockInContext(ctx->llvm, parent, "then");
	LLVMBasicBlockRef mrgeb = LLVMCreateBasicBlockInContext(ctx->llvm, "merge");

	LLVMBuildCondBr(ctx->builder, cond, thenb, mrgeb);
	LLVMPositionBuilderAtEnd(ctx->builder, thenb);
	for (int i = 0; i < ast->istmt.branch_t.len; i++) {
		_codegen(a, tmp, ast->istmt.branch_t.memory[i], ctx);
	}
	thenb = LLVMGetInsertBlock(ctx->builder);
	LLVMValueRef lastinst = LLVMGetLastInstruction(thenb);
	if (!lastinst || !LLVMIsATerminatorInst(lastinst)) {
		LLVMBuildBr(ctx->builder, mrgeb);
	}

	LLVMAppendExistingBasicBlock(parent, mrgeb);
	LLVMPositionBuilderAtEnd(ctx->builder, mrgeb);

	return NULL;
}

static void *ast_s_else(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	LLVMValueRef cond = _codegen(a, tmp, ast->istmt.cond, ctx);
	LLVMTypeRef cond_type = LLVMTypeOf(cond);
	if (LLVMGetTypeKind(cond_type) != LLVMIntegerTypeKind) {
		FATAL_GENERIC_UNSUPPORTED("non integer conditional");
	}
	LLVMTypeRef type_dest = LLVMInt1TypeInContext(ctx->llvm);
	cond = LLVMBuildTrunc(ctx->builder, cond, type_dest, "iftrunc");

	LLVMBasicBlockRef curblk = LLVMGetInsertBlock(ctx->builder);
	LLVMValueRef parent = LLVMGetBasicBlockParent(curblk);
	LLVMBasicBlockRef thenb = LLVMAppendBasicBlockInContext(ctx->llvm, parent, "then");
	LLVMBasicBlockRef elseb = LLVMCreateBasicBlockInContext(ctx->llvm, "else");
	LLVMBasicBlockRef mrgeb = LLVMCreateBasicBlockInContext(ctx->llvm, "merge");

	LLVMBuildCondBr(ctx->builder, cond, thenb, elseb);
	LLVMPositionBuilderAtEnd(ctx->builder, thenb);
	for (int i = 0; i < ast->istmt.branch_t.len; i++) {
		_codegen(a, tmp, ast->istmt.branch_t.memory[i], ctx);
	}
	thenb = LLVMGetInsertBlock(ctx->builder);
	LLVMValueRef lastinst = LLVMGetLastInstruction(thenb);
	if (!lastinst || !LLVMIsATerminatorInst(lastinst)) {
		LLVMBuildBr(ctx->builder, mrgeb);
	}

	LLVMAppendExistingBasicBlock(parent, elseb);
	LLVMPositionBuilderAtEnd(ctx->builder, elseb);
	for (int i = 0; i < ast->istmt.branch_f.len; i++) {
		_codegen(a, tmp, ast->istmt.branch_f.memory[i], ctx);
	}
	elseb = LLVMGetInsertBlock(ctx->builder);
	lastinst = LLVMGetLastInstruction(thenb);
	if (!lastinst || !LLVMIsATerminatorInst(lastinst)) {
		LLVMBuildBr(ctx->builder, mrgeb);
	}

	LLVMAppendExistingBasicBlock(parent, mrgeb);
	LLVMPositionBuilderAtEnd(ctx->builder, mrgeb);

	return NULL;
}

static void *ast_proc_def(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	symtbl_scope_enter(&ctx->symtbl);
	symtbl_scope_clear(&ctx->symtbl);

	// get c_str for proc name
	eligible_id id = ast->proc_def.name;
	char memory[id.ns.len + id.id.len + 2];
	char *name = eligible_c_str(memory, ast->proc_def.name);

	// construct type of procedure
	int argc = ast->proc_def.args.len;
	LLVMTypeRef args[argc + 1];
	for (int i = 0; i < argc; i++) {
		args[i] = typeref_make(a, ctx, ast->proc_def.args.memory[i]->type);
	}
	ctx->ret_tn = ast->proc_def.return_type;
	LLVMTypeRef return_type = typeref_make(a, ctx, ctx->ret_tn);
	LLVMTypeRef fntype = LLVMFunctionType(return_type, args, argc, 0);
	LLVMValueRef fnvalue = LLVMAddFunction(ctx->module, name, fntype);
	LLVMLinkage linkage = ast->proc_def.exported 
		? LLVMExternalLinkage 
		: LLVMInternalLinkage;
	LLVMSetLinkage(fnvalue, linkage);
	LLVMBasicBlockRef block = LLVMAppendBasicBlockInContext(ctx->llvm, fnvalue, "entry");
	LLVMPositionBuilderAtEnd(ctx->builder, block);

	// enter arguments into symtbl
	LLVMValueRef arg;
	LLVMTypeRef arg_type;
	struct proc_def_arg *pda;
	for (int i = 0; i < argc; i++) {
		arg = LLVMGetParam(fnvalue, i);
		pda = ast->proc_def.args.memory[i];
		arg_type = typeref_make(a, ctx, pda->type);

		char c_str[pda->name.len + 1];
		memcpy(c_str, pda->name.addr, pda->name.len);
		c_str[pda->name.len] = '\0';

		LLVMValueRef variable = LLVMBuildAlloca(ctx->builder, arg_type, c_str);
		LLVMBuildStore(ctx->builder, arg, variable);
		symtbl_set(&ctx->symtbl, pda->name, variable);
	}

	// emit body
	astnode *stmt;
	for (int i = 0; i < ast->proc_def.stmts.len; i++) {
		_codegen(a, tmp, ast->proc_def.stmts.memory[i], ctx);
	}

	if (ast->proc_def.implicit_ret) {
		LLVMBuildRet(ctx->builder, NULL);
	}

	symtbl_scope_leave(&ctx->symtbl);

	return NULL;
}

static void *ast_proc_imp(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	// get c_str for proc name
	eligible_id id = ast->proc_imp.name;
	char memory[id.ns.len + id.id.len + 2];
	char *name = eligible_c_str(memory, ast->proc_imp.name);

	// construct type of procedure
	int argc = ast->proc_imp.args.len;
	LLVMTypeRef args[argc + 1];
	for (int i = 0; i < argc; i++) {
		args[i] = typeref_make(a, ctx, ast->proc_imp.args.memory[i]->type);
	}
	LLVMTypeRef return_type = typeref_make(a, ctx, ast->proc_imp.return_type);
	LLVMTypeRef fntype = LLVMFunctionType(return_type, args, argc, 0);
	LLVMValueRef fnvalue = LLVMAddFunction(ctx->module, name, fntype);
	LLVMSetLinkage(fnvalue, LLVMExternalLinkage);

	return NULL;
}

static void *ast_struct_def(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	char c_str[ast->strct.name.ns.len + ast->strct.name.id.len + 1];
	eligible_c_str(c_str, ast->strct.name);
	string combo = string_mask(a, c_str);

	int memc = ast->type->strct.members.len;
	LLVMTypeRef elements[memc];
	LLVMTypeRef strct = LLVMStructCreateNamed(ctx->llvm, c_str);
	for (int i = 0; i < memc; i++) {
		elements[i] = typeref_make(a, ctx, ast->type->strct.members.memory[i]->type);
	}
	LLVMStructSetBody(strct, elements, memc, 0);

	dprh_tbl_set(&ctx->udtypes, combo, strct);

	return NULL;
}

static void *ast_module(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	for (int i = 0; i < ast->module.stmts.len; i++) {
		_codegen(a, tmp, ast->module.stmts.memory[i], ctx);
	}
	return NULL;
}


static void *_codegen(arena *a, arena tmp, astnode *ast, cgctx *ctx)
{
	switch (ast->kind) {
	case AST_E_INT:
		return ast_e_int(a, tmp, ast, ctx);
	case AST_E_FLOAT:
		return ast_e_float(a, tmp, ast, ctx);
	case AST_E_CALL:
		return ast_e_call(a, tmp, ast, ctx);
	case AST_E_MUL:
		return ast_e_mul(a, tmp, ast, ctx);
	case AST_E_DIV:
		return ast_e_div(a, tmp, ast, ctx);
	case AST_E_ADD:
		return ast_e_add(a, tmp, ast, ctx);
	case AST_E_SUB:
		return ast_e_sub(a, tmp, ast, ctx);
	case AST_E_MOD:
		return ast_e_mod(a, tmp, ast, ctx);
	case AST_E_VAR:
		return ast_e_var(a, tmp, ast, ctx);
	case AST_E_MEMACC:
		return ast_e_memacc(a, tmp, ast, ctx);
	case AST_E_DEREF_MEMACC:
		return ast_e_memacc(a, tmp, ast, ctx);
	case AST_E_REF:
		return ast_e_ref(a, tmp, ast, ctx);
	case AST_E_DEREF:
		return ast_e_deref(a, tmp, ast, ctx);
	case AST_E_INDEX:
		return ast_e_deref(a, tmp, ast, ctx);
	case AST_E_LT:
		return ast_e_lt(a, tmp, ast, ctx);
	case AST_E_LTE:
		return ast_e_lte(a, tmp, ast, ctx);
	case AST_E_GT:
		return ast_e_gt(a, tmp, ast, ctx);
	case AST_E_GTE:
		return ast_e_gte(a, tmp, ast, ctx);
	case AST_E_EQ:
		return ast_e_eq(a, tmp, ast, ctx);
	case AST_E_NEQ:
		return ast_e_neq(a, tmp, ast, ctx);
	case AST_E_CAST:
		return ast_e_cast(a, tmp, ast, ctx);
	case AST_E_STRING:
		return ast_e_string(a, tmp, ast, ctx);
	case AST_S_RETURN:
		return ast_s_return(a, tmp, ast, ctx);
	case AST_S_DECL:
		return ast_s_decl(a, tmp, ast, ctx);
	case AST_S_ASSIGN:
		return ast_s_assign(a, tmp, ast, ctx);
	case AST_S_CALL:
		return ast_s_call(a, tmp, ast, ctx);
	case AST_S_WHILE:
		return ast_s_while(a, tmp, ast, ctx);
	case AST_S_IF:
		return ast_s_if(a, tmp, ast, ctx);
	case AST_S_IFELSE:
		return ast_s_else(a, tmp, ast, ctx);
	case AST_S_BLOCK:
		return ast_s_block(a, tmp, ast, ctx);
	case AST_PROC_IMPORT:
		return ast_proc_imp(a, tmp, ast, ctx);
	case AST_PROC_DEF:
		return ast_proc_def(a, tmp, ast, ctx);
	case AST_STRUCT_DEF:
		return ast_struct_def(a, tmp, ast, ctx);
	case AST_MODULE:
		return ast_module(a, tmp, ast, ctx);
	case AST_SEAM:
		FATAL_BAD_SEAM();
	default:
		FATAL_UNDEF_SEM_RULE(ast->kind);
	}
	return NULL;
}

void codegen(arena *a, arena tmp, astnode *ast, cgopt opt)
{
	cgctx *ctx = cgctx_make(a, opt);

	// create string type really quick (break out?)
	typenode type_i8 = { .kind = TNK_i8 };
	typenode type_len = { .kind = TNK_i32 };
	typenode type_addr = { 
		.kind = TNK_PTR, 
		.ptr = { .subject = &type_i8 } 
	};
	LLVMTypeRef str = LLVMStructCreateNamed(ctx->llvm, "std_string");
	LLVMTypeRef elements[] = {
		typeref_make(a, ctx, &type_len),
		typeref_make(a, ctx, &type_addr)
	};
	LLVMStructSetBody(str, elements, 2, 0);
	dprh_tbl_set(&ctx->udtypes, S("std_string"), str);

	// actual code gen
	_codegen(a, tmp, ast, ctx);

	// worry about verification, optimization, and writing to disk
	char *err;
        if (LLVMVerifyModule(ctx->module, LLVMPrintMessageAction, &err)) {
                LLVMDumpModule(ctx->module);
		FATAL_LLVM_ERR2("failed to verify module", err);
        }

        LLVMErrorRef error;
        LLVMPassBuilderOptionsRef popt = LLVMCreatePassBuilderOptions();

	if (opt.optimize) {
		if ((error = LLVMRunPasses(
			ctx->module, 
			"mem2reg,instcombine,early-cse,gvn,sroa", 
			NULL, popt))
		) {
			FATAL_LLVM_ERR(LLVMGetErrorMessage(error));
		}
	}

	string path = opt.dest_path;
        if (!path.addr || path.len < 1) {
                LLVMDumpModule(ctx->module);
        } else {
		printf("%.*s\n", path.len, path.addr);
                char *errmsg = NULL;
                char buffer[path.len + 1];
                memcpy(buffer, path.addr, path.len);
                buffer[path.len] = '\0';
                LLVMTargetMachineEmitToFile(
			ctx->target, ctx->module, buffer, LLVMObjectFile, &errmsg
		);
                if (errmsg != NULL) {
			FATAL_LLVM_ERR(errmsg);
                }
        }
}

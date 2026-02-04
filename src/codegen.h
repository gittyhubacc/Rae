#ifndef gamma_codegen_h
#define gamma_codegen_h

#include <llvm-c/Core.h>
#include <llvm-c/TargetMachine.h>
#include "symtbl.h"
#include "ast.h"

typedef enum {
	LELNUX64,
	WASM32,
} target;

typedef struct {
	int optimize;
	target target;
	string dest_path;
} cgopt; // code gen options

typedef struct {
	LLVMContextRef llvm;
	LLVMModuleRef module;
	LLVMBuilderRef builder;
	LLVMTargetMachineRef target;
	dprh_tbl udtypes;
	symtbl symtbl;
	cgopt opt;
	typenode *ret_tn;
} cgctx; // code gen context

void codegen(arena *a, arena tmp, astnode *ast, cgopt opt);

#endif

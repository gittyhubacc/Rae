#include <llvm-c/Analysis.h>
#include <llvm-c/Core.h>
#include <llvm-c/Transforms/PassBuilder.h>
#include "grammar.h"
#include "codegen.h"
#include "ast.h"

/*
static void search(ptnode *pt)
{
	if (pt->label.type == 55) {
		printf("%i\n", pt->child_cnt);
	}
	for (int i = 0; i < pt->child_cnt; i++) {
		search(&pt->children[i]);
	}
}
*/

int raec(arena *a, arena tmp, string source, cgopt opt)
{
        ptnode *pt = scfe_parse_tree(a, tmp, &rae_grammar, source);

	semctx *sctx = semctx_make(a);
	astnode *ast = sem_engine(a, tmp, sctx, pt);

	codegen(a, tmp, ast, opt);

	return 0;
}

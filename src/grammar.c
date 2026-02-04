#include "grammar.h"

scfe_grammar rae_grammar = {
	.terminals = {
		.len = RAE_TTOK_TOTAL,
		.addr = (string[]) {
			S("return"),
			S("do"),
			S("end"),
			S("if"),
			S("then"),
			S("else"),
			S("while"),
			S("call"),
			S("synth"),
			S("proc"),
			S("attr"),
			S("struct"),
			S("import"),
			S("export"),
			S("not"),
			S("and"),
			S("or"),
			S("cast"),
			S("as"),
			S("ptr"),
			S("bind"),
			S("using"),
			S("cat"),
			S("aspect"),
			S("before"),
			S("after"),
			S("weave"),

			S("="),
			S(","),
			S("!"),
			S("\\-.>"),
			S("\\("),
			S("\\)"),
			S("{"),
			S("}"),
			S("\\["),
			S("\\]"),
			S("\\+"),
			S("\\-"),
			S("\\*"),
			S("/"),
			S("%"),
			S("<"),
			S("<.="),
			S(">"),
			S(">.="),
			S("=.="),
			S("!.="),
			S("&"),
			S("\\."),

			S("0+([1-9].[0-9]*)"), 			// int literal
			S("(0+([1-9].[0-9]*)).\\..([0-9]*)"), 	// float literal
			S("\".([\\-!]+[#-\\])*.(~+\")"), 	// string literal

			S("(_+[a-zA-Z]).(_+[0-9a-zA-Z])*"),	// plain identifier
			S("(_+[a-zA-Z]).(_+[0-9a-zA-Z])*.(~+:).(~+_+[0-9a-zA-Z]).(_+[0-9a-zA-Z])*"),

			// for deferred tokenization
			S("`.([\\-_]+[a-\\])*.(~+`)"), // regex
			//S("|.([\\-{]+[}-\\])*.(~+|)"), // rule body
		}
	},
	.nonterminals = {
		.len = SYN_RULE_TOTAL,
		.addr = (scfe_pda_rule[]){
			// expression rules
			{
				.head = {.type = NTOK_EXPR_PRIMARY},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_PARENL},
						{ .type = NTOK_EXPRESSION},
						{ .type = TTOK_PARENR},
					}
				}
			},
			{
				.head = {.type = NTOK_EXPR_PRIMARY},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_LIT_INT },
					}
				}
			},
			{
				.head = {.type = NTOK_EXPR_PRIMARY},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_LIT_FLOAT },
					}
				}
			},
			{
				.head = {.type = NTOK_EXPR_PRIMARY},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_PROC_APP},
					}
				}
			},
			{
				.head = {.type = NTOK_EXPR_PRIMARY},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_HOM_APP},
					}
				}
			},
			{
				.head = {.type = NTOK_EXPR_VAR},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_ID_ELIGIBLE},
					}
				}
			},
			{
				.head = {.type = NTOK_EXPR_VAR},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPR_VAR},
						{ .type = TTOK_UN_PERIOD},
						{ .type = TTOK_ID_PLAIN},
					}
				}
			},
			{
				.head = {.type = NTOK_EXPR_VAR},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPR_VAR},
						{ .type = TTOK_ARROW},
						{ .type = TTOK_ID_PLAIN},
					}
				}
			},
			{
				.head = {.type = NTOK_EXPR_VAR},
				.body = (scfe_tkn_arr){
					.len = 4,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPR_VAR},
						{ .type = TTOK_SQUAREL},
						{ .type = NTOK_EXPRESSION},
						{ .type = TTOK_SQUARER},
					}
				}
			},
			{
				.head = {.type = NTOK_EXPR_PRIMARY},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPR_VAR},
					}
				}
			},
			{
				.head = {.type = NTOK_EXPR_PRIMARY},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_SQUARER},
						{ .type = TTOK_ID_PLAIN},
						{ .type = TTOK_SQUAREL},
					}
				}
			},
			{
				.head = {.type = NTOK_EXPR_PRIMARY},
				.body = (scfe_tkn_arr){
					.len = 2,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_EXCLAIM},
						{ .type = NTOK_EXPRESSION},
					}
				}
			},

			{
				.head = {.type = NTOK_EXPR_SECONDARY},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPR_PRIMARY },
					}
				}
			},
			{
				.head = {.type = NTOK_EXPR_SECONDARY},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPR_SECONDARY },
						{ .type = TTOK_BIN_MUL },
						{ .type = NTOK_EXPR_PRIMARY },
					}
				}
			},
			{
				.head = {.type = NTOK_EXPR_SECONDARY},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPR_SECONDARY },
						{ .type = TTOK_BIN_DIV},
						{ .type = NTOK_EXPR_PRIMARY },
					}
				}
			},

			{
				.head = {.type = NTOK_EXPR_TERTIARY},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPR_SECONDARY },
					}
				}
			},
			{
				.head = {.type = NTOK_EXPR_TERTIARY},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPR_TERTIARY },
						{ .type = TTOK_BIN_ADD},
						{ .type = NTOK_EXPR_SECONDARY },
					}
				}
			},
			{
				.head = {.type = NTOK_EXPR_TERTIARY},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPR_TERTIARY },
						{ .type = TTOK_BIN_SUB},
						{ .type = NTOK_EXPR_SECONDARY },
					}
				}
			},
			{
				.head = {.type = NTOK_EXPR_TERTIARY},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPR_TERTIARY },
						{ .type = TTOK_BIN_MOD},
						{ .type = NTOK_EXPR_SECONDARY },
					}
				}
			},

			{
				.head = {.type = NTOK_EXPRESSION},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPR_TERTIARY },
					}
				}
			},
			{
				.head = {.type = NTOK_EXPRESSION},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_LIT_STRING },
					}
				}
			},
			{
				.head = {.type = NTOK_EXPRESSION},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPRESSION},
						{ .type = TTOK_KW_CAT},
						{ .type = NTOK_EXPRESSION},
					}
				}
			},
			{
				.head = {.type = NTOK_EXPRESSION},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_CURLYL},
						{ .type = NTOK_EXPR_LIST},
						{ .type = TTOK_CURLYR},
					}
				}
			},
			{
				.head = {.type = NTOK_EXPRESSION},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPRESSION },
						{ .type = TTOK_BIN_LT},
						{ .type = NTOK_EXPRESSION},
					}
				}
			},
			{
				.head = {.type = NTOK_EXPRESSION},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPRESSION },
						{ .type = TTOK_BIN_LTE},
						{ .type = NTOK_EXPRESSION},
					}
				}
			},
			{
				.head = {.type = NTOK_EXPRESSION},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPRESSION },
						{ .type = TTOK_BIN_GT},
						{ .type = NTOK_EXPRESSION},
					}
				}
			},
			{
				.head = {.type = NTOK_EXPRESSION},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPRESSION },
						{ .type = TTOK_BIN_GTE},
						{ .type = NTOK_EXPRESSION},
					}
				}
			},
			{
				.head = {.type = NTOK_EXPRESSION},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPRESSION },
						{ .type = TTOK_BIN_EQ},
						{ .type = NTOK_EXPRESSION},
					}
				}
			},
			{
				.head = {.type = NTOK_EXPRESSION},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPRESSION },
						{ .type = TTOK_BIN_NEQ},
						{ .type = NTOK_EXPRESSION},
					}
				}
			},
			{
				.head = {.type = NTOK_EXPRESSION},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPRESSION },
						{ .type = TTOK_KW_AND},
						{ .type = NTOK_EXPRESSION},
					}
				}
			},
			{
				.head = {.type = NTOK_EXPRESSION},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPRESSION },
						{ .type = TTOK_KW_OR},
						{ .type = NTOK_EXPRESSION},
					}
				}
			},
			{
				.head = {.type = NTOK_EXPRESSION},
				.body = (scfe_tkn_arr){
					.len = 2,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_KW_NOT},
						{ .type = NTOK_EXPRESSION },
					}
				}
			},
			{
				.head = {.type = NTOK_EXPRESSION},
				.body = (scfe_tkn_arr){
					.len = 2,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_UN_AMPERSAND},
						{ .type = NTOK_EXPRESSION},
					}
				}
			},
			{
				.head = {.type = NTOK_EXPRESSION},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_CAST}
					}
				}
			},

			{
				.head = {.type = NTOK_EXPR_LIST},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPRESSION},
					}
				}
			},
			{
				.head = {.type = NTOK_EXPR_LIST},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_EXPR_LIST},
						{ .type = TTOK_COMMA},
						{ .type = NTOK_EXPRESSION},
					}
				}
			},

			// identifier rules
			{
				.head = {.type = NTOK_ID_ELIGIBLE },
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_ID_PLAIN },
					}
				}
			},
			{
				.head = {.type = NTOK_ID_ELIGIBLE },
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_ID_QUALIFIED },
					}
				}
			},

			// type expression rules
			{
				.head = {.type = NTOK_TYPE_EXPR},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_ID_ELIGIBLE },
					}
				}
			},
			{
				.head = {.type = NTOK_TYPE_EXPR},
				.body = (scfe_tkn_arr){
					.len = 2,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_KW_PTR},
						{ .type = NTOK_TYPE_EXPR},
					}
				}
			},
			{
				.head = {.type = NTOK_TYPE_EXPR},
				.body = (scfe_tkn_arr){
					.len = 4,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_TYPE_EXPR},
						{ .type = TTOK_SQUAREL},
						{ .type = TTOK_LIT_INT},
						{ .type = TTOK_SQUARER},
					}
				}
			},
			{
				.head = {.type = NTOK_CAST},
				.body = (scfe_tkn_arr){
					.len = 4,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_KW_CAST},
						{ .type = NTOK_EXPRESSION},
						{ .type = TTOK_KW_AS},
						{ .type = NTOK_TYPE_EXPR},
					}
				}
			},


			// statement rules
			{
				.head = {.type = NTOK_STMT_RETURN},
				.body = (scfe_tkn_arr){
					.len = 2,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_KW_RETURN },
						{ .type = NTOK_EXPRESSION },
					}
				}
			},
			{
				.head = {.type = NTOK_STMT_DECLARE},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_KW_BIND},
						{ .type = NTOK_TYPE_EXPR},
						{ .type = TTOK_ID_PLAIN},
					}
				}
			},
			{
				.head = {.type = NTOK_STMT_DECLARE},
				.body = (scfe_tkn_arr){
					.len = 5,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_KW_BIND},
						{ .type = NTOK_TYPE_EXPR},
						{ .type = TTOK_ID_PLAIN},
						{ .type = TTOK_SET_EQ},
						{ .type = NTOK_EXPRESSION},
					}
				}
			},
			{
				.head = {.type = NTOK_LVAL },
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_ID_PLAIN},
					}
				}
			},
			{
				.head = {.type = NTOK_LVAL },
				.body = (scfe_tkn_arr){
					.len = 2,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_EXCLAIM},
						{ .type = NTOK_LVAL},
					}
				}
			},
			{
				.head = {.type = NTOK_LVAL },
				.body = (scfe_tkn_arr){
					.len = 4,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_LVAL},
						{ .type = TTOK_SQUAREL},
						{ .type = NTOK_EXPRESSION},
						{ .type = TTOK_SQUARER},
					}
				}
			},
			{
				.head = {.type = NTOK_LVAL },
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_LVAL},
						{ .type = TTOK_UN_PERIOD},
						{ .type = TTOK_ID_PLAIN},
					}
				}
			},
			{
				.head = {.type = NTOK_LVAL },
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_LVAL},
						{ .type = TTOK_ARROW},
						{ .type = TTOK_ID_PLAIN},
					}
				}
			},
			{
				.head = {.type = NTOK_STMT_ASSIGN},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_LVAL},
						{ .type = TTOK_SET_EQ},
						{ .type = NTOK_EXPRESSION},
					}
				}
			},
			{
				.head = {.type = NTOK_STMT_CALL},
				.body = (scfe_tkn_arr){
					.len = 2,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_KW_CALL},
						{ .type = NTOK_PROC_APP},
					}
				}
			},
			{
				.head = {.type = NTOK_STMT_SYNTH},
				.body = (scfe_tkn_arr){
					.len = 2,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_KW_SYNTH},
						{ .type = NTOK_HOM_APP},
					}
				}
			},
			{
				.head = {.type = NTOK_STMT_DO},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_KW_DO},
						{ .type = NTOK_BLOCK},
						{ .type = TTOK_KW_END},
					}
				}
			},
			{
				.head = {.type = NTOK_STMT_IF},
				.body = (scfe_tkn_arr){
					.len = 5,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_KW_IF},
						{ .type = NTOK_EXPRESSION},
						{ .type = TTOK_KW_THEN},
						{ .type = NTOK_BLOCK},
						{ .type = TTOK_KW_END},
					}
				}
			},
			{
				.head = {.type = NTOK_STMT_IF},
				.body = (scfe_tkn_arr){
					.len = 7,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_KW_IF},
						{ .type = NTOK_EXPRESSION},
						{ .type = TTOK_KW_THEN},
						{ .type = NTOK_BLOCK},
						{ .type = TTOK_KW_ELSE},
						{ .type = NTOK_BLOCK},
						{ .type = TTOK_KW_END},
					}
				}
			},
			{
				.head = {.type = NTOK_STMT_WHILE},
				.body = (scfe_tkn_arr){
					.len = 5,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_KW_WHILE},
						{ .type = NTOK_EXPRESSION},
						{ .type = TTOK_KW_DO},
						{ .type = NTOK_BLOCK},
						{ .type = TTOK_KW_END},
					}
				}
			},
			{
				.head = {.type = NTOK_STATEMENT},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_STMT_DECLARE }
					}
				}
			},
			{
				.head = {.type = NTOK_STATEMENT},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_STMT_ASSIGN}
					}
				}
			},
			{
				.head = {.type = NTOK_STATEMENT},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_STMT_CALL}
					}
				}
			},
			{
				.head = {.type = NTOK_STATEMENT},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_STMT_SYNTH}
					}
				}
			},
			{
				.head = {.type = NTOK_STATEMENT},
				.body = (scfe_tkn_arr){
					.len = 5,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_SQUARER},
						{ .type = TTOK_SQUARER},
						{ .type = TTOK_ID_PLAIN},
						{ .type = TTOK_SQUAREL},
						{ .type = TTOK_SQUAREL},
					}
				}
			},
			{
				.head = {.type = NTOK_STATEMENT},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_STMT_DO}
					}
				}
			},
			{
				.head = {.type = NTOK_STATEMENT},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_STMT_IF}
					}
				}
			},
			{
				.head = {.type = NTOK_STATEMENT},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_STMT_WHILE}
					}
				}
			},


			// block rules
			{
				.head = {.type = NTOK_STMT_LIST},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_STATEMENT }
					}
				}
			},
			{
				.head = {.type = NTOK_STMT_LIST},
				.body = (scfe_tkn_arr){
					.len = 2,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_STMT_LIST },
						{ .type = NTOK_STMT_LIST },
					}
				}
			},
			{
				.head = {.type = NTOK_BLOCK},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_STMT_LIST }
					}
				}
			},
			{
				.head = {.type = NTOK_BLOCK},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_STMT_RETURN }
					}
				}
			},
			{
				.head = {.type = NTOK_BLOCK},
				.body = (scfe_tkn_arr){
					.len = 2,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_STMT_LIST },
						{ .type = NTOK_STMT_RETURN }
					}
				}
			},

			// procedure rules
			{
				.head = {.type = NTOK_PROC_ARG_DECL},
				.body = (scfe_tkn_arr){
					.len = 2,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_TYPE_EXPR },
						{ .type = TTOK_ID_PLAIN }
					}
				}
			},
			{
				.head = {.type = NTOK_PROC_ARG_DECLS},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_PROC_ARG_DECL},
					}
				}
			},
			{
				.head = {.type = NTOK_PROC_ARG_DECLS},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_PROC_ARG_DECLS},
						{ .type = TTOK_COMMA},
						{ .type = NTOK_PROC_ARG_DECL},
					}
				}
			},
			{
				.head = {.type = NTOK_PROC_ARGS},
				.body = (scfe_tkn_arr){
					.len = 2,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_PARENL},
						{ .type = TTOK_PARENR},
					}
				}
			},
			{
				.head = {.type = NTOK_PROC_ARGS},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_PARENL},
						{ .type = NTOK_PROC_ARG_DECLS},
						{ .type = TTOK_PARENR},
					}
				}
			},
			{
				.head = {.type = NTOK_PROC_IMP },
				.body = (scfe_tkn_arr){
					.len = 5,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_KW_IMPORT },
						{ .type = TTOK_KW_PROC },
						{ .type = TTOK_ID_QUALIFIED},
						{ .type = NTOK_TYPE_EXPR},
						{ .type = NTOK_PROC_ARGS},
					}
				}
			},
			{
				.head = {.type = NTOK_PROC_DEF },
				.body = (scfe_tkn_arr){
					.len = 6,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_KW_PROC },
						{ .type = TTOK_ID_QUALIFIED},
						{ .type = NTOK_TYPE_EXPR},
						{ .type = NTOK_PROC_ARGS},
						{ .type = NTOK_BLOCK},
						{ .type = TTOK_KW_END},
					}
				}
			},
			{
				.head = {.type = NTOK_PROC_DEF },
				.body = (scfe_tkn_arr){
					.len = 7,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_KW_EXPORT},
						{ .type = TTOK_KW_PROC },
						{ .type = TTOK_ID_QUALIFIED},
						{ .type = NTOK_TYPE_EXPR},
						{ .type = NTOK_PROC_ARGS},
						{ .type = NTOK_BLOCK},
						{ .type = TTOK_KW_END},
					}
				}
			},
			{
				.head = {.type = NTOK_PROC_APP },
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_ID_ELIGIBLE},
						{ .type = TTOK_PARENL},
						{ .type = TTOK_PARENR},
					}
				}
			},
			{
				.head = {.type = NTOK_PROC_APP },
				.body = (scfe_tkn_arr){
					.len = 4,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_ID_ELIGIBLE},
						{ .type = TTOK_PARENL},
						{ .type = NTOK_EXPR_LIST},
						{ .type = TTOK_PARENR},
					}
				}
			},
			{
				.head = {.type = NTOK_HOM_ARG_DECL},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_ID_ELIGIBLE}
					}
				}
			},
			{
				.head = {.type = NTOK_HOM_ARG_DECL},
				.body = (scfe_tkn_arr){
					.len = 2,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_ID_ELIGIBLE},
						{ .type = TTOK_ID_PLAIN}
					}
				}
			},
			{
				.head = {.type = NTOK_HOM_ARG_DECLS},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_HOM_ARG_DECL},
					}
				}
			},
			{
				.head = {.type = NTOK_HOM_ARG_DECLS},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_HOM_ARG_DECLS},
						{ .type = TTOK_COMMA},
						{ .type = NTOK_HOM_ARG_DECL},
					}
				}
			},
			{
				.head = {.type = NTOK_HOM_DEF },
				.body = (scfe_tkn_arr){
					.len = 7,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_KW_HOM},
						{ .type = TTOK_ID_QUALIFIED},
						{ .type = TTOK_PARENL},
						{ .type = NTOK_HOM_ARG_DECLS},
						{ .type = TTOK_PARENR},
						{ .type = NTOK_EXPRESSION},
						{ .type = TTOK_KW_END},
					}
				}
			},
			{
				.head = {.type = NTOK_HOM_DEF },
				.body = (scfe_tkn_arr){
					.len = 7,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_KW_HOM},
						{ .type = TTOK_ID_QUALIFIED},
						{ .type = TTOK_PARENL},
						{ .type = NTOK_HOM_ARG_DECLS},
						{ .type = TTOK_PARENR},
						{ .type = NTOK_BLOCK},
						{ .type = TTOK_KW_END},
					}
				}
			},
			{
				.head = {.type = NTOK_WEAVE},
				.body = (scfe_tkn_arr){
					.len = 2,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_KW_WEAVE},
						{ .type = NTOK_ID_ELIGIBLE},
					}
				}
			},
			{
				.head = {.type = NTOK_WEAVES},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_WEAVE},
					}
				}
			},
			{
				.head = {.type = NTOK_WEAVES},
				.body = (scfe_tkn_arr){
					.len = 2,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_WEAVES},
						{ .type = NTOK_WEAVE},
					}
				}
			},
			{
				.head = {.type = NTOK_HOM_APP },
				.body = (scfe_tkn_arr){
					.len = 4,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_ID_ELIGIBLE},
						{ .type = TTOK_PARENL},
						{ .type = TTOK_HOM_APP},
						{ .type = TTOK_PARENR},
					}
				}
			},
			{
				.head = {.type = NTOK_HOM_APP },
				.body = (scfe_tkn_arr){
					.len = 5,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_ID_ELIGIBLE},
						{ .type = NTOK_WEAVES},
						{ .type = TTOK_PARENL},
						{ .type = TTOK_HOM_APP},
						{ .type = TTOK_PARENR},
					}
				}
			},

			// struct rules
			{
				.head = {.type = NTOK_STRUCT_MEM},
				.body = (scfe_tkn_arr){
					.len = 2,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_TYPE_EXPR},
						{ .type = TTOK_ID_PLAIN},
					}
				}
			},
			{
				.head = {.type = NTOK_STRUCT_MEM_LIST},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_STRUCT_MEM},
					}
				}
			},
			{
				.head = {.type = NTOK_STRUCT_MEM_LIST},
				.body = (scfe_tkn_arr){
					.len = 2,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_STRUCT_MEM_LIST},
						{ .type = NTOK_STRUCT_MEM},
					}
				}
			},
			{
				.head = {.type = NTOK_STRUCT_DEF},
				.body = (scfe_tkn_arr){
					.len = 4,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_KW_STRUCT},
						{ .type = TTOK_ID_QUALIFIED},
						{ .type = NTOK_STRUCT_MEM_LIST},
						{ .type = TTOK_KW_END},
					}
				}
			},

			// aspect rules
			{
				.head = {.type = NTOK_ADVICE},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_ID_ELIGIBLE},
						{ .type = TTOK_KW_BEFORE},
						{ .type = NTOK_ID_ELIGIBLE},
					}
				}
			},
			{
				.head = {.type = NTOK_ADVICE},
				.body = (scfe_tkn_arr){
					.len = 3,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_ID_ELIGIBLE},
						{ .type = TTOK_KW_AFTER},
						{ .type = NTOK_ID_ELIGIBLE},
					}
				}
			},
			{
				.head = {.type = NTOK_ADVICE_LIST},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_ADVICE},
					}
				}
			},
			{
				.head = {.type = NTOK_ADVICE_LIST},
				.body = (scfe_tkn_arr){
					.len = 2,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_ADVICE_LIST},
						{ .type = NTOK_ADVICE},
					}
				}
			},
			{
				.head = {.type = NTOK_ASPECT_DEF},
				.body = (scfe_tkn_arr){
					.len = 4,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_KW_ASPECT},
						{ .type = TTOK_ID_QUALIFIED},
						{ .type = NTOK_ADVICE_LIST},
						{ .type = TTOK_KW_END},
					}
				}
			},


			// using statement rules
			{
				.head = {.type = NTOK_USING_STMT },
				.body = (scfe_tkn_arr){
					.len = 2,
					.addr = (scfe_tkn[]){
						{ .type = TTOK_KW_USING },
						{ .type = TTOK_ID_PLAIN },
					}
				}
			},

			// chunk rules
			{
				.head = {.type = NTOK_CHUNK},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_USING_STMT },
					}
				}
			},
			{
				.head = {.type = NTOK_CHUNK},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_PROC_IMP},
					}
				}
			},
			{
				.head = {.type = NTOK_CHUNK},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_PROC_DEF},
					}
				}
			},
			{
				.head = {.type = NTOK_CHUNK},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_HOM_DEF},
					}
				}
			},
			{
				.head = {.type = NTOK_CHUNK},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_STRUCT_DEF},
					}
				}
			},
			{
				.head = {.type = NTOK_CHUNK},
				.body = (scfe_tkn_arr){
					.len = 1,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_ASPECT_DEF},
					}
				}
			},
			{
				.head = {.type = NTOK_CHUNK},
				.body = (scfe_tkn_arr){
					.len = 2,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_CHUNK},
						{ .type = NTOK_CHUNK},
					}
				}
			},
			{
				.head = {.type = NTOK_START},
				.body = (scfe_tkn_arr){
					.len = 2,
					.addr = (scfe_tkn[]){
						{ .type = NTOK_CHUNK},
						{ .type = TTOK_OVER},
					}
				}
			},
		}
	}
};


#ifndef errfmt_h
#define errfmt_h

#include <stdio.h>
#include <stdlib.h>

#define ERRFMT2 "%s: unsupported syn rule %i\n"
#define FATAL_UNDEF_SYN_RULE(rule) FATAL_ERROR(2, rule)

#define ERRFMT3 "%s: reference to undefined variable '%.*s'\n"
#define FATAL_UNDEF_VAR(name) FATAL_ERROR(3, name.len, name.addr)

#define ERRFMT4 "%s: '%s' bad types '%i' and '%i'\n"
#define FATAL_BAD_TYPES(op, t1, t2) FATAL_ERROR(4, op, t1, t2)

#define ERRFMT5 "%s: bad namespace '%.*s'\n"
#define FATAL_BAD_NS(ns) FATAL_ERROR(5, ns.len, ns.addr)

#define ERRFMT6 "%s: reference to undefined type '%.*s'\n"
#define FATAL_UNDEF_TYPE(name) FATAL_ERROR(6, name.len, name.addr)

#define ERRFMT7 "%s: %s\n"
#define FATAL_LLVM_ERR(msg) FATAL_ERROR(7, msg)


#define ERRFMT8 "%s: unsupported sem rule %i\n"
#define FATAL_UNDEF_SEM_RULE(rule) FATAL_ERROR(8, rule)

#define ERRFMT9 "%s: %s does not support type %i\n"
#define FATAL_BAD_TYPE(fn, rule) FATAL_ERROR(9, fn, rule)

#define ERRFMT10 "%s: sunsupported: %s\n"
#define FATAL_GENERIC_UNSUPPORTED(msg) FATAL_ERROR(10, msg)

#define ERRFMT11 "%s: type error calling '%.*s': incorrect number of arguments\n"
#define FATAL_CALL_BADLEN(msg) FATAL_ERROR(11, msg.len, msg.addr)

#define ERRFMT12 "%s: type error calling '%.*s': argument %i has bad type\n"
#define FATAL_CALL_BADTYPE(fn, idx) FATAL_ERROR(12, fn.len, fn.addr, idx)

#define ERRFMT13 "%s: type error assigning into '%.*s'\n"
#define FATAL_ASSIGN_BADTYPE(fn) FATAL_ERROR(13, fn.len, fn.addr)

#define ERRFMT14 "%s: unsupported syntax type expr rule: %i\n"
#define FATAL_UNSUP_TYPE(ty) FATAL_ERROR(14, ty)

#define ERRFMT15 "%s: unsupported lval syntax: %i\n"
#define FATAL_UNSUP_LVAL(rule) FATAL_ERROR(15, rule)

#define ERRFMT16 "%s: nonexistant member: %.*s\n"
#define FATAL_BAD_MEMBER(mem) FATAL_ERROR(16, mem.len, mem.addr)

#define ERRFMT17 "%s: %s: %s\n"
#define FATAL_LLVM_ERR2(msg, msg2) FATAL_ERROR(17, msg, msg2)

#define ERRFMT18 "%s: unsupported variable astkind: %i\n"
#define FATAL_UNSUP_VAR(rule) FATAL_ERROR(18, rule)

#define ERRFMT19 "%s: unknown option: %s\n"
#define FATAL_BAD_OPT(opt) FATAL_ERROR(19, opt)

#define ERRFMT20 "%s: bad access with '%.*s'\n"
#define FATAL_BAD_ACC(mem) FATAL_ERROR(20, mem.len, mem.addr)

#define ERRFMT21 "%s: index into something that's not an array%s"
#define FATAL_BAD_INDEX() FATAL_ERROR(21, "\n")

#define ERRFMT22 "%s: index on non-indexable%s"
#define FATAL_NONINDEXABLE() FATAL_ERROR(22, "\n")

#define ERRFMT23 "%s: if statement with bad type%s"
#define FATAL_BAD_CONDITIONAL() FATAL_ERROR(23, "\n")

#define ERRFMT24 "%s: bad comparison%s"
#define FATAL_BAD_CMP() FATAL_ERROR(24, "\n")

#define ERRFMT25 "%s: unsupported cast: %i\n"
#define FATAL_UNSUPPORTED_CAST(t) FATAL_ERROR(25, t)

#define ERRFMT26 "%s: bad chunk idk%s"
#define FATAL_BAD_CHUNK(t) FATAL_ERROR(26, "\n")

#define ERRFMT27 "%s: plumbing error%s"
#define FATAL_PLUMBING(t) FATAL_ERROR(27, "\n")

#define ERRFMT28 "%s: attr does not exist: %.*s\n"
#define FATAL_BAD_RULE(rule) FATAL_ERROR(28, rule.len, rule.addr)

#define ERRFMT29 "%s: legacy zero index: '%.*s'%s"
#define FATAL_LEGACY_ZI(rule) FATAL_ERROR(29, rule.len, rule.addr, "\n")

#define ERRFMT30 "%s: exotic seam?%s"
#define FATAL_BAD_SEAM() FATAL_ERROR(30, "\n")

#define ERRFMT31 "%s: nonexistant aspect: %.*s\n"
#define FATAL_BAD_ASPECT(asp) FATAL_ERROR(31, asp.len, asp.addr)


#define FATAL_ERROR(code, ...) { \
	fprintf(stderr, ERRFMT##code, __FUNCTION__, __VA_ARGS__);\
	exit(code);\
}

#endif

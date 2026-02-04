#ifndef raec_h_
#define raec_h_

#include "codegen.h"
#include <mf/memory.h>
#include <mf/string.h>

// read from src write to dst
int raec(arena *a, arena tmp, string src, cgopt opt);

#endif

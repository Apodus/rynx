
#pragma once

#ifdef RYNX_CODEGEN
#define ANNOTATE(s) __attribute__((annotate(s)))
#else
#define ANNOTATE(...)
#endif

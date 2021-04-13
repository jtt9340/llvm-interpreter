#ifndef UTIL_H
#define UTIL_H

#include "KaleidoscopeJIT.h"     // llvm::orc::KaleidoscopeJIT

static std::unique_ptr<llvm::orc::KaleidoscopeJIT> JIT;
#endif

#ifndef LOGGING_H
#define LOGGING_H

#include <llvm/IR/Value.h> // llvm::Value

#include "ast.h"           // ExprAST, PrototypeAST

/// These are basic helper functions for basic error handling.
std::unique_ptr<ExprAST> LogError(const char *Str);

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str);

llvm::Value *LogErrorV(const char *Str);

#endif

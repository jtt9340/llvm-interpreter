#ifndef LOGGING_H
#define LOGGING_H

#include "ast.h"  // ExprAST, ProtortypeAST

/// These are basic helper functions for basic error handling.
std::unique_ptr<ExprAST> LogError(const char *Str);

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str);

#endif

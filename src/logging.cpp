#include <cstdio> // std::fprintf

#include "logging.h"
// TODO - make error reports more user friendly

std::unique_ptr<ExprAST> LogError(const char *Str) {
  std::fprintf(stderr, "LogError: %s\n", Str);
  return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
  LogError(Str);
  return nullptr;
}

llvm::Value *LogErrorV(const char *Str) {
  LogError(Str);
  return nullptr;
}

#include <sstream> // std::ostringstream

#include "VariableExprAST.h"

/// The constructor for the VariableExprAST class.
VariableExprAST::VariableExprAST(const std::string &Name) : Name(Name) {}

/// Generate LLVM IR for a variable reference.
llvm::Value *VariableExprAST::codegen() {
  // Look this variable up in the function.
  llvm::Value *V = getNamedValues().at(Name);

  if (!V) {
    std::ostringstream errMsg("Unknown variable name: ", std::ios_base::ate);
    errMsg << Name;
    return LogErrorV(errMsg.str().c_str());
  }

  return getBuilder().CreateLoad(V, Name.c_str());
}

/// "NumberExprAST(%s)"
std::string VariableExprAST::toString(const unsigned depth) const {
  std::ostringstream repr;
  insert_indent(repr, depth);
  repr << "VariableExprAST(" << Name << ')';
  return repr.str();
}

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
std::string VariableExprAST::toString() const {
  std::ostringstream repr("VariableExprAST(", std::ios_base::ate);
  repr << Name << ')';
  return repr.str();
}

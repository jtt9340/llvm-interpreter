#include <sstream> // std::ostringstream

#include "CallExprAST.h"

using std::size_t;

CallExprAST::CallExprAST(const std::string &Callee,
                         std::vector<std::unique_ptr<ExprAST>> Args)
    : Callee(Callee), Args(std::move(Args)) {}

/// Generate LLVM IR for a function call.
llvm::Value *CallExprAST::codegen() {
  // A string stream for holding potential error messages.
  std::ostringstream errMsg;
  // Look up the function name in the global module table.
  llvm::Function *CalleeF = getFunction(Callee);
  if (!CalleeF) {
    errMsg << "Unknown function referenced: " << Callee;
    return LogErrorV(errMsg.str().c_str());
  }

  // Handle argument mismatch error.
  const size_t expected = CalleeF->arg_size();
  const size_t actual = Args.size();
  if (expected != actual) {
    errMsg << "Wrong number of arguments passed to " << Callee << ", expecting "
           << expected << " but got " << actual;
    return LogErrorV(errMsg.str().c_str());
  }

  std::vector<llvm::Value *> ArgsV;
  for (unsigned i = 0; i < actual; i++) {
    ArgsV.push_back(Args[i]->codegen());
    if (!ArgsV.back())
      return nullptr;
  }

  return getBuilder().CreateCall(CalleeF, ArgsV, "calltmp");
}

/// "CallExprAST(function(arg0, arg1, ..., argn))"
std::string CallExprAST::toString(const unsigned depth) const {
  std::ostringstream repr;
  insert_indent(repr, depth);
  repr << "CallExprAST(" << Callee << '(';
  for (auto it = Args.begin(); it != Args.end(); it++) {
    repr << (*it)->toString();
    if (it != Args.end() - 1)
      repr << ", ";
  }
  repr << "))";
  return repr.str();
}

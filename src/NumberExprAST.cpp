#include <sstream> // std::ostringstream

#include "NumberExprAST.h"

/// The constructor for the NumberExprAST class.
NumberExprAST::NumberExprAST(double Val) : Val(Val) {}

/// Generate LLVM IR for a numeric constant.
llvm::Value *NumberExprAST::codegen() {
  // ConstantFP -> holds a compile-time floating point
  //               constant, represeted by a...
  // APFloat    -> Arbitrary Precision Float
  return llvm::ConstantFP::get(getContext(), llvm::APFloat(Val));
}

/// "NumberExprAST(%f)"
std::string NumberExprAST::toString() const {
  std::ostringstream repr("NumberExprAST(", std::ios_base::ate);
  repr << Val << ')';
  return repr.str();
}

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
std::string NumberExprAST::toString(const unsigned depth) const {
  std::ostringstream repr;
  insert_indent(repr, depth);
  repr << "NumberExprAST(" << Val << ')';
  return repr.str();
}

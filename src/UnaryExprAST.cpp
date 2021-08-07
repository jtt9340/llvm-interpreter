#include <sstream> // std::ostringstream

#include "UnaryExprAST.h"

using std::size_t;

/// The constuctor for the UnaryExprAST class.
UnaryExprAST::UnaryExprAST(char Opcode, std::unique_ptr<ExprAST> Operand)
    : Op(Opcode), Operand(std::move(Operand)) {}

/// Generate LLVM IR for a unary expression.
llvm::Value *UnaryExprAST::codegen() {
  llvm::Value *OperandValue = Operand->codegen();
  if (!OperandValue)
    return nullptr;

  llvm::Function *Operator = getFunction(std::string("unary") + Op);
  if (!Operator) {
    // "Unknown unary operator " is 23 characters + 1 for Op + 1 for NUL byte
    constexpr size_t errMsgBufSize = 25;
    char errMsg[errMsgBufSize];
    std::snprintf(errMsg, errMsgBufSize, "Unknown unary operator %c", Op);
    return LogErrorV(errMsg);
  }

  return getBuilder().CreateCall(Operator, OperandValue, "unop");
}

/// "op rhs"
std::string UnaryExprAST::toString(const unsigned depth) const {
  std::ostringstream repr;
  insert_indent(repr, depth);
  repr << Op << Operand->toString();
  return repr.str();
}

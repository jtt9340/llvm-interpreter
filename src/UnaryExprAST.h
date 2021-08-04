#include "ExprAST.h"

#ifndef UNARYEXPRAST_H
#define UNARYEXPRAST_H

/// UnaryExprAST - Expression class for a unary operator.
///
/// This AST node stores a unary operator and other AST node
/// that the unary operator acts upon.
class UnaryExprAST : public ExprAST {
  /// The particular operator.
  char Op;
  /// The operand of the unary operator.
  std::unique_ptr<ExprAST> Operand;

public:
  /// The constructor for the UnaryExprAST class. This constuctor takes in the
  /// operator that this unary expression represents as well as the AST node for
  /// operand being acted upon.
  ///
  /// @param Opcode the unary operator
  /// @param Operand the expression being acted upon
  UnaryExprAST(char Opcode, std::unique_ptr<ExprAST> Operand);

  /// Generate LLVM IR for a unary expression.
  llvm::Value *codegen() override;

  /// Return a helpful string representation of this UnaryExprAST, useful for
  /// debugging.
  ///
  /// @return a string of the form  "%c %s", where %c is the unary operator and
  /// %s is a string representation of the operand
  std::string toString() const override;
};

#endif // UNARYEXPRAST_H

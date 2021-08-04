#include "ExprAST.h"

#ifndef BINARYEXPRAST_H
#define BINARYEXPRAST_H

/// BinaryExprAST - Expression class for a binary operator.
///
/// This AST node stores a binary operator and two other AST nodes that the
/// binary operator acts upon.
class BinaryExprAST : public ExprAST {
  /// The particular operator.
  char Op;
  /// The two operands of the binary operator.
  std::unique_ptr<ExprAST> LHS, RHS;

public:
  /// The constructor for the BinaryExprAST class. This constuctor takes in
  /// the character representing the binary operator as well as the expressions
  /// on either side of the operator.
  ///
  /// @param op the binary operator
  /// @param LHS the left-hand-side of the binary operator
  /// @param RHS the right-hand-side of the binary operator
  BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS);

  /// Generate LLVM IR for a binary expression.
  llvm::Value *codegen() override;

  /// Return a helpful string representation of this BinaryExprAST, useful
  /// for debugging.
  ///
  /// @return a string of the form "%1$s %c %2$s", where %1$s is the string
  /// representation of the left side of this BinaryExprAST, %2$s is the string
  /// representation of the right side of this BinaryExprAST, and %c is
  /// the binary operator conjoining the left- and right-hand sides of this
  /// BinaryExprAST
  std::string toString() const override;
};

#endif // BINARYEXPRAST_H

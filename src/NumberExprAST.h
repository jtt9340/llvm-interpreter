#include "ExprAST.h"

#ifndef NUMBEREXPRAST_H
#define NUMBEREXPRAST_H

/// NumberExprAST - Expression class for numeric literals like "1.0".
///
/// This AST node just holds a single double value to represent a numeric
/// literal.
class NumberExprAST : public ExprAST {
  /// The particular number this NumberExprAST is storing.
  double Val;

public:
  /// The constructor for the NumberExprAST class.
  ///
  /// @param Val the numeric value that this node of the AST represents
  explicit NumberExprAST(double Val);

  /// Generate LLVM IR for a numeric constant.
  llvm::Value *codegen() override;

  /// Return a helpful string representation of this NumberExprAST useful
  /// for debugging.
  ///
  /// @return a string of the form "NumberExprAST(%f)", where %f is the value
  ///         that this NumberExprAST wraps
  std::string toString() const override;
};

#endif // NUMBEREXPRAST_H

#include "ExprAST.h"

#ifndef VARIABLEEXPRAST_H
#define VARIABLEEXPRAST_H

/// VariableExprAST - Expression class for referencing a variable, like "a".
///
/// This AST node just hold the name of a variable reference.
struct VariableExprAST : public ExprAST {
  /// The name of the variable.
  const std::string Name;

  /// The constructor for the VariableExprAST class.
  ///
  /// @param Name the name of the variable that this AST node
  /// represents
  explicit VariableExprAST(const std::string &Name);

  /// Generate LLVM IR for a variable reference.
  llvm::Value *codegen() override;

  /// Return a helpful string representation of this VariableExprAST useful
  /// for debugging.
  ///
  /// @return a string of the form "VariableExprAST(%s)", where %s is the name
  ///         of the variable that this VariableExprAST wraps
  std::string toString() const override;
};

#endif // VARIABLEEXPRAST_H

#include "ExprAST.h"

#ifndef LETEXPRAST_H
#define LETEXPRAST_H

/// LetExprAST - A "let"/"in" expression for defining local variables.
///
/// A LetExprAST node contains a vector of pairs mapping variable names
/// to values and a body that uses those variables.
class LetExprAST : public ExprAST {
  /// All the variable names paired with the values.
  /// For example the let statement
  ///
  ///   let
  ///     a = 1
  ///     b = 2
  ///   in
  ///
  /// would generate a value of
  ///
  ///   {{"a", NumberExprAST(1)}, {"b", NumberExprAST(2)}}
  ///
  /// for this field.
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;
  /// The code after the "in" keyword.
  std::unique_ptr<ExprAST> Body;

public:
  /// The constructor for the LetExprAST class.
  ///
  /// @param VarNames the variable names and values for the let/in expression
  /// @param Body the body of the let/in expression.
  LetExprAST(
      std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
      std::unique_ptr<ExprAST> Body);

  /// Generate LLVM IR for a let/in expression.
  llvm::Value *codegen() override;

  /// Return a helpful string representation of this LetExprAST node
  /// useful for debugging.
  ///
  /// @param depth the level of indentation to print this LetExprAST at,
  ///              useful for pretty-printing (may be ignored by implementation)
  /// @return a string of the form "LetExprAST(
  ///		%1$s = %2$s,
  ///		%3$s = %4$s,
  ///		...,
  ///		%n$s = %(n + 1)$s;
  ///		%s
  /// )" where %$is, where i is an odd integer, is the name of a variable,
  /// %$(i + 1)s, where i is an odd integer, is the string representation
  /// of the value of that variable, if given, and %s is the string
  /// representation of the body of this LetExprAST.
  std::string toString(const unsigned depth = 0) const override;
};

#endif // LETEXPRAST_H

#include "ExprAST.h"

#ifndef IFEXPRAST_H
#define IFEXPRAST_H

/// IfExprAST - Expression for if expressions (similar to C's ternary operator)
///
/// An if expression contains three components:
///
/// 	1. A condition (Cond)
/// 	2. A then-clause (Then)
/// 	3. An else-clause (Else)
///
/// The else-clause is not optional. This makes the if expression more like an
/// if expression in functional languages, where each branch of an if
/// statement must evaluate to a value, or like a tenrary operator in more
/// statement-oriented languages like C or Java.
class IfExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Cond; ///< The condition to evaluate
  std::unique_ptr<ExprAST> Then; ///< The expression to evaluate if Cond
                                 ///< evaluates to a non-zero value
  std::unique_ptr<ExprAST>
      Else; ///< The expression to evaluate if Cond evalutates to 0.0

public:
  /// The constructor for the IfExprAST class.
  ///
  /// @param Cond the condition that the if expression evaluates
  /// @param Then the code to evaluate if the condition is true
  /// @param Else the code to evaluate if the condition is false
  IfExprAST(std::unique_ptr<ExprAST> Cond, std::unique_ptr<ExprAST> Then,
            std::unique_ptr<ExprAST> Else);

  /// Generate LLVM IR for an if expression.
  llvm::Value *codegen() override;

  /// Return a helpful string representation of this IfExprAST node
  /// useful for debugging.
  ///
  /// @param depth the level of indentation to print this IfExprAST at,
  ///              useful for pretty-printing (may be ignored by implementation)
  /// @return a string of the form "IfExprAST(%1$s
  ///         	? %2$s
  ///         	: %3$s
  ///         )", where %1$s is the string representation of the condition
  ///         part of this IfExprAST, %2$s is the string representation of the
  ///         then-clause of this IfExprAST, and %$3s is the string
  ///         representation of the else-clause of this IfExprAST
  std::string toString(const unsigned depth = 0) const override;
};

#endif // IFEXPRAST_H

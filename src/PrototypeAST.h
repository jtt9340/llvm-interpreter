#include "util.h"

#ifndef PROTOTYPEAST_H
#define PROTOTYPEAST_H

/// PrototypeAST - This class represents the "Prototype" for a function,
/// which captures its name and its argument names (which inadvertently
/// captures the number of formal parameters for that function).
class PrototypeAST : public Showable {
  /// The name of the function.
  std::string Name;
  /// The names of the formal paramters of the function.
  std::vector<std::string> Args;
  /// Is this a unary or binary operator?
  bool IsOperator;
  /// The precedence if this is a binary operator, or 0 if
  /// this is not
  unsigned Precedence;

public:
  /// The constructor for the PrototypeAST class.
  ///
  /// @param Name the name of the function prototype
  /// @param Args the names of the parameters to the function being represented
  ///        by this prototype
  /// @param IsOperator whether or not this Prototype AST node represents a
  ///        unary or binary operator
  /// @param Precedence the precedence of this binary operator of this Prototype
  ///        AST node represents a binary operator
  PrototypeAST(const std::string &Name, std::vector<std::string> Args,
               bool IsOperator = false, unsigned Precedence = 0);

  /// Get the name of the function that this is a prototype for.
  const std::string &getName() const;

  /// Generate LLVM IR for a function prototype.
  llvm::Function *codegen();

  /// Return a helpful string representation of this PrototypeAST node
  /// useful for debugging.
  ///
  /// @return a string of the form "PrototypeAST(%1$s(%2$s, %3$s, ..., %n$s))"
  ///         where %1$s is the name of the function that this PrototypeAST
  ///         represents, and %2$s, %3$s, ..., %n$s are the names of the formal
  ///         parameters of this PrototypeAST
  std::string toString() const override;

  /// Is this a function prototype for a unary operator?
  ///
  /// The output of this funciton is opposite of isBinaryOp,
  /// since a user-defined operator cannot be both a unary and binary operator.
  bool isUnaryOp() const;

  /// Is this a function prototype for a binary operator?
  ///
  /// The output of this function is opposite of isUnaryOp,
  /// since a user-defined operator cannot be both a unary and binary operator.
  bool isBinaryOp() const;

  /// If this function prototype represents a unary or binary operator, then
  /// return the character for this operator. Otherwise, return the NUL byte.
  char getOperatorName() const;

  /// If this function prototype represents a binary operator, then return the
  /// precedence for operator. Otherwise return 0.
  unsigned getBinaryPrecedence() const;
};

#endif // PROTOTYPEAST_H

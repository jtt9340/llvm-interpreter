#include "ExprAST.h"

#ifndef CALLEXPRAST_H
#define CALLEXPRAST_H

/// CallExprAST - Expression class for function calls.
///
/// This AST node stores the name of the function being called and
/// the arguments being passed.
class CallExprAST : public ExprAST {
  /// The function being called.
  std::string Callee;
  /// The values passed to the function call.
  std::vector<std::unique_ptr<ExprAST>> Args;

public:
  /// The constructor for the CallExprAST class. This constuctor accepts the
  /// name of the function being called (callee) as well as all the values
  /// passed to the function.
  ///
  /// @param Callee the name of the function being called
  /// @param Args the arguments passed to the function
  CallExprAST(const std::string &Callee,
              std::vector<std::unique_ptr<ExprAST>> Args);

  /// Generate LLVM IR for a function call.
  llvm::Value *codegen() override;

  /// Return a helpful string representation of this CallExprAST node
  /// useful for debugging.
  ///
  /// @return a string of the form "CallExprAST(%1$s(%2$s, %3$s, ..., %n$s))"
  ///         where %1$s is the name of the function being called,
  ///         and %2$s, %3$s, ..., %n$s are the string representations of the
  ///         arguments being passed to this function call
  std::string toString() const override;
};

#endif // CALLEXPRAST_H

#include "util.h"

#ifndef FUNCTIONAST_H
#define FUNCTIONAST_H

/// FunctionAST - a function prototype coupled with the code of the function
/// to create a complete function definition.
class FunctionAST : public Showable {
  /// The function prototype that represents this function definition.
  std::unique_ptr<PrototypeAST> Proto;
  /// The AST that represents the code for this function definition.
  std::unique_ptr<ExprAST> Body;

public:
  /// The constructor for the FunctionAST class. This constructor takes in the
  /// function prototype part of this function definition followed by the
  /// code that defines the behavior of the function.
  ///
  /// @param Proto the function prototype for this function definition
  /// @param the body of the function
  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::unique_ptr<ExprAST> Body);

  /// Generate LLVM IR for a function definition.
  llvm::Function *codegen();

  /// Return a helpful string representation of this FunctionAST node useful
  /// for debugging.
  ///
  /// @return a string of the form "FunctionAST(
  ///         	%1$s,
  ///         	%2$s
  ///         )", where %1$s is the string representation of this FunctionAST's
  ///         prototype, and %2$s is the string representation of this
  ///         FunctionAST's body
  std::string toString() const override;
};

#endif // FUNCTIONAST_H

#include "ExprAST.h"

#ifndef FOREXPRAST_H
#define FOREXPRAST_H

/// ForExprAST - This class encapsulates the AST node for a for loop, which
/// looks like
///
/// for i = 0, i < 6.28319, 0.523599 in
/// 	sin(i)
///
/// The ForExprAST stores the iterator variable name ("i" in the example above),
/// the initializer expression, the conditional expression, and the "step" as
/// well as the body of the for loop. This for loop is very similar to C's for
/// loop, except if the "step" is omitted it is assumed to be 1.
class ForExprAST : public ExprAST {
  std::string VarName; ///< The name of the iterator variable, commonly "i"
  std::unique_ptr<ExprAST> Start; ///< The initializer expression
  std::unique_ptr<ExprAST> End; ///< The conditional expression that determines
                                ///< when the for loop will end
  std::unique_ptr<ExprAST>
      Step; ///< The value to increment the variable
            ///< represented by VarName by.
            ///< If negative, the variable will be decremented.
  std::unique_ptr<ExprAST> Body; ///< The code contained within the for loop

public:
  /// The constructor for the ForExprAST class. A for expression has the
  /// following:
  ///
  ///   1. An initializer expression, which declares an...
  ///   2. ...induction variable
  ///   3. A conditional expression that determines when the loop ends
  ///   4. An optional step that determines how much to increment the
  ///      induction variable by at the end of each iteration
  ///   5. A body (what gets run inside the loop)
  ///
  /// This constructor takes all five pieces of information to create
  /// an AST node represeing this.
  ///
  /// @param VarName the name of the induction variable
  /// @param Start AST node for the initial expression
  /// @param End AST node for the condition
  /// @param Step AST node of the value that the induction variable will be
  /// incremented by
  /// @param Body the body of the for expression
  ForExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Start,
             std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
             std::unique_ptr<ExprAST> Body);

  /// Generate LLVM IR for a for expression.
  llvm::Value *codegen() override;

  /// Return a helpful string representation of this ForExprAST node
  /// useful for debugging.
  ///
  /// @param depth the level of indentation to print this ForExprAST at,
  ///              useful for pretty-printing (may be ignored by implementation)
  /// @return a string of the form "ForExprAST(%1$s = %2$s, %$3s, %$4s,
  ///		%$5s
  /// )" where %1$s is the induction variable for this ForExprAST,
  /// %$2s is the string representation of the initializer expression,
  /// %$3s is the string representation of the conditional expression,
  /// %$4s is the string representation of the step amount, if present,
  /// and finally %$5s is the string representation of the body.
  std::string toString(const unsigned depth = 0) const override;
};

#endif // FOREXPRAST_H

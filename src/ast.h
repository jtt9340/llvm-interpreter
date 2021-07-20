#ifndef AST_H
#define AST_H

#include <llvm/IR/Instructions.h> // llvm::PHINode, llvm::AllocaInst
#include <llvm/IR/Value.h>        // llvm::Value

#include <sstream> // std::ostringstream
#include <vector>  // std::vector

/// An interface for objects that can convert themselves
/// into a string representation for debugging.
struct Showable {
  /// Return a string representation of this object.
  ///
  /// @return a string representation of this object useful for debugging
  virtual std::string toString() const {
    // Default implementation is to just return the memory
    // address of this object as a string
    std::ostringstream repr("Showable@", std::ios_base::ate);
    repr << std::hex << this;
    return repr.str();
  }
};

/// ExprAST - Base class for all expression nodes.
class ExprAST : public Showable {
public:
  /// The destructor for the ExprAST class. Since the ExprAST class is just an
  /// abstract base class for all possible nodes of our AST, this is just an
  /// empty destructor.
  virtual ~ExprAST() = default;

  /// Generate LLVM IR for this AST node and all dependent AST nodes.
  virtual llvm::Value *codegen() = 0;
};

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
  /// @return a string of the form "IfExprAST(%1$s
  ///         	? %2$s
  ///         	: %3$s
  ///         )", where %1$s is the string representation of the condition
  ///         part of this IfExprAST, %2$s is the string representation of the
  ///         then-clause of this IfExprAST, and %$3s is the string
  ///         representation of the else-clause of this IfExprAST
  std::string toString() const override;
};

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
  /// @return a string of the form "ForExprAST(%1$s = %2$s, %$3s, %$4s,
  ///		%$5s
  /// )" where %1$s is the induction variable for this ForExprAST,
  /// %$2s is the string representation of the initializer expression,
  /// %$3s is the string representation of the conditional expression,
  /// %$4s is the string representation of the step amount, if present,
  /// and finally %$5s is the string representation of the body.
  std::string toString() const override;
};

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
  std::string toString() const override;
};

/// Set up the internal module for the interpreter and initialize all
/// optimizations.
///
/// This function should be called before running the interpreter. It
/// will enable optimizations for generated function code and initialize
/// the LLVM module to store symbol names.
void InitializeModuleAndPassManager();

/// Search through the global llvm::Module for a function with the given name.
/// If no such function exists, generate a new function declaration, or return
/// nullptr if function code generation failed.
///
/// @param Name the name of the function to search for
/// @returns a function with the given name, or nullptr if the function didn't
///          exist and code generation for it failed
llvm::Function *getFunction(const std::string &Name);

/// Create an LLVM alloca instruction for storing a variable named VarName on
/// the stack inside of the given function. This alloca instruction will then
/// be turned into a register instruction by the mem2reg LLVM optimization,
/// which prevents the interpreter from having to use SSA (Static Single
/// Assignment. where every variable is written to exactly once and every
/// variable is declared before it is used) form to create local mutable
/// variables.
///
/// @param Function the function to create a local, stack-allocated variable for
/// @param VarName the name of the local variable
/// @return the alloca instruction
llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *Function,
                                         const std::string &VarName);

/// What to do when a function definition is encountered at the REPL.
void HandleDefinition();

/// What to do when an extern function delcaration is encountered at the REPL.
void HandleExtern();

/// What to do when any other expression that is not a function definition or
/// extern function declaration is encountered at the REPL.
void HandleTopLevelExpression();
#endif

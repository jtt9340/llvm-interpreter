#ifndef AST_H
#define AST_H

#include <llvm/IR/Value.h> // llvm::Value

#include <sstream>         // std::ostringstream
#include <vector>          // std::vector

/// An interface for objects that can convert themselves
/// into a string representation for debugging
struct Showable {
	/// Return a string representation of this object
	virtual std::string toString() {
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
	/// The destructor for the ExprAST class. Since ther ExprAST class is just an abstract
	/// base class for all possible nodes of our AST, this is just an empty destructor.
	virtual ~ExprAST() = default;

	/// Generate LLVM IR for this AST node and all dependent AST nodes.
	virtual llvm::Value *codegen() = 0;
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
	/// The particular number this NumberExprAST is storing.
	double Val;

public:
	NumberExprAST(double Val);

	llvm::Value *codegen() override;

	std::string toString() override;
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
	/// The name of the variable.
	std::string Name;

public:
	VariableExprAST(const std::string &Name);

	llvm::Value *codegen() override;

	std::string toString() override;
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
	/// The particular operator.
	char Op;
	/// The two operands of the binary operator.
	std::unique_ptr<ExprAST> LHS, RHS;

public:
	BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS,
			std::unique_ptr<ExprAST> RHS);

	llvm::Value *codegen() override;

	std::string toString() override;
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
	/// The function being called.
	std::string Callee;
	/// The values passed to the function call.
	std::vector<std::unique_ptr<ExprAST>> Args;

public:
	CallExprAST(const std::string &Callee,
			std::vector<std::unique_ptr<ExprAST>> Args);

	llvm::Value *codegen() override;

	std::string toString() override;
};

/// IfExprAST - Expression for if expressions (similar to C's ternary operator)
class IfExprAST : public ExprAST {
	std::unique_ptr<ExprAST> Cond; ///< The condition to evaluate
	std::unique_ptr<ExprAST> Then; ///< The expression to evaluate if Cond evaluates to a non-zero value
	std::unique_ptr<ExprAST> Else; ///< The expression to evaluate if Cond evalutates to 0.0

public:
	IfExprAST(std::unique_ptr<ExprAST> Cond,
			std::unique_ptr<ExprAST> Then,
			std::unique_ptr<ExprAST> Else);

	llvm::Value *codegen() override;

	std::string toString() override;
};

/// ForExprAST - This class encapsulates the AST node for a for loop, which looks like
///
/// for i = 0, i < 6.28319, 0.523599 in
/// 	sin(i)
///
/// The ForExprAST stores the iterator variable name ("i" in the example above),
/// the initializer expression, the conditional expression, and the "step" as well
/// as the body of the for loop. This for loop is very similar to C's for loop, except
/// if the "step" is omitted it is assumed to be 1.
class ForExprAST : public ExprAST {
	std::string VarName; ///< The name of the iterator variable, commonly "i"
	std::unique_ptr<ExprAST> Start; ///< The initializer expression
	std::unique_ptr<ExprAST> End; ///< The conditional expression that determines when the for loop will end
	/// The value to increment the variable represented by VarName by.
	/// If negative, the variable will be decremented.
	std::unique_ptr<ExprAST> Step; 
	std::unique_ptr<ExprAST> Body; ///< The code contained within the for loop

public:
	ForExprAST(const std::string &VarName,
			std::unique_ptr<ExprAST> Start,
			std::unique_ptr<ExprAST> End,
			std::unique_ptr<ExprAST> Step,
			std::unique_ptr<ExprAST> Body);

	llvm::Value *codegen() override;

	std::string toString() override;
};

/// PrototypeAST - This class represents the "Prototype" for a function,
/// which captures its name and its argument names (which inadvertently
/// captures the number of formal parameters for that function).
class PrototypeAST : public Showable {
	/// The name of the function.
	std::string Name;
	/// The names of the formal paramters of the function.
	std::vector<std::string> Args;

public:
	PrototypeAST(const std::string &Name, std::vector<std::string> Args);

	const std::string &getName() const;

	llvm::Function *codegen();

	std::string toString() override;
};

/// FunctionAST - a function prototype coupled with the code of the function
/// to create a complete function definition.
class FunctionAST : public Showable {
	/// The function prototype that represents this function definition.
	std::unique_ptr<PrototypeAST> Proto;
	/// The AST that represents the code for this function definition.
	std::unique_ptr<ExprAST> Body;
	
public:
	FunctionAST(std::unique_ptr<PrototypeAST> Proto,
			std::unique_ptr<ExprAST> Body);

	llvm::Function *codegen();

	std::string toString() override;
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

/// What to do when a function definition is encountered at the REPL.
void HandleDefinition();

/// What to do when an extern function delcaration is encountered at the REPL.
void HandleExtern();

/// What to do when any other expression that is not a function definition or
/// extern function declaration is encountered at the REPL.
void HandleTopLevelExpression();
#endif

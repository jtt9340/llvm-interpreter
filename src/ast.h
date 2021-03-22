#ifndef AST_H
#define AST_H

#include <string> // std::string
#include <vector> // std::vector

/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
	/// The destructor for the ExprAST class. Since ther ExprAST class is just an abstract
	/// base class for all possible nodes of our AST, this is just an empty destructor.
	virtual ~ExprAST() = default;
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
	/// The particular number this NumberExprAST is storing.
	double Val;

public:
	NumberExprAST(double Val);
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
	/// The name of the variable.
	std::string Name;

public:
	VariableExprAST(const std::string &Name);
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
};

/// PrototypeAST - This class represents the "Prototype" for a function,
/// which captures its name and its argument names (which inadvertently
/// captures the number of formal parameters for that function).
class PrototypeAST {
	/// The name of the function.
	std::string Name;
	/// The names of the formal paramters of the function.
	std::vector<std::string> Args;

public:
	PrototypeAST(const std::string &Name, std::vector<std::string> Args);

	const std::string &getName() const;
};

/// FunctionAST - a function prototype coupled with the code of the function
/// to create a complete function definition.
class FunctionAST {
	/// The function prototype that represents this function definition.
	std::unique_ptr<PrototypeAST> Proto;
	/// The AST that represents the code for this function definition.
	std::unique_ptr<ExprAST> Body;
	
public:
	FunctionAST(std::unique_ptr<PrototypeAST> Proto,
			std::unique_ptr<ExprAST> Body);
};
#endif

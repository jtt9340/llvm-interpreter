#include "ast.h"

/// The destructor for the ExprAST class. Since ther ExprAST class is just an abstract
/// base class for all possible nodes of our AST, this is just an empty destructor.
ExprAST::~ExprAST() {}

/// The constructor for the NumberExprAST class. This constructor just takes a single
/// parameter: the numeric value that this node of the AST represents.
NumberExprAST::NumberExprAST(double Val) : Val(Val) {}

/// The constructor for the VariableExprAST class. This constructor just takes a single
/// parameter: the name of the variable that this AST node represents.
VariableExprAST::VariableExprAST(const std::string &Name) : Name(Name) {}

/// The constructor for thr BinaryExprAST class. This constuctor takes in
/// the character representing the binary operator as well as the expressions
/// on either side of the operator.
BinaryExprAST::BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS,
		std::unique_ptr<ExprAST> RHS)
	: Op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

/// The constructor for the CallExprAST class. This constuctor accepts the name
/// of the function being called (callee) as well as all the values passed to the
/// function.
CallExprAST::CallExprAST(const std::string &Callee,
		std::vector<std::unique_ptr<ExprAST>> Args)
	: Callee(Callee), Args(std::move(Args)) {}

/// The constructor for the PrototypeAST class. A function prototype names the function
/// as well as the names of its arguments. This constructor takes the function name as
/// a string and the parameter names as a vector of strings.
PrototypeAST::PrototypeAST(const std::string &Name, std::vector<std::string> Args)
	: Name(Name), Args(std::move(Args)) {}

/// Getter for the "Name" field of instances of PrototypeAST.
const std::string &PrototypeAST::getName() const { return Name; }

/// The constructor for the FunctionAST class. This constructor takes in the
/// funciton prototype part of this function definition followed by the
/// code that defines the behavior of the function.
FunctionAST::FunctionAST(std::unique_ptr<PrototypeAST> Proto,
		std::unique_ptr<ExprAST> Body)
	: Proto(std::move(Proto)), Body(std::move(Body)) {}

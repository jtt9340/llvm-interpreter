#include <llvm/IR/LLVMContext.h> // llvm::LLVMContext
#include <llvm/IR/IRBuilder.h>   // llvm::IRBuilder
#include <llvm/IR/Module.h>      // llvm::Module

#include <unordered_map>         // std::unordered_map
#include <sstream>               // std::ostringstream

#include "ast.h"
#include "logging.h"             // LogErrorV

static llvm::LLVMContext Context;
static llvm::IRBuilder<> Builder(Context);
static std::unique_ptr<llvm::Module> Module;
static std::unordered_map<std::string, llvm::Value *> NamedValues;

/// The constructor for the NumberExprAST class. This constructor just takes a single
/// parameter: the numeric value that this node of the AST represents.
NumberExprAST::NumberExprAST(double Val) : Val(Val) {}

/// Generate LLVM IR for a numeric constant.
llvm::Value *NumberExprAST::codegen() {
	// ConstantFP -> holds a compile-time floating point constant, represeted by a...
	// APFloat -> Arbitrary Precision Float
	return llvm::ConstantFP::get(Context, llvm::APFloat(Val));
}

/// The constructor for the VariableExprAST class. This constructor just takes a single
/// parameter: the name of the variable that this AST node represents.
VariableExprAST::VariableExprAST(const std::string &Name) : Name(Name) {}

/// Generate LLVM IR for a variable reference.
llvm::Value *VariableExprAST::codegen() {
	// Look this variable up in the function.
	llvm::Value *V = NamedValues[Name];

	if (!V) {
		std::ostringstream errMsg("Unknown variable name: ");
		errMsg << Name;
		LogErrorV(errMsg.str().c_str());
	}

	return V;
}

/// The constructor for the BinaryExprAST class. This constuctor takes in
/// the character representing the binary operator as well as the expressions
/// on either side of the operator.
BinaryExprAST::BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS,
		std::unique_ptr<ExprAST> RHS)
	: Op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}

/// Generate LLVM IR for a binary expression.
llvm::Value *BinaryExprAST::codegen() {
	llvm::Value *L = LHS->codegen();
	llvm::Value *R = RHS->codegen();
	if (!L || !R)
		return nullptr;

	// TODO: Add more binary operators
	switch (Op) {
	// The string literal parameter in each of these function invocations
	// is an optional name to use in the generator instructions which makes
	// reading the generated instructions a lot easier/
	case '+':
		return Builder.CreateFAdd(L, R, "addtmp");
	case '-':
		return Builder.CreateFSub(L, R, "subtmp");
	case '*':
		return Builder.CreateFMul(L, R, "multmp");
	case '<':
		// fcmp ult is an instruction that always returns a 1-bit integer,
		//     1 if the first operand is strictly less than the second,
		//     0 otherwise
		// taking into account floating-point NaN (we would use fcmp olt if the operands
		// being compared followed a total ordering, i.e. didn't have the concept of NaN)
		// but since our programming language only supports double-precision floating point
		// numbers, we need to convert the result of fcmp olt to a double using uitofp
		// (unsigned integer to floating-point)
		L = Builder.CreateFCmpULT(L, R, "cmptmp");
		// Convert boolean 0/1 to double 0.0 or 1.0
		return Builder.CreateUIToFP(L, llvm::Type::getDoubleTy(Context), "booltmp");
	default:
		std::ostringstream errMsg("unrecognized binary operator: ");
		errMsg << Op;
		return LogErrorV(errMsg.str().c_str());
	}
}

/// The constructor for the CallExprAST class. This constuctor accepts the name
/// of the function being called (callee) as well as all the values passed to the
/// function.
CallExprAST::CallExprAST(const std::string &Callee,
		std::vector<std::unique_ptr<ExprAST>> Args)
	: Callee(Callee), Args(std::move(Args)) {}

llvm::Value *CallExprAST::codegen() {
	// A string stream for holding potential error messages.
	std::ostringstream errMsg;
	// Look up the function name in the global module table.
	llvm::Function *CalleeF = Module->getFunction(Callee);
	if (!CalleeF) {
		errMsg << "Unknown function referenced: " << Callee;
		return LogErrorV(errMsg.str().c_str());
	}

	// Handle argument mismatch error.
	const std::size_t expected = CalleeF->arg_size();
	const std::size_t actual = Args.size();
	if (expected != actual) {
		errMsg << "Wrong number of arguments passed to " << Callee
			<< ", expecting " << expected << " but got " << actual;
		return LogErrorV(errMsg.str().c_str());
	}

	std::vector<llvm::Value *> ArgsV;
	for (unsigned i = 0; i < actual; i++) {
		ArgsV.push_back(Args[i]->codegen());
		if (!ArgsV.back()) return nullptr;
	}

	return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}	

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

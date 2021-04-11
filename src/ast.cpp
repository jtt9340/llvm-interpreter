#include <llvm/IR/LLVMContext.h> // llvm::LLVMContext
#include <llvm/IR/IRBuilder.h>   // llvm::IRBuilder
#include <llvm/IR/Module.h>      // llvm::Module
#include <llvm/IR/verifier.h>    // llvm::verifyFunction
#include <llvm/IR/LegacyPassManager.h>               // llvm::legacy::FunctionPassManager
#include <llvm/Transforms/InstCombine/InstCombine.h> // llvm::createInstructionCombiningPass
#include <llvm/Transforms/Scalar.h>                  // llvm::createReassociatePass
#include <llvm/Transforms/Scalar/GVN.h>              // llvm::createGVNPass
#include <llvm/Transforms/Scalar/SimplifyCFG.h>      // llvm::createCFGSimplificationPass

#include <unordered_map>         // std::unordered_map
#include <sstream>               // std::ostringstream

#include "ast.h"
#include "logging.h"             // LogErrorV

static llvm::LLVMContext Context;
static llvm::IRBuilder<> Builder(Context);
static std::unique_ptr<llvm::Module> Module;
static std::unordered_map<std::string, llvm::Value *> NamedValues;
static std::unique_ptr<llvm::legacy::FunctionPassManager> FunctionPassManager;

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
		std::ostringstream errMsg("Unknown variable name: ", std::ios_base::ate);
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
	case '/':
		return Builder.CreateFDiv(L, R, "divtmp");
	case '<':
		// fcmp ult is an instruction that always returns a 1-bit integer,
		//     1 if the first operand is strictly less than the second,
		//     0 otherwise
		// taking into account floating-point NaN (we would use fcmp olt if the operands
		// being compared followed a total ordering, i.e. didn't have the concept of NaN)
		// but since our programming language only supports double-precision floating point
		// numbers, we need to convert the result of fcmp ult to a double using uitofp
		// (unsigned integer to floating-point)
		L = Builder.CreateFCmpULT(L, R, "cmpulttmp");
		// Convert boolean 0/1 to double 0.0 or 1.0
		return Builder.CreateUIToFP(L, llvm::Type::getDoubleTy(Context), "booltmp");
	case '>':
		L = Builder.CreateFCmpUGT(L, R, "cmpugttmp");
		return Builder.CreateUIToFP(L, llvm::Type::getDoubleTy(Context), "booltmp");
	default:
		std::ostringstream errMsg("unrecognized binary operator: ", std::ios_base::ate);
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

llvm::Function *PrototypeAST::codegen() {
	// All arguments to functions in our language are doubles so create a vector
	// of "N" LLVM double types where N is the number of arguments in the function
	// prototype
	std::vector<llvm::Type *> Doubles(Args.size(),
			llvm::Type::getDoubleTy(Context));

	// Create a function type that returns a double (the first parameter to
	// FunctionType::get), takes Doubles.size() number of arguments, each
	// of type double (the second parameter to FunctionType::get), and is
	// not vararg (the false parameter to FunctionType::get).
	llvm::FunctionType *FT =
		llvm::FunctionType::get(llvm::Type::getDoubleTy(Context), Doubles, false);

	// Actually generate the LLVM IR from the function type above.
	// External linkage means the function can be defined outside of this module.
	llvm::Function *F =
		llvm::Function::Create(FT, llvm::Function::ExternalLinkage, Name, Module.get());

	// Optional but helpful: set all of the names of 
	// of the function arguments in the LLVM IR to the user-specified names for clarity.
	unsigned Idx = 0;
	for (auto &Arg : F->args())
		Arg.setName(Args[Idx++]);

	return F;
}

/// The constructor for the FunctionAST class. This constructor takes in the
/// funciton prototype part of this function definition followed by the
/// code that defines the behavior of the function.
FunctionAST::FunctionAST(std::unique_ptr<PrototypeAST> Proto,
		std::unique_ptr<ExprAST> Body)
	: Proto(std::move(Proto)), Body(std::move(Body)) {}

llvm::Function *FunctionAST::codegen() {
	// Check for an existing function made by an 'extern' declaration.
	llvm::Function *Function = Module->getFunction(Proto->getName());
	// If an existing declaration wasn't found then generate the LLVM IR
	// for it.
	if (!Function) Function = Proto->codegen();
	// If that failed for some reason, return a nullptr.
	if (!Function) return nullptr;
	// TODO Supposedly there is a bug in this function where an existing
	// LLVM IR function definition does not validate its signature against
	// its own prototype which can cause codegen to fail when an earlier
	// function's prototype does not match the definition. I don't really
	// see what the bug is here but I will have to come back and think
	// about this one.

	// If an existing declaration was found and contains a definiton, then
	// we are trying to redefine an extern function which isn't allowed in
	// our language.
	if (!Function->empty()) {
		std::ostringstream errMsg("Function ", std::ios_base::ate);
		errMsg << Proto->getName() << " cannot be redefined";
		return static_cast<llvm::Function *>(LogErrorV(errMsg.str().c_str()));
	}

	// A basic block is a block of code with only one extry point and only one
	// exit point and no branching. They compose the nodes of a control flow graph.
	//
	// "entry" allows us to label the LLVM IR with where functions begin for easier
	// reading. We pass the Function parameter to indicate to start this basic block
	// at the end of the function were are codegen'ing.
	llvm::BasicBlock *BB = llvm::BasicBlock::Create(Context, "entry", Function);
	Builder.SetInsertPoint(BB);

	// Record the function arguments in the NamedValues map.
	NamedValues.clear();
	for (auto &Arg : Function->args())
		NamedValues.emplace(Arg.getName(), &Arg);

	// Generate the LLVM IR for the root expression of this function.
	if (llvm::Value *RetVal = Body->codegen()) {
		// If that succeeded, then create an LLVM ret instruction to
		// return from the function...
		Builder.CreateRet(RetVal);

		// ...and validate the generated code, checking for consistency.
		llvm::verifyFunction(*Function);

		// Run optimizations on the generated code.
		FunctionPassManager->run(*Function);

		return Function;
	}
	// Otherwise, generating the LLVM IR for the root expression failed,
	// so remove the function from the symbol table, which allows the user
	// to redefine it. Otherwise, the namespace would be polluted with
	// a faulty function.
	Function->eraseFromParent();
	return nullptr;
}

void InitializeModuleAndPassManager() {
	// Open a new module.
	Module = std::make_unique<llvm::Module>("Kaleidoscope", Context);

	// Create a new pass manager attached to it. We are using a function pass manager, which
	// passes over code at the function level, looking for optimizations.
	FunctionPassManager = std::make_unique<llvm::legacy::FunctionPassManager>(Module.get());

	// Isn't it crazy that all of these optimizations are built into LLVM and it's just a matter
	// of "ooh pick this optimization"?

	// Add an optimization that can combine obvious duplicate expressions, e.g.
	// (1+2+x)*(x+2+1) becomes (x+3)*(x+3)
	FunctionPassManager->add(llvm::createInstructionCombiningPass());
	// Add an optimization to reorder expressions taking advantage of the commutative and
	// associative properties, e.g. 4 + (x + 5) becomes x + (4 + 5) 
	FunctionPassManager->add(llvm::createReassociatePass());
	// Remove redundant expressions
	FunctionPassManager->add(llvm::createGVNPass());
	// CFG -> Control Flow Graph simplification, i.e. removing dead code, merging basic blocks, etc.
	FunctionPassManager->add(llvm::createCFGSimplificationPass());
	// Initialize all of the above passes.
	FunctionPassManager->doInitialization();
}

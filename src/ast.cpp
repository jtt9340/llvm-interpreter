#include <llvm/IR/IRBuilder.h>         // llvm::IRBuilder
#include <llvm/IR/LLVMContext.h>       // llvm::LLVMContext
#include <llvm/IR/LegacyPassManager.h> // llvm::legacy::FunctionPassManager
#include <llvm/IR/Module.h>            // llvm::Module
#include <llvm/IR/Verifier.h>          // llvm::verifyFunction
#include <llvm/Transforms/InstCombine/InstCombine.h> // llvm::createInstructionCombiningPass
#include <llvm/Transforms/Scalar.h>             // llvm::createReassociatePass
#include <llvm/Transforms/Scalar/GVN.h>         // llvm::createGVNPass
#include <llvm/Transforms/Scalar/SimplifyCFG.h> // llvm::createCFGSimplificationPass
#include <llvm/Transforms/Utils.h> // llvm::createPromoteMemoryToRegisterPass

#include <cassert>       // assert
#include <iostream>      // std::cerr, std::endl
#include <sstream>       // std::ostringstream
#include <unordered_map> // std::unordered_map

#include "KaleidoscopeJIT.h" // JIT
#include "ast.h"
#include "lexer.h"
#include "logging.h" // LogErrorV
#include "parser.h"

using llvm::orc::KaleidoscopeJIT;

static llvm::LLVMContext Context;
static llvm::IRBuilder<> Builder(Context);
static std::unique_ptr<llvm::Module> Module;
static std::unordered_map<std::string, llvm::AllocaInst *> NamedValues;
static std::unique_ptr<llvm::legacy::FunctionPassManager> FunctionPassManager;
static std::unordered_map<std::string, std::unique_ptr<PrototypeAST>>
    FunctionProtos;

/// Get or code generate a function in the current module with the given name,
/// returning null if there is no such function and code generation fails.
llvm::Function *getFunction(const std::string &Name) {
  // See if the function with the given name has aready been added to
  // the current module
  if (auto *F = Module->getFunction(Name))
    return F;

  // If it hasn't, determine if we can codegen the declaration
  // from a pre-existing prototype
  auto FI = FunctionProtos.find(Name);
  if (FI != FunctionProtos.end())
    return FI->second->codegen();

  // No existing prototype exists
  return nullptr;
}

/// Create an alloca instruction in the entry block of the given function.
/// Used for mutable variables.
llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *Function,
                                         const std::string &VarName) {
  // Create an IRBuilder that points to the first instruction of the Function.
  llvm::IRBuilder<> TmpB(&Function->getEntryBlock(),
                         Function->getEntryBlock().begin());
  // Create the alloca with the given name. If we had other types in our
  // language, we would have to pass in a differnet type besides double here and
  // probably take that type as a parameter to this funciton.
  return TmpB.CreateAlloca(llvm::Type::getDoubleTy(Context), 0,
                           VarName.c_str());
}

/// The constructor for the NumberExprAST class.
NumberExprAST::NumberExprAST(double Val) : Val(Val) {}

/// Generate LLVM IR for a numeric constant.
llvm::Value *NumberExprAST::codegen() {
  // ConstantFP -> holds a compile-time floating point constant, represeted by
  // a... APFloat -> Arbitrary Precision Float
  return llvm::ConstantFP::get(Context, llvm::APFloat(Val));
}

/// "NumberExprAST(%f)"
std::string NumberExprAST::toString() const {
  std::ostringstream repr("NumberExprAST(", std::ios_base::ate);
  repr << Val << ')';
  return repr.str();
}

/// The constructor for the VariableExprAST class.
VariableExprAST::VariableExprAST(const std::string &Name) : Name(Name) {}

/// Generate LLVM IR for a variable reference.
llvm::Value *VariableExprAST::codegen() {
  // Look this variable up in the function.
  llvm::Value *V = NamedValues[Name];

  if (!V) {
    std::ostringstream errMsg("Unknown variable name: ", std::ios_base::ate);
    errMsg << Name;
    return LogErrorV(errMsg.str().c_str());
  }

  return Builder.CreateLoad(V, Name.c_str());
}

/// "NumberExprAST(%s)"
std::string VariableExprAST::toString() const {
  std::ostringstream repr("VariableExprAST(", std::ios_base::ate);
  repr << Name << ')';
  return repr.str();
}

/// The constuctor for the BinaryExprAST class.
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
  // reading the generated instructions a lot easier.
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
    // taking into account floating-point NaN (we would use fcmp olt if the
    // operands being compared followed a total ordering, i.e. didn't have the
    // concept of NaN) but since our programming language only supports
    // double-precision floating point numbers, we need to convert the result of
    // fcmp ult to a double using uitofp (unsigned integer to floating-point)
    L = Builder.CreateFCmpULT(L, R, "cmpulttmp");
    // Convert boolean 0/1 to double 0.0 or 1.0
    return Builder.CreateUIToFP(L, llvm::Type::getDoubleTy(Context), "booltmp");
  case '>':
    L = Builder.CreateFCmpUGT(L, R, "cmpugttmp");
    return Builder.CreateUIToFP(L, llvm::Type::getDoubleTy(Context), "booltmp");
  case '=':
    // Set up the base error message for future error conditions in this switch
    // case.
    std::string RHSS = RHS->toString(), LHSS = LHS->toString();
    std::ostringstream errMsg("Could not assign value ", std::ios_base::ate);
    errMsg << RHSS << " to " << LHSS << " beacuse ";

    // For the variable assignment operator =, the LHS is not emitted as an
    // expression. We require the LHS to be a variable name since it doesn't
    // make sense to assign a value to another value. Sort of like how C and C++
    // distinguish between lvalues and rvalues, in this case the only lvalue we
    // have is a variable name.

    // Call .get() on LHS instead of using the dereference operator -> since we
    // want to extract the ExprAST from the unique pointer instead of call a
    // method on the underlying ExprAST.

    // Use dynamic_cast to downcast the underlying ExprAST to a VariableAST.
    // If we used static_cast and the conversion failed (meaning the LHS was NOT
    // a variable expression) then that would be undefined behavior, whereas a
    // dynamic_vast just return nullptr.
    VariableExprAST *LHSE = dynamic_cast<VariableExprAST *>(LHS.get());
    if (!LHSE) {
      errMsg << LHSS << " is not a variable expression.";
      return LogErrorV(errMsg.str().c_str());
    }

    // Look up the variable value by name.
    llvm::Value *Var = NamedValues[LHSE->Name];
    if (!Var) {
      errMsg << LHSE->Name << " is an unknown variable name.";
      return LogErrorV(errMsg.str().c_str());
    }

    // Create the store instruction.
    Builder.CreateStore(R, Var);
    return R;
  }

  // If we have gotten to this point, then Op is a user-defined binary operator
  llvm::Function *F = getFunction(std::string("binary") + Op);
  assert(F);

  llvm::Value *Operands[2] = {L, R};
  return Builder.CreateCall(F, Operands, "binop");
}

/// "lhs op rhs"
std::string BinaryExprAST::toString() const {
  return LHS->toString() + ' ' + Op + ' ' + RHS->toString();
}

/// The constuctor for the UnaryExprAST class.
UnaryExprAST::UnaryExprAST(char Opcode, std::unique_ptr<ExprAST> Operand)
    : Op(Opcode), Operand(std::move(Operand)) {}

/// Generate LLVM IR for a unary expression.
llvm::Value *UnaryExprAST::codegen() {
  llvm::Value *OperandValue = Operand->codegen();
  if (!OperandValue)
    return nullptr;

  llvm::Function *Operator = getFunction(std::string("unary") + Op);
  if (!Operator) {
    // "Unknown unary operator" is 23 characters  + 1 for Op + 1 for NUL byte
    char errMsg[25];
    std::sprintf(errMsg, "Unknown unary operator %c", Op);
    return LogErrorV(errMsg);
  }

  return Builder.CreateCall(Operator, OperandValue, "unop");
}

/// "op rhs"
std::string UnaryExprAST::toString() const { return Op + Operand->toString(); }

CallExprAST::CallExprAST(const std::string &Callee,
                         std::vector<std::unique_ptr<ExprAST>> Args)
    : Callee(Callee), Args(std::move(Args)) {}

/// Generate LLVM IR for a function call.
llvm::Value *CallExprAST::codegen() {
  // A string stream for holding potential error messages.
  std::ostringstream errMsg;
  // Look up the function name in the global module table.
  llvm::Function *CalleeF = getFunction(Callee);
  if (!CalleeF) {
    errMsg << "Unknown function referenced: " << Callee;
    return LogErrorV(errMsg.str().c_str());
  }

  // Handle argument mismatch error.
  const std::size_t expected = CalleeF->arg_size();
  const std::size_t actual = Args.size();
  if (expected != actual) {
    errMsg << "Wrong number of arguments passed to " << Callee << ", expecting "
           << expected << " but got " << actual;
    return LogErrorV(errMsg.str().c_str());
  }

  std::vector<llvm::Value *> ArgsV;
  for (unsigned i = 0; i < actual; i++) {
    ArgsV.push_back(Args[i]->codegen());
    if (!ArgsV.back())
      return nullptr;
  }

  return Builder.CreateCall(CalleeF, ArgsV, "calltmp");
}

/// "CallExprAST(function(arg0, arg1, ..., argn))"
std::string CallExprAST::toString() const {
  std::ostringstream repr("CallExprAST(", std::ios_base::ate);
  repr << Callee << '(';
  for (auto it = Args.begin(); it != Args.end(); it++) {
    repr << (*it)->toString();
    if (it != Args.end() - 1)
      repr << ", ";
  }
  repr << "))";
  return repr.str();
}

/// The constructor for the IfExprAST class.
IfExprAST::IfExprAST(std::unique_ptr<ExprAST> Cond,
                     std::unique_ptr<ExprAST> Then,
                     std::unique_ptr<ExprAST> Else)
    : Cond(std::move(Cond)), Then(std::move(Then)), Else(std::move(Else)) {}

/// Generate LLVM IR for an if expression.
llvm::Value *IfExprAST::codegen() {
  llvm::Value *CondV = Cond->codegen();
  if (!CondV)
    return nullptr;

  // Convert Cond to a boolean by seeing if it is unequal to 0.0
  CondV = Builder.CreateFCmpONE(
      CondV, llvm::ConstantFP::get(Context, llvm::APFloat(0.0)), "ifcond");

  // Get the encompassing function containing the if expression. This
  // is how we determine where to insert the then-clause.
  // GetInsertBlock finds the current basic block,
  // and getParent find the encompassing block, the function in this case.
  llvm::Function *Function = Builder.GetInsertBlock()->getParent();
  // Create basic blocks for the then and else clauses, then insert
  // the then-clause at the end of the function.
  llvm::BasicBlock *ThenBuildingBlock =
      llvm::BasicBlock::Create(Context, "then", Function);
  llvm::BasicBlock *ElseBuildingBlock =
      llvm::BasicBlock::Create(Context, "else");
  llvm::BasicBlock *Merged = llvm::BasicBlock::Create(Context, "ifcont");

  Builder.CreateCondBr(CondV, ThenBuildingBlock, ElseBuildingBlock);

  // Emit then-clause as LLVM IR.
  Builder.SetInsertPoint(ThenBuildingBlock);

  llvm::Value *ThenV = Then->codegen();
  if (!ThenV)
    return nullptr;

  Builder.CreateBr(Merged);
  ThenBuildingBlock = Builder.GetInsertBlock();

  // Emit else-clause as LLVM IR.
  Function->getBasicBlockList().push_back(ElseBuildingBlock);
  Builder.SetInsertPoint(ElseBuildingBlock);

  llvm::Value *ElseV = Else->codegen();
  if (!ElseV)
    return nullptr;

  Builder.CreateBr(Merged);
  ElseBuildingBlock = Builder.GetInsertBlock();

  // Emit the merged basic block as LLVM IR. The merged
  // block is where both execution paths will end up after the
  // if expression. In the control flow graph, it is where each node
  // if the if expression will point to next.
  Function->getBasicBlockList().push_back(Merged);
  Builder.SetInsertPoint(Merged);
  llvm::PHINode *PhiNode =
      Builder.CreatePHI(llvm::Type::getDoubleTy(Context), 2, "iftmp");

  PhiNode->addIncoming(ThenV, ThenBuildingBlock);
  PhiNode->addIncoming(ElseV, ElseBuildingBlock);
  return PhiNode;
}

/// "IfExprAST(cond ? ifTrue : ifFalse)"
std::string IfExprAST::toString() const {
  std::ostringstream repr("IfExptAST(", std::ios_base::ate);
  repr << Cond->toString() << std::endl
       << "\t? " << Then->toString() << std::endl
       << "\t: " << Else->toString() << std::endl
       << ')';
  return repr.str();
}

/// The constructor for the ForExprAST class.
ForExprAST::ForExprAST(const std::string &Name, std::unique_ptr<ExprAST> Start,
                       std::unique_ptr<ExprAST> End,
                       std::unique_ptr<ExprAST> Step,
                       std::unique_ptr<ExprAST> Body)
    : VarName(Name), Start(std::move(Start)), End(std::move(End)),
      Step(std::move(Step)), Body(std::move(Body)) {}

/// Generate LLVM IR for a for expression.
llvm::Value *ForExprAST::codegen() {
  // Generate the basic block for the start of the loop body,
  // taking into account that that block could have multiple
  // blocks (e.g. if statements, more for loops)
  llvm::Function *Function = Builder.GetInsertBlock()->getParent();

  // Create an alloca for the variable in the entry block.
  // This is required because we allow the user (programmer) to
  // mutate induction variables in loops.
  llvm::AllocaInst *Alloca = CreateEntryBlockAlloca(Function, VarName);

  // Emit LLVM IR for the initial expression without the
  // induction variable in scope
  llvm::Value *StartVal = Start->codegen();
  if (!StartVal)
    return nullptr;

  // Use the alloca instruction we created above to store
  // StartVal on the stack.
  Builder.CreateStore(StartVal, Alloca);

  llvm::BasicBlock *LoopHeaderBasicBlock = Builder.GetInsertBlock();
  llvm::BasicBlock *LoopBasicBlock =
      llvm::BasicBlock::Create(Context, "loop", Function);

  // Explicit fallthrough from current block to loop block
  Builder.CreateBr(LoopBasicBlock);

  // Start insterting code into the LoopBasicBlock
  Builder.SetInsertPoint(LoopBasicBlock);

  // Save the old value of the variable with this name in case
  // it shadows an earlier one
  llvm::AllocaInst *OldVal = NamedValues[VarName];
  NamedValues[VarName] = Alloca;

  // Emit LLVM IR for the loop body. Keep in mind doing this
  // can change the current basic block
  if (!Body->codegen())
    return nullptr; // Don't allow an error

  // Emit the step value, setting it to 1.0 if it does not exist
  llvm::Value *StepVal = nullptr;
  if (Step) {
    StepVal = Step->codegen();
    if (!StepVal)
      return nullptr;
  } else {
    StepVal = llvm::ConstantFP::get(Context, llvm::APFloat(1.0));
  }

  // Emit the conditional expression
  llvm::Value *CondVal = End->codegen();
  if (!CondVal)
    return nullptr;

  // Reload, increment, and store the alloca instruction.
  // This is necessary because the loop body couuld mutate
  // the induction variable.
  llvm::Value *CurrentVal = Builder.CreateLoad(Alloca);
  // Add an increment at the end of the loop, similar to manually adding
  // i++ at the end of a while loop
  llvm::Value *IncrementedVar =
      Builder.CreateFAdd(CurrentVal, StepVal, "incremented");
  Builder.CreateStore(IncrementedVar, Alloca);

  // Convert condition to a boolean by comparing not-equal to 0.0
  CondVal = Builder.CreateFCmpONE(
      CondVal, llvm::ConstantFP::get(Context, llvm::APFloat(0.0)), "loopcond");

  // Create an insert the basic block that goes after the loop
  llvm::BasicBlock *LoopEndBasicBlock = Builder.GetInsertBlock();
  llvm::BasicBlock *AfterBasicBlock =
      llvm::BasicBlock::Create(Context, "afterloop", Function);

  // Insert the conditional branch that determine
  // if we go back to the start of the loop
  Builder.CreateCondBr(CondVal, LoopBasicBlock, AfterBasicBlock);

  // Insert any new code after the AfterBasicBlock
  Builder.SetInsertPoint(AfterBasicBlock);

  // Restore the variable that was potentially shadowed before entering the loop
  if (OldVal)
    NamedValues[VarName] = OldVal;
  else
    NamedValues.erase(VarName);

  // A for expression evaluates to 0.0 (what other value would make sense?)
  return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(Context));
}

/// "ForExprAST(var = init, cond, step, body)"
std::string ForExprAST::toString() const {
  std::ostringstream repr("ForExprAST(", std::ios_base::ate);
  repr << VarName << " = " << Start->toString() << ", " << End->toString();
  if (Step)
    repr << ", " << Step->toString();
  repr << ',' << std::endl << '\t' << Body->toString() << std::endl << ')';
  return repr.str();
}

/// Constructor for the PrototypeAST class.
PrototypeAST::PrototypeAST(const std::string &Name,
                           std::vector<std::string> Args, bool IsOperator,
                           unsigned Precedence)
    : Name(Name), Args(std::move(Args)), IsOperator(IsOperator),
      Precedence(Precedence) {}

/// Getter for the "Name" field of instances of PrototypeAST.
const std::string &PrototypeAST::getName() const { return Name; }

/// Generate LLVM IR for a function prototype.
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
  llvm::Function *F = llvm::Function::Create(
      FT, llvm::Function::ExternalLinkage, Name, Module.get());

  // Optional but helpful: set all of the names of
  // of the function arguments in the LLVM IR to the
  // user-specified names for clarity.
  unsigned Idx = 0;
  for (auto &Arg : F->args())
    Arg.setName(Args[Idx++]);

  return F;
}

/// "PrototypeAST(function(arg0, arg1, ..., argn))"
std::string PrototypeAST::toString() const {
  std::ostringstream repr("PrototypeAST(", std::ios_base::ate);
  repr << Name << '(';
  for (auto it = Args.begin(); it != Args.end(); it++) {
    repr << *it;
    if (it != Args.end() - 1)
      repr << ", ";
  }
  repr << "))";
  return repr.str();
}

/// Is this a function prototype for a unary operator?
bool PrototypeAST::isUnaryOp() const { return IsOperator && Args.size() == 1; }

/// Is this a function prototype for a binary operator?
bool PrototypeAST::isBinaryOp() const { return IsOperator && Args.size() == 2; }

/// If this is a unary or binary operator, then the character representing the
/// operator, else the NUL byte.
char PrototypeAST::getOperatorName() const {
  return isUnaryOp() || isBinaryOp() ? Name[Name.size() - 1] : 0;
}

/// Return the precedence of thhis binary operator (if this is a binary
/// operator).
unsigned PrototypeAST::getBinaryPrecedence() const { return Precedence; }

FunctionAST::FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                         std::unique_ptr<ExprAST> Body)
    : Proto(std::move(Proto)), Body(std::move(Body)) {}

/// Generate LLVM IR for a function definition.
llvm::Function *FunctionAST::codegen() {
  const auto &P = *Proto;
  FunctionProtos[Proto->getName()] = std::move(Proto);
  // Check for an existing function made by an 'extern' declaration.
  llvm::Function *Function = getFunction(P.getName());
  if (!Function)
    return nullptr;

  // If this is a binary operator, add it to
  // the binary operator precedence table.
  if (P.isBinaryOp())
    InstallBinopPrecedence(P.getOperatorName(), P.getBinaryPrecedence());

  // TODO Supposedly there is a bug in this function where an existing
  // LLVM IR function definition does not validate its signature against
  // its own prototype which can cause codegen to fail when an earlier
  // function's prototype does not match the definition. I don't really
  // see what the bug is here but I will have to come back and think
  // about this one.

  // A basic block is a block of code with only one extry point and only one
  // exit point and no branching. They compose the nodes of a control flow
  // graph.
  //
  // "entry" allows us to label the LLVM IR with where functions begin for
  // easier reading. We pass the Function parameter to indicate to start this
  // basic block at the end of the function were are codegen'ing.
  llvm::BasicBlock *BB = llvm::BasicBlock::Create(Context, "entry", Function);
  Builder.SetInsertPoint(BB);

  // Record the function arguments in the NamedValues map.
  NamedValues.clear();
  for (auto &Arg : Function->args()) {
    // Create an alloca for this function argument.
    // This allows the user (programmer) to mutate
    // function parameters.
    llvm::AllocaInst *Alloca =
        CreateEntryBlockAlloca(Function, Arg.getName().str());

    // Store the passed-in parameter value in the alloca instruction.
    Builder.CreateStore(&Arg, Alloca);

    // Add arguments to the current scope.
    NamedValues[Arg.getName().str()] = Alloca;
  }

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

/// "FunctionAST(prototype, body)"
std::string FunctionAST::toString() const {
  std::ostringstream repr("FunctionAST(\n\t", std::ios_base::ate);
  repr << Proto->toString() << ",\n\t" << Body->toString() << std::endl << ')';
  return repr.str();
}

/// The constructor for the LetExprAST class.
LetExprAST::LetExprAST(
    std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames,
    std::unique_ptr<ExprAST> Body)
    : VarNames(std::move(VarNames)), Body(std::move(Body)) {}

/// Generate LLVM IR for a let/in expression.
llvm::Value *LetExprAST::codegen() {
  // If we encounter any new variables that shadow existing ones,
  // we put the old values here so that they can be restored after
  // the new ones go out of scope.
  std::vector<llvm::AllocaInst *> OldBindings(VarNames.size());

  llvm::Function *Function = Builder.GetInsertBlock()->getParent();

  for (const auto &NameValuePair : VarNames) {
    const std::string &VarName = NameValuePair.first;
    ExprAST *const InitialExpr = NameValuePair.second.get();

    // We generate LLVM IR for the initial value before
    // adding the variable to the scope, this way self-referential
    // variable declarations are errors (let a = a)
    // and declarations like the following are possible:
    //
    //    let a = 1 in
    //      let a = a in # Refers to outer a
    //        a;
    llvm::Value *InitialValue =
        InitialExpr ? InitialExpr->codegen()
                    // If no value for the variable was given,
                    // initialize to 0.0.
                    : llvm::ConstantFP::get(Context, llvm::APFloat(0.0));
    if (!InitialValue)
      return nullptr;

    llvm::AllocaInst *Alloca = CreateEntryBlockAlloca(Function, VarName);
    Builder.CreateStore(InitialValue, Alloca);

    // Add any old value for this variable so that it can be restored after
    // unrecursing.
    OldBindings.push_back(NamedValues[VarName]);

    // Now add this variable to the current scope.
    NamedValues[VarName] = Alloca;
  }

  // Now that we've created the variable, we can emit the body.
  llvm::Value *BodyVal = Body->codegen();
  if (!BodyVal)
    return nullptr;

  // Restore all the old values of the variables.
  for (unsigned i = 0; i < VarNames.size(); i++)
    NamedValues[VarNames[i].first] = OldBindings[i];

  // Return the value that the body evaluates to.
  return BodyVal;
}

/// "LetExprAST(var0 = val0, var1 = val1, ..., varn = valn; body)"
std::string LetExprAST::toString() const {
  std::ostringstream repr("LetExprAST(\n", std::ios_base::ate);
  for (auto it = VarNames.begin(); it != VarNames.end(); it++) {
    const auto VarName = it->first;
    const auto *InitialExpr = it->second.get();

    repr << '\t' << VarName << " = "
         << (InitialExpr ? InitialExpr->toString()
                         : NumberExprAST(0.0).toString())
         << (it == VarNames.end() - 1 ? ';' : ',') << '\n';
  }

  repr << '\t' << Body->toString() << "\n)";
  return repr.str();
}

void InitializeModuleAndPassManager() {
  // Open a new module.
  Module = std::make_unique<llvm::Module>("Kaleidoscope", Context);
  Module->setDataLayout(
      KaleidoscopeJIT::getInstance()->getTargetMachine().createDataLayout());

  // Create a new pass manager attached to it. We are using a function pass
  // manager, which passes over code at the function level, looking for
  // optimizations.
  FunctionPassManager =
      std::make_unique<llvm::legacy::FunctionPassManager>(Module.get());

  // Isn't it crazy that all of these optimizations are built into LLVM and it's
  // just a matter of "ooh pick this optimization"?

  // Turn alloca instructions into registers. (mem2reg)
  FunctionPassManager->add(llvm::createPromoteMemoryToRegisterPass());
  // Add an optimization that can combine obvious duplicate expressions, e.g.
  // (1+2+x)*(x+2+1) becomes (x+3)*(x+3)
  FunctionPassManager->add(llvm::createInstructionCombiningPass());
  // Add an optimization to reorder expressions taking advantage of the
  // commutative and associative properties, e.g. 4 + (x + 5) becomes x + (4 +
  // 5)
  FunctionPassManager->add(llvm::createReassociatePass());
  // Remove redundant expressions
  FunctionPassManager->add(llvm::createGVNPass());
  // CFG -> Control Flow Graph simplification, i.e. removing dead code, merging
  // basic blocks, etc.
  FunctionPassManager->add(llvm::createCFGSimplificationPass());
  // Initialize all of the above passes.
  FunctionPassManager->doInitialization();
}

/// What to do when a function definition is encountered at the REPL.
void HandleDefinition() {
  const auto defn = ParseDefinition();
  if (defn) {
    const auto *ir = defn->codegen();
    if (ir) {
      std::cerr << "Generate LLVM IR for function definition:" << std::endl;
      ir->print(llvm::errs());
      std::cerr << std::endl;
      KaleidoscopeJIT::getInstance()->addModule(std::move(Module));
      InitializeModuleAndPassManager();
    }
  } else {
    // Skip token to handle errors.
    getNextToken();
  }
}

/// What to do when an extern function delcaration is encountered at the REPL.
void HandleExtern() {
  auto externDeclaration = ParseExtern();
  if (externDeclaration) {
    const auto *ir = externDeclaration->codegen();
    if (ir) {
      std::cerr << "Generate LLVM IR for extern function declaration:"
                << std::endl;
      ir->print(llvm::errs());
      std::cerr << std::endl;
      FunctionProtos[externDeclaration->getName()] =
          std::move(externDeclaration);
    }
  } else {
    // Skip token to handle errors.
    getNextToken();
  }
}

/// What to do when any other expression that is not a function definition or
/// extern function declaration is encountered at the REPL.
void HandleTopLevelExpression() {
  // Evaluate a top-level expression in an anonymous function.
  const auto expr = ParseTopLevelExpr();
  if (expr) {
    const auto *ir = expr->codegen();
    if (ir) {
      // Just-in-time compile the generated LLVM IR
      // We need to keep a handle to it so that it can be freed later
      auto H = KaleidoscopeJIT::getInstance()->addModule(std::move(Module));
      InitializeModuleAndPassManager();

      // Search the JIT for the __anon_expr symbol
      auto ExprSymbol =
          KaleidoscopeJIT::getInstance()->findSymbol("__anon_expr");
      assert(ExprSymbol);

      // Get the memory address of the function. This could fail if
      // for some reason the function isn't found. Just report an error
      // in this case
      auto ExprSymbolAddress = ExprSymbol.getAddress();
      if (auto E = ExprSymbolAddress.takeError()) {
        LogError(toString(std::move(E)).c_str());
      } else {
        // Cast the symbol's address to a function pointer
        // and call the function natively
        double (*FP)() = reinterpret_cast<double (*)()>(
            static_cast<intptr_t>(*ExprSymbolAddress));
        std::cerr << FP() << std::endl;
      }

      // Delete the module created for the anonymous expression
      KaleidoscopeJIT::getInstance()->removeModule(H);
    }
  } else {
    // Skip token to handle errors.
    getNextToken();
  }
}

#include <sstream> // std::ostringstream

#include "ForExprAST.h"

/// The constructor for the ForExprAST class.
ForExprAST::ForExprAST(const std::string &Name, std::unique_ptr<ExprAST> Start,
                       std::unique_ptr<ExprAST> End,
                       std::unique_ptr<ExprAST> Step,
                       std::unique_ptr<ExprAST> Body)
    : VarName(Name), Start(std::move(Start)), End(std::move(End)),
      Step(std::move(Step)), Body(std::move(Body)) {}

/// Generate LLVM IR for a for expression.
llvm::Value *ForExprAST::codegen() {
  auto &Builder = getBuilder();
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
      llvm::BasicBlock::Create(getContext(), "loop", Function);

  // Explicit fallthrough from current block to loop block
  Builder.CreateBr(LoopBasicBlock);

  // Start insterting code into the LoopBasicBlock
  Builder.SetInsertPoint(LoopBasicBlock);

  // Save the old value of the variable with this name in case
  // it shadows an earlier one
  auto &NamedValues = getNamedValues();
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
    StepVal = llvm::ConstantFP::get(getContext(), llvm::APFloat(1.0));
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
      CondVal, llvm::ConstantFP::get(getContext(), llvm::APFloat(0.0)),
      "loopcond");

  // Create an insert the basic block that goes after the loop
  llvm::BasicBlock *LoopEndBasicBlock = Builder.GetInsertBlock();
  llvm::BasicBlock *AfterBasicBlock =
      llvm::BasicBlock::Create(getContext(), "afterloop", Function);

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
  return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(getContext()));
}

/// "ForExprAST(var = init, cond, step, body)"
std::string ForExprAST::toString() const {
  std::ostringstream repr("ForExprAST(", std::ios_base::ate);
  repr << VarName << " = " << Start->toString() << ", " << End->toString();
  if (Step)
    repr << ", " << Step->toString();
  repr << ',' << "\n\t" << Body->toString() << "\n)";
  return repr.str();
}

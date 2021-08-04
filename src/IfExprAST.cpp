#include <sstream> // std::ostringstream

#include "IfExprAST.h"

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

  auto &Builder = getBuilder();
  auto &Context = getContext();

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
  repr << Cond->toString() << "\n\t? " << Then->toString()
       << "\n\t: " << Else->toString() << "\n)";
  return repr.str();
}

#include <llvm/IR/Verifier.h> // llvm::verifyFunction

#include <sstream> // std::ostringstream

#include "parser.h" // InstallBinopPrecedence

#include "ExprAST.h"
#include "FunctionAST.h"

FunctionAST::FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                         std::unique_ptr<ExprAST> Body)
    : Proto(std::move(Proto)), Body(std::move(Body)) {}

/// Generate LLVM IR for a function definition.
llvm::Function *FunctionAST::codegen() {
  const auto &P = *Proto;
  auto &FunctionProtos = getFunctionProtos();
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
  llvm::BasicBlock *BB =
      llvm::BasicBlock::Create(getContext(), "entry", Function);
  auto &Builder = getBuilder();
  Builder.SetInsertPoint(BB);

  // Record the function arguments in the NamedValues map.
  auto &NamedValues = getNamedValues();
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
    getFunctionPassManager().run(*Function);

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
std::string FunctionAST::toString(const unsigned depth) const {
  std::ostringstream repr;
  insert_indent(repr, depth);
  repr << "FunctionAST(" << std::endl << Proto->toString(depth + 1) << ',' << std::endl << Body->toString(depth + 1) << std::endl;
  insert_indent(repr, depth);
  repr << ')';
  return repr.str();
}

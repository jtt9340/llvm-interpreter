#include <sstream> // std::ostringstream

#include "BinaryExprAST.h"
#include "VariableExprAST.h"

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

  auto &Builder = getBuilder();

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
    return Builder.CreateUIToFP(L, llvm::Type::getDoubleTy(getContext()),
                                "booltmp");
  case '>':
    L = Builder.CreateFCmpUGT(L, R, "cmpugttmp");
    return Builder.CreateUIToFP(L, llvm::Type::getDoubleTy(getContext()),
                                "booltmp");
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
    llvm::Value *Var = getNamedValues().at(LHSE->Name);
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

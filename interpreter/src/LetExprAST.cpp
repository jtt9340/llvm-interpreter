#include <sstream> // std::ostringstream

#include "LetExprAST.h"
#include "NumberExprAST.h"

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

  auto &Builder = getBuilder();
  llvm::Function *Function = Builder.GetInsertBlock()->getParent();

  auto &NamedValues = getNamedValues();
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
                    : llvm::ConstantFP::get(getContext(), llvm::APFloat(0.0));
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
std::string LetExprAST::toString(const unsigned depth) const {
  std::ostringstream repr;
  insert_indent(repr, depth);
  repr << "LetExprAST(" << std::endl;

  for (auto it = VarNames.begin(); it != VarNames.end(); it++) {
    const auto VarName = it->first;
    const auto *InitialExpr = it->second.get();

    insert_indent(repr, depth + 1);
    repr << VarName << " = "
         << (InitialExpr ? InitialExpr->toString()
                         : NumberExprAST(0.0).toString())
         << (it == VarNames.end() - 1 ? ';' : ',') << std::endl;
  }

  repr << Body->toString(depth + 1) << std::endl;
  insert_indent(repr, depth);
  repr << ')';
  return repr.str();
}

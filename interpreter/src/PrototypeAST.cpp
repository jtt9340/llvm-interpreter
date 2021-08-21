#include <sstream> // std::ostringstream

#include "ExprAST.h"
#include "PrototypeAST.h"

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
                                    llvm::Type::getDoubleTy(getContext()));

  // Create a function type that returns a double (the first parameter to
  // FunctionType::get), takes Doubles.size() number of arguments, each
  // of type double (the second parameter to FunctionType::get), and is
  // not vararg (the false parameter to FunctionType::get).
  llvm::FunctionType *FT = llvm::FunctionType::get(
      llvm::Type::getDoubleTy(getContext()), Doubles, false);

  // Actually generate the LLVM IR from the function type above.
  // External linkage means the function can be defined outside of this module.
  llvm::Function *F = llvm::Function::Create(
      FT, llvm::Function::ExternalLinkage, Name, borrowModule());

  // Optional but helpful: set all of the names of
  // of the function arguments in the LLVM IR to the
  // user-specified names for clarity.
  unsigned Idx = 0;
  for (auto &Arg : F->args())
    Arg.setName(Args[Idx++]);

  return F;
}

/// "PrototypeAST(function(arg0, arg1, ..., argn))"
std::string PrototypeAST::toString(const unsigned depth) const {
  std::ostringstream repr;
  insert_indent(repr, depth);
  repr << "PrototypeAST(" << Name << '(';
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
  return (isUnaryOp() || isBinaryOp()) ? Name[Name.size() - 1] : '\0';
}

/// Return the precedence of thhis binary operator (if this is a binary
/// operator).
unsigned PrototypeAST::getBinaryPrecedence() const { return Precedence; }

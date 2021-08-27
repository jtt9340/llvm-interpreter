#include <llvm/Transforms/InstCombine/InstCombine.h> // llvm::createInstructionCombiningPass
#include <llvm/Transforms/Scalar.h>             // llvm::createReassociatePass
#include <llvm/Transforms/Scalar/GVN.h>         // llvm::createGVNPass
#include <llvm/Transforms/Scalar/SimplifyCFG.h> // llvm::createCFGSimplificationPass
#include <llvm/Transforms/Utils.h> // llvm::createPromoteMemoryToRegisterPass

#include <iostream> // std::cerr, std::endl
#include <sstream>  // std::ostringstream

#include "lexer.h" // getNextToken
#include "parser.h"
#include "util.h"

#include "ExprAST.h"
#include "KaleidoscopeJIT.h" // JIT

using llvm::orc::KaleidoscopeJIT;

/// "Showable@<address>"
std::string Showable::toString(unsigned depth) const {
  // Default implementation is to just return the memory
  // address of this object as a string
  std::ostringstream repr;
  insert_indent(repr, depth);
  repr << "Showable@" << std::hex << this;
  return repr.str();
}

void InitializeModuleAndPassManager() {
  // Open a new module.
  newModule("Kaleidoscope");
#if 0
  borrowModule().setDataLayout(
      KaleidoscopeJIT::getInstance()->getTargetMachine().createDataLayout());

  // Create a new pass manager attached to it. We are using a function pass
  // manager, which passes over code at the function level, looking for
  // optimizations.
  resetFunctionPassManager();

  // Isn't it crazy that all of these optimizations are built into LLVM and it's
  // just a matter of "ooh pick this optimization"?
  auto &FunctionPassManager = getFunctionPassManager();

  // Turn alloca instructions into registers. (mem2reg)
  FunctionPassManager.add(llvm::createPromoteMemoryToRegisterPass());
  // Add an optimization that can combine obvious duplicate expressions, e.g.
  // (1+2+x)*(x+2+1) becomes (x+3)*(x+3)
  FunctionPassManager.add(llvm::createInstructionCombiningPass());
  // Add an optimization to reorder expressions taking advantage of the
  // commutative and associative properties, e.g. 4 + (x + 5) becomes x + (4 +
  // 5)
  FunctionPassManager.add(llvm::createReassociatePass());
  // Remove redundant expressions
  FunctionPassManager.add(llvm::createGVNPass());
  // CFG -> Control Flow Graph simplification, i.e. removing dead code, merging
  // basic blocks, etc.
  FunctionPassManager.add(llvm::createCFGSimplificationPass());
  // Initialize all of the above passes.
  FunctionPassManager.doInitialization();
#endif
}

/// Get or code generate a function in the current module with the given name,
/// returning null if there is no such function and code generation fails.
llvm::Function *getFunction(const std::string &Name) {
  // See if the function with the given name has aready been added to
  // the current module
  if (auto *F = borrowModule().getFunction(Name))
    return F;

  // If it hasn't, determine if we can codegen the declaration
  // from a pre-existing prototype
  const auto &FunctionProtos = getFunctionProtos();
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
  return TmpB.CreateAlloca(llvm::Type::getDoubleTy(getContext()), 0,
                           VarName.c_str());
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
#if 0
      KaleidoscopeJIT::getInstance()->addModule(takeModule());
      InitializeModuleAndPassManager();
#endif
    }
  } else {
    // Skip token to handle errors.
    getNextToken();
  }
}

/// What to do when an extern function delcaration is encountered at the REPL.
void HandleExtern() {
  if (auto externDeclaration = ParseExtern()) {
    if (const auto *ir = externDeclaration->codegen()) {
      llvm::errs() << "Generate LLVM IR for extern function declaration:\n";
      ir->print(llvm::errs());
      llvm::errs() << '\n';
      auto &FunctionProtos = getFunctionProtos();
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
#if 0
    if (ir) {
      // Just-in-time compile the generated LLVM IR
      // We need to keep a handle to it so that it can be freed later
      auto H = KaleidoscopeJIT::getInstance()->addModule(takeModule());
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
#endif
  } else {
    // Skip token to handle errors.
    getNextToken();
  }
}

std::ostream &insert_indent(std::ostream &os, const unsigned n) {
  for (unsigned i = 0; i < n; i++)
    os << '\t';
  return os;
}

/// Remove whitespace from beginning.
std::string &strltrim(std::string &s) {
  auto nonwhitespaceIndex = s.find_first_not_of(WHITESPACE_CHARS);
  s.erase(0, nonwhitespaceIndex);
  return s;
}

// TODO - make error reports more user friendly
std::unique_ptr<ExprAST> LogError(const char *Str) {
  std::cerr << "LogError: " << Str << std::endl;
  return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
  LogError(Str);
  return nullptr;
}

llvm::Value *LogErrorV(const char *Str) {
  LogError(Str);
  return nullptr;
}

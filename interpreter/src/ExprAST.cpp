#include <sstream>

#include "ExprAST.h"

using llvm::LLVMContext;

static LLVMContext Context;
static llvm::IRBuilder<> Builder(Context);
static std::unique_ptr<llvm::Module> Module;
static std::unique_ptr<llvm::DIBuilder> DBuilder;
static std::unordered_map<std::string, llvm::AllocaInst *> NamedValues;
static std::unique_ptr<llvm::legacy::FunctionPassManager> FunctionPassManager =
    nullptr;
static std::unordered_map<std::string, std::unique_ptr<PrototypeAST>>
    FunctionProtos;

SourceLocation::SourceLocation(int Line, int Col) : Line(Line), Col(Col) {}

int SourceLocation::line() const { return Line; }
int &SourceLocation::line() { return Line; }

int SourceLocation::col() const { return Col; }
int &SourceLocation::col() { return Col; }

std::string SourceLocation::toString(const unsigned depth) const {
  std::ostringstream repr;
  repr << ':' << Line << ':' << Col;
  return repr.str();
}

ExprAST::ExprAST(SourceLocation Loc) : Loc(Loc) {}

const SourceLocation ExprAST::loc() const { return Loc; }

LLVMContext &getContext() { return Context; }
llvm::IRBuilder<> &getBuilder() { return Builder; }

llvm::DIBuilder &borrowDBuilder() { return *DBuilder; }
void newDBuilder() { DBuilder = std::make_unique<llvm::DIBuilder>(*Module); }

llvm::Module &borrowModule() { return *Module; }
std::unique_ptr<llvm::Module> takeModule() { return std::move(Module); }
void newModule(const char *newModuleName) {
  Module = std::make_unique<llvm::Module>(newModuleName, Context);
}

std::unordered_map<std::string, llvm::AllocaInst *> &getNamedValues() {
  return NamedValues;
}

llvm::legacy::FunctionPassManager *getFunctionPassManager() {
  return FunctionPassManager.get();
}

void resetFunctionPassManager() {
  FunctionPassManager =
      std::make_unique<llvm::legacy::FunctionPassManager>(Module.get());
}

std::unordered_map<std::string, std::unique_ptr<PrototypeAST>> &
getFunctionProtos() {
  return FunctionProtos;
}

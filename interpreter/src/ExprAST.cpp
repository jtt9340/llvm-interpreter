#include "ExprAST.h"

using llvm::LLVMContext;

static LLVMContext Context;
static llvm::IRBuilder<> Builder(Context);
static std::unique_ptr<llvm::Module> Module;
static std::unordered_map<std::string, llvm::AllocaInst *> NamedValues;
static std::unique_ptr<llvm::legacy::FunctionPassManager> FunctionPassManager =
    nullptr;
static std::unordered_map<std::string, std::unique_ptr<PrototypeAST>>
    FunctionProtos;

LLVMContext &getContext() { return Context; }
llvm::IRBuilder<> &getBuilder() { return Builder; }

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

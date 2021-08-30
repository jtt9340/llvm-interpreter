#include <llvm/IR/IRBuilder.h>         // llvm::IRBuilder
#include <llvm/IR/LLVMContext.h>       // llvm::LLVMContext
#include <llvm/IR/LegacyPassManager.h> // llvm::legacy::FunctionPassManager
#include <llvm/IR/Module.h>            // llvm::Module

#include <unordered_map> // std::unordered_map

#include "PrototypeAST.h"
#include "util.h" // Showable

#ifndef EXPRAST_H
#define EXPRAST_H

/// ExprAST - Base class for all expression nodes.
class ExprAST : public Showable {
public:
  /// The destructor for the ExprAST class. Since the ExprAST class is just an
  /// abstract base class for all possible nodes of our AST, this is just an
  /// empty destructor.
  virtual ~ExprAST() = default;

  /// Generate LLVM IR for this AST node and all dependent AST nodes.
  virtual llvm::Value *codegen() = 0;
};

// Global variables used by subclasses of ExprAST.
llvm::LLVMContext &getContext();
llvm::IRBuilder<> &getBuilder();

llvm::Module &borrowModule();
void newModule(const char *newModuleName);
std::unique_ptr<llvm::Module> takeModule();

std::unordered_map<std::string, llvm::AllocaInst *> &getNamedValues();

llvm::legacy::FunctionPassManager *getFunctionPassManager();
void resetFunctionPassManager();

std::unordered_map<std::string, std::unique_ptr<PrototypeAST>> &
getFunctionProtos();

#endif // EXPRAST_H

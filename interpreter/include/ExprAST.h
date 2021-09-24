#include <llvm/IR/DIBuilder.h>         // llvm::DIBuilder
#include <llvm/IR/IRBuilder.h>         // llvm::IRBuilder
#include <llvm/IR/LLVMContext.h>       // llvm::LLVMContext
#include <llvm/IR/LegacyPassManager.h> // llvm::legacy::FunctionPassManager
#include <llvm/IR/Module.h>            // llvm::Module

#include <unordered_map> // std::unordered_map

#include "PrototypeAST.h"
#include "util.h" // Showable

#ifndef EXPRAST_H
#define EXPRAST_H

/// An object that stores a location in a file so that the parser "remembers"
/// where it is.
class SourceLocation : public Showable {
  int Line, Col;

public:
  SourceLocation() = default;

  SourceLocation(int Line, int Col);

  int line() const;
  int &line();

  int col() const;
  int &col();

  std::string toString(const unsigned depth = 0) const override;
};

/// ExprAST - Base class for all expression nodes.
class ExprAST : public Showable {
  /// Where in the source file this AST node appears.
  SourceLocation Loc;

public:
  explicit ExprAST(SourceLocation Loc);

  /// The destructor for the ExprAST class. Since the ExprAST class is just an
  /// abstract base class for all possible nodes of our AST, this is just an
  /// empty destructor.
  virtual ~ExprAST() = default;

  /// Generate LLVM IR for this AST node and all dependent AST nodes.
  virtual llvm::Value *codegen() = 0;

  const SourceLocation loc() const;
};

// Global variables used by subclasses of ExprAST.
llvm::LLVMContext &getContext();
llvm::IRBuilder<> &getBuilder();

llvm::DIBuilder &borrowDBuilder();
void newDBuilder();

llvm::Module &borrowModule();
void newModule(const char *newModuleName);
std::unique_ptr<llvm::Module> takeModule();

std::unordered_map<std::string, llvm::AllocaInst *> &getNamedValues();

llvm::legacy::FunctionPassManager *getFunctionPassManager();
void resetFunctionPassManager();

std::unordered_map<std::string, std::unique_ptr<PrototypeAST>> &
getFunctionProtos();

#endif // EXPRAST_H

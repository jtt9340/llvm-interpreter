#include <llvm/IR/Function.h>     // llvm::Function
#include <llvm/IR/Instructions.h> // llvm::PHINode, llvm::AllocaInst
#include <llvm/IR/Value.h>        // llvm::Value

#ifndef loop
/// Infinite loop.
#define loop for (;;)
#endif

#ifndef UTIL_H
#define UTIL_H

/// An interface for objects that can convert themselves
/// into a string representation for debugging.
struct Showable {
  /// Return a string representation of this object.
  ///
  /// @return a string representation of this object useful for debugging
  virtual std::string toString() const;
};

// Forward-declare ExprAST and PrototypeAST: these classes are needed in
// this file but we cannot #include "ExprAST.h" and #include "PrototypeAST.h"
// because they will #include this file, either directly or indirectly, thus
// causing a circular dependency.
class ExprAST;
class PrototypeAST;

/// Set up the internal module for the interpreter and initialize all
/// optimizations.
///
/// This function should be called before running the interpreter. It
/// will enable optimizations for generated function code and initialize
/// the LLVM module to store symbol names.
void InitializeModuleAndPassManager();

/// Search through the global llvm::Module for a function with the given name.
/// If no such function exists, generate a new function declaration, or return
/// nullptr if function code generation failed.
///
/// @param Name the name of the function to search for
/// @returns a function with the given name, or nullptr if the function didn't
///          exist and code generation for it failed
llvm::Function *getFunction(const std::string &Name);

/// Create an LLVM alloca instruction for storing a variable named VarName on
/// the stack inside of the given function. This alloca instruction will then
/// be turned into a register instruction by the mem2reg LLVM optimization,
/// which prevents the interpreter from having to use SSA (Static Single
/// Assignment. where every variable is written to exactly once and every
/// variable is declared before it is used) form to create local mutable
/// variables.
///
/// @param Function the function to create a local, stack-allocated variable for
/// @param VarName the name of the local variable
/// @return the alloca instruction
llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *Function,
                                         const std::string &VarName);

/// What to do when a function definition is encountered at the REPL.
void HandleDefinition();

/// What to do when an extern function delcaration is encountered at the REPL.
void HandleExtern();

/// What to do when any other expression that is not a function definition or
/// extern function declaration is encountered at the REPL.
void HandleTopLevelExpression();

/// These are basic helper functions for basic error handling.
std::unique_ptr<ExprAST> LogError(const char *Str);

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str);

llvm::Value *LogErrorV(const char *Str);

#endif // UTIL_H

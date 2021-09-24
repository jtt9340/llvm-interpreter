#include <llvm/IR/Function.h>          // llvm::Function
#include <llvm/IR/Instructions.h>      // llvm::PHINode, llvm::AllocaInst
#include <llvm/IR/LegacyPassManager.h> // llvm::legacy::PassManagerBase
#include <llvm/IR/Value.h>             // llvm::Value

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
  /// @param depth the level of indentation to print this object at,
  ///              useful for pretty-printing (may be ignored by implementation)
  /// @return a string representation of this object useful for debugging
  virtual std::string toString(const unsigned depth = 0) const;
};

struct DebugInfo {
  llvm::DICompileUnit *CompileUnit;
  llvm::DIType *DblTy;

  static DebugInfo *getInstance();

  llvm::DIType *getDoubleTy();

private:
  DebugInfo() = default;

  static std::unique_ptr<DebugInfo> TheInstance;
};

// Forward-declare ExprAST, PrototypeAST, and SourceLocation: these classes are needed in
// this file but we cannot #include "ExprAST.h" and #include "PrototypeAST.h"
// because they will #include this file, either directly or indirectly, thus
// causing a circular dependency.
class ExprAST;
class PrototypeAST;
class SourceLocation;

/// Sequence of characters that are considered whitespace.
#define WHITESPACE_CHARS " \f\n\r\t\v"

/// Apply a set of optimization passes to the given Pass Manager.
///
/// @param pass the Pass Manager instance to add passes to
void AddPasses(llvm::legacy::PassManagerBase *pass);

/// Set up the internal module for the interpreter and initialize all
/// optimizations.
///
/// This function should be called before running the interpreter. It
/// will enable optimizations for generated function code and initialize
/// the LLVM module to store symbol names.
///
/// This function configures whether or not the internal module's data layout
/// should be set to that of the native machine the interpreter is running on
/// and is controlled by the "native" parameter. If there is to be cross
/// compilation to object code, this parameter should be false.
///
/// @param native whether or not to additionally initialize the module for the
/// native
///        machine
void InitializeModuleAndPassManager(bool native);

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

/// Create a DISubroutineType that represents a function that takes NumArgs
/// number of parameters of type double (since Kaleidoscope only has one data
/// type: double) and returns a double (since Kaleidoscope is expression-based
/// and everything evaluates to a value regardless of the meaningfulness of that
/// value). This function cannot be abstracted to return a single type that can
/// be used in code generation of both function and emitting debug symbols,
/// since LLVM seems to have two different type hierarchies for functions and
/// DWARF objects.
///
/// @param NumArgs the number of args the function will accept
/// @param Unit unused
/// @return a subroutine type that describes the given function
llvm::DISubroutineType *CreateFunctionType(unsigned NumArgs,
                                           llvm::DIFile *Unit);

/// What to do when a function definition is encountered at the REPL.
///
/// @param native whether or not the function definition should be handled
///        relative to the native architecture the interpreter is running on
void HandleDefinition(bool native);

/// What to do when an extern function delcaration is encountered at the REPL.
void HandleExtern();

/// What to do when any other expression that is not a function definition or
/// extern function declaration is encountered at the REPL.
///
/// @param native whether or not the top-level expression should be handled
/// relative
///        to the native architecture the interpreter is running on
void HandleTopLevelExpression(bool native);

/// Helper function for adding n tabs to the output stream os.
///
/// @param os the ostream to write to
/// @param n the number of tab characters to write
/// @returns the output stream written to
std::ostream &insert_indent(std::ostream &os, const unsigned n);

/// Helper function for trimming whitespace from the beginning of a string.
///
/// @param s the string to trim the whitespace off the beginning of
/// @returns the given string with whitespace at the beginning removed.
///          Note that this function mutates the given string as well as
///          returning it
std::string &strltrim(std::string &s);

/// These are basic helper functions for basic error handling.
std::unique_ptr<ExprAST> LogError(const char *Str, const SourceLocation loc);

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str, const SourceLocation loc);

llvm::Value *LogErrorV(const char *Str, const SourceLocation loc);

#endif // UTIL_H

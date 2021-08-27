#include <cstdio>   // std::fputc, std::printf
#include <iostream> // std::cerr, std::endl

#include <llvm/Support/FileSystem.h>     // llvm::sys::fs::OF_None
#include <llvm/Support/Host.h>           // llvm::sys::getDefaultTargetTriple
#include <llvm/Support/TargetRegistry.h> // llvm::TargetRegistry
#include <llvm/Support/TargetSelect.h>   // llvm::InitializeNativeTarget, llvm::InitializeNativeTargetAsmPrinter, llvm::InitializeNativeTargetAsmParser

#include "KaleidoscopeJIT.h" // JIT
#include "lexer.h"
#include "parser.h" // ParseDefinition, ParseExtern, ParseTopLevelExpr

#define loop for (;;) // Infinite loop

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

/// The interpreter will look for any symbol in the same
/// address space as itself, so we can define new standard library
/// functions in C++ like this one, which prints out a given
/// character given its ASCII code.
///
/// @param c the ASCII code of the character to print out
/// @returns 0
extern "C" DLLEXPORT double putchard(double c) {
  std::fputc(static_cast<char>(c), stderr);
  return 0;
}

/// Print out a double to standard error. For use as a "standard libary"
/// function within the interpreter.
///
/// @param d the double to print out
/// @returns the given double
extern "C" DLLEXPORT double putd(double d) {
  std::fprintf(stderr, "%f\n", d);
  return d;
}

/// Run a REPL for interpreting expressions. This function runs an interactive
/// prompt in an infinite loop while there are still tokens to lex/parse.
///
/// top ::= definition | external | expression | ';'
///
/// The prompt given to the user can be contolled by the 'ProgName' parameter.
///
/// @param ProgName the name of the program do display to the user in the
///                 interactive REPL
static void MainLoop(const char *ProgName) {
  loop {
    std::cerr << ProgName << "> ";
    switch (getCurrentToken()) {
    case tok_eof:
      return;
    case ';':         // ignore top-level semicolons
      getNextToken(); // eat the ';'
      break;
    case tok_def:
      HandleDefinition();
      break;
    case tok_extern:
      HandleExtern();
      break;
    default:
      HandleTopLevelExpression();
    }
  }
}

int main(int argc, const char **argv) {
#if 0
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();
#endif
  SetupBinopPrecedences();
  InitializeModuleAndPassManager();

  // Get ready to parse the first token.
  std::cerr << argv[0] << "> ";
  getNextToken();

  // Run the REPL now.
  MainLoop(argv[0]);

  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();

  auto TargetTriple = llvm::sys::getDefaultTargetTriple();

  std::string Error;
  auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

  if (!Target) {
	  llvm::errs() << Error;
	  return 1;
  }

  auto CPU = "generic";
  auto Features = "";

  llvm::TargetOptions opt;
  auto RM = llvm::Optional<llvm::Reloc::Model>();
  auto TargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

  borrowModule().setDataLayout(TargetMachine->createDataLayout());
  borrowModule().setTargetTriple(TargetTriple);

  auto Filename = "session.o";
  std::error_code EC;
  llvm::raw_fd_ostream dest(Filename, EC, llvm::sys::fs::OF_None);

  if (EC) {
	  llvm::errs() << "Could not open " << Filename << ": " << EC.message();
	  return 1;
  }

  llvm::legacy::PassManager pass;
  auto FileType = llvm::CGFT_ObjectFile;

  if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
	  llvm::errs() << "Cannot emit a file of this type";
	  return 1;
  }

  pass.run(borrowModule());
  dest.flush();
  llvm::outs() << "Wrote " << Filename;

  return 0;
}

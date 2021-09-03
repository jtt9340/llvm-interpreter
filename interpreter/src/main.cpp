#include <cstdio>   // std::fputc, std::printf
#include <iostream> // std::cerr, std::endl

#include <llvm/Support/FileSystem.h>     // llvm::sys::fs::OF_None
#include <llvm/Support/Host.h>           // llvm::sys::getDefaultTargetTriple
#include <llvm/Support/TargetRegistry.h> // llvm::TargetRegistry
#include <llvm/Support/TargetSelect.h> // llvm::InitializeNativeTarget, llvm::InitializeNativeTargetAsmPrinter, llvm::InitializeNativeTargetAsmParser

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
static void MainLoop(const char *ProgName, bool native) {
  loop {
    std::cerr << ProgName << "> ";
    switch (getCurrentToken()) {
    case tok_eof:
      return;
    case ';':         // ignore top-level semicolons
      getNextToken(); // eat the ';'
      break;
    case tok_def:
      HandleDefinition(native);
      break;
    case tok_extern:
      HandleExtern();
      break;
    default:
      HandleTopLevelExpression(native);
    }
  }
}

static int usage(const char *argv0) {
  std::cerr << "usage: " << argv0 << " [help | <CPU architecture>] [<name>]"
            << std::endl;
  std::cerr << std::endl;
  std::cerr
      << "With no arguments, run the main interpreter loop.\n"
         "With <CPU architecture>, every function run in the interpreter loop\n"
         "will be compiled into an object file called <name> (\"session.o\" if "
         "not given)\n"
         "that matches the given CPU architecture. Run `llvm-as < /dev/null | "
         "llc -march=x86 -mattr=help`\n"
         "for a list of supported architectures. With \"help\", display this "
         "message."
      << std::endl;
  return 0;
}

int main(int argc, const char **argv) {
  const bool CompileToObjectCode = argc > 1;
  if (CompileToObjectCode) {
    std::string CPUOrHelp = argv[1];
    std::transform(
        CPUOrHelp.begin(), CPUOrHelp.end(), CPUOrHelp.begin(),
        [](unsigned char c) -> unsigned char { return std::tolower(c); });

    if (CPUOrHelp == "help") {
      return usage(argv[0]);
    }

    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
  } else {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
  }

  SetupBinopPrecedences();
  InitializeModuleAndPassManager(!CompileToObjectCode);

  // Get ready to parse the first token.
  std::cerr << argv[0] << "> ";
  getNextToken();

  // Run the REPL now.
  MainLoop(argv[0], !CompileToObjectCode);

  if (CompileToObjectCode) {
    auto TargetTriple = llvm::sys::getDefaultTargetTriple();

    std::string Error;
    auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);

    // Print an error and exit if we couldn't find the default target,
    // which would really only happen if the TargetRegistry is not initialized
    // or the TargetTriple is incorrect.
    if (!Target) {
      llvm::errs() << Error;
      return 1;
    }

    // Use the CPU architecture supplied at the command line.
    auto CPU = argv[1];

    // Do not add any additional features, options, or relocation models for
    // now.
    auto Features = "";
    llvm::TargetOptions opt;
    auto RM = llvm::Optional<llvm::Reloc::Model>();

    auto TargetMachine =
        Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

    borrowModule().setDataLayout(TargetMachine->createDataLayout());
    borrowModule().setTargetTriple(TargetTriple);

    auto Filename = argc > 2 ? argv[2] : "session.o";
    std::error_code EC;
    llvm::raw_fd_ostream dest(Filename, EC, llvm::sys::fs::OF_None);

    if (EC) {
      llvm::errs() << "Could not open " << Filename << ": " << EC.message();
      return 1;
    }

    llvm::legacy::PassManager pass;
    AddPasses(&pass);

    auto FileType = llvm::CGFT_ObjectFile;

    if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
      llvm::errs() << "Cannot emit a file of this type";
      return 1;
    }

    pass.run(borrowModule());
    dest.flush();
    llvm::outs() << "Wrote " << Filename;
  }

  return 0;
}

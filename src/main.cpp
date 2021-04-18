#include <iostream>  // std::cerr, std::endl
#include <cstdio>    // std::fputc

#include <llvm/Support/TargetSelect.h> // llvm::InitializeNativeTarget, llvm::InitializeNativeTargetAsmPrinter, llvm::InitializeNativeTargetAsmParser 

#include "KaleidoscopeJIT.h"  // JIT
#include "parser.h"           // ParseDefinition, ParseExtern, ParseTopLevelExpr
#include "repl.h"             // MainLoop, HandleDefinition, HandleExtern, HandleTopLevelExpression

#define loop for(;;)          // Infinite loop

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

int main(int argc, const char **argv) {
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeNativeTargetAsmParser();

	SetupBinopPrecedences();
	InitializeModuleAndPassManager();

	// Get ready to parse the first token.
	std::cerr << argv[0] << "> ";
	getNextToken();

	// Run the REPL now.
	MainLoop(argv[0]);

	return 0;
}

#include <cstdio>  // std::fprintf

#include <llvm/Support/TargetSelect.h> // llvm::InitializeNativeTarget, llvm::InitializeNativeTargetAsmPrinter, llvm::InitializeNativeTargetAsmParser 

#include "KaleidoscopeJIT.h"  // JIT
#include "lexer.h"            // SetupBinopPrecedences, getNextToken, MainLoop

using llvm::orc::KaleidoscopeJIT;

int main(int argc, const char **argv) {
	const char *argv0 = argv[0];
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmPrinter();
	llvm::InitializeNativeTargetAsmParser();

	SetupBinopPrecedences();
	InitializeModuleAndPassManager();

	// Get ready to parse the first token.
	std::fprintf(stderr, "%s> ", argv0);
	getNextToken();

	JIT = std::make_unique<KaleidoscopeJIT>();

	// Run the REPL now.
	MainLoop(argv0);

	return 0;
}

#include <iostream>  // std::cerr, std::endl

#include <llvm/Support/TargetSelect.h> // llvm::InitializeNativeTarget, llvm::InitializeNativeTargetAsmPrinter, llvm::InitializeNativeTargetAsmParser 

#include "KaleidoscopeJIT.h"  // JIT
#include "parser.h"           // ParseDefinition, ParseExtern, ParseTopLevelExpr
#include "repl.h"             // MainLoop, HandleDefinition, HandleExtern, HandleTopLevelExpression

#define loop for(;;)          // Infinite loop

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

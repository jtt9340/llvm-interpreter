#include <iostream>  // std::cerr, std::endl

#include <llvm/Support/TargetSelect.h> // llvm::InitializeNativeTarget, llvm::InitializeNativeTargetAsmPrinter, llvm::InitializeNativeTargetAsmParser 

#include "KaleidoscopeJIT.h"  // JIT
#include "parser.h"           // ParseDefinition, ParseExtern, ParseTopLevelExpr
#include "repl.h"             // MainLoop, HandleDefinition, HandleExtern, HandleTopLevelExpression

#define loop for(;;)          // Infinite loop

using llvm::orc::KaleidoscopeJIT;

void HandleDefinition() {
	const auto defn = ParseDefinition();
	if (defn) {
		const auto *ir = defn->codegen();
		if (ir) {
			std::cerr << "Generate LLVM IR for function definition:" << std::endl;
			ir->print(llvm::errs());
			std::cerr << std::endl;
		}
	} else {
		// Skip token to handle errors.
		getNextToken();
	}
}

void HandleExtern() {
	const auto externDeclaration = ParseExtern();
	if (externDeclaration) {
		const auto* ir = externDeclaration->codegen();
		if (ir) {
			std::cerr << "Generate LLVM IR for extern function declaration:" << std::endl;
			ir->print(llvm::errs());
			std::cerr << std::endl;
		}
	} else {
		// Skip token to handle errors.
		getNextToken();
	}
}

void HandleTopLevelExpression() {
	// Evaluate a top-level expression in an anonymous function.
	const auto expr = ParseTopLevelExpr();
	if (expr) {
		const auto *ir = expr->codegen();
		if (ir) {
			std::cerr << "Generate LLVM IR for top-level expression:" << std::endl;
			ir->print(llvm::errs());
			std::cerr << std::endl;
		}
	} else {
		// Skip token to handle errors.
		getNextToken();
	}
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

	JIT = std::make_unique<KaleidoscopeJIT>();

	// Run the REPL now.
	MainLoop(argv[0]);

	return 0;
}

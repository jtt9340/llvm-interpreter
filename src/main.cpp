#include <iostream>  // std::cerr, std::endl

#include <llvm/Support/TargetSelect.h> // llvm::InitializeNativeTarget, llvm::InitializeNativeTargetAsmPrinter, llvm::InitializeNativeTargetAsmParser 

#include "KaleidoscopeJIT.h"  // JIT
#include "lexer.h"            // SetupBinopPrecedences, getNextToken, MainLoop
#include "parser.h"           // ParseDefinition, ParseExtern, ParseTopLevelExpr

#define loop for(;;)          // Infinite loop

using llvm::orc::KaleidoscopeJIT;

static void HandleDefinition() {
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

static void HandleExtern() {
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

static void HandleTopLevelExpression() {
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
		case ';': // ignore top-level semicolons
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

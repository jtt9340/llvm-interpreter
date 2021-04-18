#include <iostream> // std::cout, std::endl

#include "../src/repl.h"   // MainLoop, HandleDefinition, HandleExtern, HandleTopLevelExpression
#include "../src/ast.h"
#include "../src/parser.h" // ParseTopLevelExpr

void HandleDefinition() {
	const auto defn = ParseDefinition();
	if (defn) {
		std::cout << defn->toString() << std::endl;
	} else {
		getNextToken();
	}
}

void HandleExtern() {
	const auto externDeclaration = ParseExtern();
	if (externDeclaration) {
		std::cout << externDeclaration->toString() << std::endl;
	} else {
		getNextToken();
	}
}

void HandleTopLevelExpression() {
	const auto expr = ParseTopLevelExpr();
	if (expr) {
		std::cout << expr->toString() << std::endl;
	} else {
		getNextToken();
	}
}

int main(int argc, const char **argv) {
	SetupBinopPrecedences();

	std::cerr << argv[0] << "> ";
	getNextToken();

	MainLoop(argv[0]);
	return 0;
}

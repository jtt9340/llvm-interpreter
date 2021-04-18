#include <iostream> // std::cout, std::endl

#include "../src/ast.h"
#include "../src/lexer.h"
#include "../src/parser.h" // ParseTopLevelExpr

#define loop for(;;)

int main(int argc, const char **argv) {
	SetupBinopPrecedences();

	std::cerr << argv[0] << "> ";
	getNextToken();

	loop {
		std::cerr << argv[0] << "> ";
		switch(getCurrentToken()) {
		case tok_eof:
			return 0;
		case ';': // ignore top-level semicolons
			getNextToken(); // eat the ';'
			break;
		case tok_def: {
			if (const auto defn = ParseDefinition())
				std::cout << defn->toString() << std::endl;
			else
				getNextToken();	
		}
		case tok_extern: {
			if (const auto externDeclaration = ParseExtern())
				std::cout << externDeclaration->toString() << std::endl;
			else
				getNextToken();
		}
		default: {
			if (const auto expr = ParseTopLevelExpr())
				std::cout << expr->toString() << std::endl;
			else
				getNextToken();
		}
		}
	}
}

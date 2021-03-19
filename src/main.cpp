#include "lexer.h"

int main() {
	// Create the binary operators, specifying their precedences.
	// The lower the number, the lower the precedence.
	// TODO: Add more binary operators
	BinopPrecedence['<'] = 10; // Lowest precedence
	BinopPrecedence['+'] = 20;
	BinopPrecedence['-'] = 20;
	BinopPrecedence['*'] = 40; // Highest precedence
}

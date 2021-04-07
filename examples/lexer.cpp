#include <iostream> // std::cout, std::cerr, std::endl

#include "../src/lexer.h"

int main() {
	const int tok = getNextToken();
	switch (tok) {
	case tok_eof:
		std::cout << "EOF (" << tok_eof << ')';
		break;
	case tok_err:
		std::cerr << "invalid token" << std::endl;
		return tok_err;
	case tok_def:
		std::cout << "def (" << tok_def << ')';
		break;
	case tok_extern:
		std::cout << "extern (" << tok_extern << ')';
		break;
	case tok_identifier:
		std::cout << "identifier (" << tok_identifier << ')';
		break;
	case tok_number:
		std::cout << "number (" << tok_number << ')';
		break;
	default:
		std::cout << "unrecognized token " << static_cast<char>(tok) <<
			 " (" << tok << ')';
	}

	std::cout << std::endl;
	return 0;
}

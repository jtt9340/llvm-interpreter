#include <string>  // std::string

#include <cctype>  // std::isspace, std::isalpha, std::isalnum, std::isdigit
#include <cstdio>  // std::getchar, EOF
#include <cstdlib> // std::strtod

#include "lexer.h" // gettok, enum Token

static std::string IdentifierStr; // Filled in if tok_identifier
static double NumVal;			  // Filled in if tok_number

// gettok - Return the next token from standard input.
int gettok() {
	static int LastChar = ' ';

	// Skip any whitespace.
	while (std::isspace(LastChar))
		LastChar = std::getchar();

	// Parse identifier and specific keywords
	if (std::isalpha(LastChar)) { // Identifier: [a-zA-Z][a-zA-Z0-9]*
		IdentifierStr = LastChar;
		while (std::isalnum((LastChar = std::getchar())))
			IdentifierStr += LastChar;

		if (IdentifierStr == "def")
			return tok_def;
		if (IdentifierStr == "extern")
			return tok_extern;
		return tok_identifier;
	}

	// Parse numeric values and store them in NumVal
	// TODO: This will currently parse 1.23.45.67 and the like as 1.23
	// Fix it so that it only parses single decimal points
	if (std::isdigit(LastChar) || LastChar == '.') {
		std::string NumStr;
		do {
			NumStr += LastChar;
			LastChar = std::getchar();
		} while (std::isdigit(LastChar) || LastChar == '.');

		NumVal = std::strtod(NumStr.c_str(), 0);
		return tok_number;
	}

	// Comments start with a # and go until the end of the line, at which point we return the next token
	if (LastChar == '#') {
		do
			LastChar = std::getchar();
		while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');
	
		if (LastChar != EOF)
			return gettok();
	}

	// Handle operators and EOF

	// Check for EOF but don't eat the EOF
	if (LastChar == EOF)
		return tok_eof;

	// Otherwise, just return the character as its ascii value
	int ThisChar = LastChar;
	LastChar = std::getchar();
	return ThisChar;
}

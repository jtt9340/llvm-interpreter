#include <string>  // std::string
#include <cstring> // std::strlen

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
	std::string NumStr;
	if (LastChar == '.') {
		// If the numeric value started with a '.', then we must accept at least one number
		// and can only accept numbers
		do {
			NumStr += LastChar;
			LastChar = std::getchar();
		} while (std::isdigit(LastChar));

		// Additional byte storage for the parts of NumStr that couldn't be parsed as a number
		// Used to indicate an invalid number token
		char *NumStrEnd = nullptr; 

		NumVal = std::strtod(NumStr.c_str(), &NumStrEnd);
		if (std::strlen(NumStrEnd)) {
			// There were parts of NumStr that could not be parsed as a double, so this is an
			// invalid token
			return tok_err;
		}

		return tok_number;
	} else if (std::isdigit(LastChar)) {
		// If we have encountered a decimal point yet
		bool DecimalPointFound = false;
		
		// Read either numbers or .'s until one . is found
		do {
			NumStr += LastChar;
			LastChar = std::getchar();
			if (!DecimalPointFound && LastChar == '.') {
				// This is our first decimal point so we accept it
				DecimalPointFound = true;
			} else if (LastChar == '.') {
				// This is not our first decomal point so this is an error
				return tok_err;
			}
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

	// Otherwise, just return the character as its ASCII value
	int ThisChar = LastChar;
	LastChar = std::getchar();
	return ThisChar;
}

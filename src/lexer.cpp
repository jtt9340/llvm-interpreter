#include <cstring>		// std::strlen
#include <sstream>      // std::ostringstream
#include <cctype>		// std::isspace, std::isalpha, std::isalnum, std::isdigit
#include <cstdio>		// std::getchar, EOF
#include <cstdlib>		// std::strtod
#include <iostream>     // std::cerr, std::endl
#include <unordered_map>// std::unordered_map

#include <llvm/IR/Function.h> // llvm::Function

#include "ast.h"		// class ExprAST, class NumberExprAST, class VariableExprAST,
				// class BinaryExprAST, class CallExprAST, class PrototypeAST, class FunctionAST
#include "lexer.h"		// gettok, enum Token
#include "logging.h"	// LogError, LogErrorP

#define loop for(;;)	// Infinite loop

static std::string IdentifierStr;   // Filled in if tok_identifier
static double      NumVal;          // Filled in if tok_number

const std::string &getIdentifierStr() { return IdentifierStr; }

double getNumVal() { return NumVal; }

const std::string tokenToString(Token tok) {
	std::ostringstream output;
	switch (tok) {
	case tok_eof:
		output << "EOF";
		break;
	case tok_err:
		output << "invalid token";
		break;
	case tok_def:
		output << "def";
		break;
	case tok_extern:
		output << "extern";
		break;
	case tok_identifier:
		output << "identifier";
		break;
	case tok_number:
		output << "number";
		break;
	case tok_if:
		output << "if";
		break;
	case tok_then:
		output << "then";
		break;
	case tok_else:
		output << "else";
		break;
	default:
		output << "unrecognized token " << static_cast<char>(tok);
	}
 	output << " (" << tok << ')';
	return output.str();
}

/// CurTok/getNextToken - Provide a simple token buffer. CurTok is the
/// current token the parser is looking at. getNextToken reads another
/// token from the lexer and updates CurTok with its results.
static int CurTok;
int getNextToken() {
	// gettok() is forward-declared in lexer.h so we can call it here even
	// though the definition does not appear until below
	return CurTok = gettok();
}

int getCurrentToken() { return CurTok; }

// gettok - Return the next token from standard input.
static int gettok() {
	static int LastChar = ' ';

	// Skip any whitespace.
	while (std::isspace(LastChar))
		LastChar = std::getchar();

	// Parse identifier and specific keywords
	if (std::isalpha(LastChar) || LastChar == '_' || LastChar == '$') { // Identifier: [a-zA-Z$_][a-zA-Z0-9$_]*
		IdentifierStr = LastChar;
		while ((LastChar = std::getchar()), std::isalnum(LastChar) || LastChar == '_' || LastChar == '$')
			IdentifierStr += LastChar;

		if (IdentifierStr == "def")
			return tok_def;
		if (IdentifierStr == "extern")
			return tok_extern;
		if (IdentifierStr == "if")
			return tok_if;
		if (IdentifierStr == "then")
			return tok_then;
		if (IdentifierStr == "else")
			return tok_else;
		return tok_identifier;
	}

	// Parse numeric values and store them in NumVal
	std::string NumStr;

	/*	
		// TODO: Handle negative numbers
		if (LastChar == '-') {
			NumStr += LastChar;
			LastChar = std::getchar();
		}
	*/

	if (LastChar == '.') {
		// If the numeric value started with a '.', then we must accept at least one number
		// and can only accept numbers
		do {
			NumStr += LastChar;
			LastChar = std::getchar();
		} while (std::isdigit(LastChar));

		// It is an error to have a letter immediately follow a number
		if (std::isalpha(LastChar))
			return tok_err;

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
				// This is not our first decimal point so this is an error
				return tok_err;
			}
		} while (std::isdigit(LastChar) || LastChar == '.');
		
		// It is an error to have a letter immediately follow a number
		if (std::isalpha(LastChar))
			return tok_err;

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


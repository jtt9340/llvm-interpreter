#ifndef LEXER_H
#define LEXER_H

#include "ast.h"

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum Token {
	tok_eof = -1,
	// An incorrect token as parsed
	tok_err = -2,

	// commands
	tok_def = -3,
	tok_extern = -4,

	// primary
	tok_identifier = -5,
	tok_number = -6,
};

/// Read from standard input until a token is read.
///
/// This function lexes standard input, reading from standard input
/// until it can return an entire token. A token is one of
///
///			EOF
///			A keyword
///			An identifier (e.g. a variable or function name)
///			A (floating point) number
///			Another unrecognized character
///
/// Once a token is found, the integer corresping to that token is returned.
/// If another unrecognized character is found, its ASCII value is returned.
///
/// @return an integer, either an enum Token if the lexed token is a recognized token,
///         or the ASCII value of the first character read
static int gettok();

/// Get the next token as lexed by gettok(), updating the internal token buffer.
/// When getting the next token, this funcion should be preferred to gettok()
/// since gettok() does not update the internal token buffer, and getting
/// the internal token buffer out of sync with the parser state could lead
/// to all sorts of trouble.
///
/// @return the next token lexed by gettok()
int getNextToken();

int getCurrentToken();

const std::string &getIdentifierStr();

double getNumVal();

#endif

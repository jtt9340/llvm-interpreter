#ifndef LEXER_H
#define LEXER_H

#include <unordered_map>

#include "ast.h"

static std::unordered_map<char, int> BinopPrecedence;	// This holds the precedence for each binary operator that is defined

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
/// 		EOF
///			A keyword
///			An identifier (e.g. a variable or function name)
///			A (floating point) number
///			Another unrecognized character
///
/// Once a token is found, the integer corresping to that token is returned.
/// If another unrecognized character is found, its ASCII value is returned.
///
/// @return an integer, either an enum Token if the lexed token is a recognized token,
/// 		or the ASCII value of the first character read
int gettok();

// TODO: Documentation
static std::unique_ptr<ExprAST> ParseNumberExpr();

// TODO: Documentation
static std::unique_ptr<ExprAST> ParseParenExpr();

// TODO: Documentation
static std::unique_ptr<ExprAST> ParseIdentififerExpr();

// TODO: Documentation
static std::unique_ptr<ExprAST> ParsePrimary();

// TODO: Documentation
static int GetTokPrecedence();

// TODO: Documentation
static std::unique_ptr<ExprAST> ParseExpression();

#endif

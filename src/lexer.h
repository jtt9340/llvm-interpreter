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
int gettok();

/// Transform the floating point number tokenized by gettok() into an AST node that
/// represents a numerical expression.
///
/// Calling gettok() will store the number it tokenizes into a global variable, if
/// the token it finds is a parse-able (floating-point) number. Calling this function
/// will then use that global variable to turn that into an AST node for a numerical
/// expression, so
///
///         (1) gettok() should be called before calling this function to find get
///             the next numerical token in the source code
///         (2) the user should check that gettok() indeed found a tok_number, or else
///             the numerical value that this function will use to build the AST node
///             will be inaccurate. 
///
/// @return an AST node wrapping a numerical value
static std::unique_ptr<ExprAST> ParseNumberExpr();

/// Consume a '(' token, then another expression, and then another ')' token, and build
/// an AST node representing the value in parenthesis.
///
/// This function assumes the current token that was lexed by gettok() is an opening
/// parenthesis. Then, it tries to find another expression to parse, returning a
/// nullptr if there is not one. This would mean this function encountered an opening
/// parenthesis but nothing after. Finally, it looks for a closing parenthesis, again
/// returning a nullptr if it does not find one.
///
/// @return an AST node that represents the value after evaluating the
///         expression in parentheses, or a nullptr as described above
static std::unique_ptr<ExprAST> ParseParenExpr();

/// Create an AST node representing either a variable reference or a function call.
///
/// If this function finds a word without an opening parenthesis, it is a variable
/// reference and the AST node returned represents the name of the variable. If
/// parenthesis follow the word, then it is a function call and the AST node returned
/// represents this.
///
/// This function assumes the current token that was lexed by gettok() is a tok_identifier,
/// so it must only be called when gettok() returns tok_identifier.
///
/// @return an AST node represting either a variable reference or a function call
static std::unique_ptr<ExprAST> ParseIdentififerExpr();

/// This function will parse one of the following:
///
///			(1) A variable reference
///			(2) A function call
///			(3) A numerical expression
///			(4) A parenthetical expression
///
/// Based on the current token as returned by gettok(). This is a "top level" AST
/// node builder that calls one of the other Parse* functions to create an AST node.
///
/// @return an AST node based on the current token lexed by gettok() 
static std::unique_ptr<ExprAST> ParsePrimary();

/// If the current token as returned by gettok() is a binary operator, return the
/// precedence of that operator, otherwise return -1.
///
/// This function is used for implementing the order of operations, PEMDAS, when
/// parsing expressions with binary operators to ensure that, e.g. * is evaluated before +.
///
/// The higher the precedence, the higher priority that binary operator will have when
/// being evaluated.
///
/// @return the predence of the current binary operator stored in the token returned by gettok(),
///         or -1 if the current token is not a valid binary operator
static int GetTokPrecedence();

/// Parse a sequence of primary expressions (see ParsePrimary) conjoined by binary operators, after
/// having already parsed the initial primary expression in the sequence. In other words, given a
/// sequence of primary expressions and binary operators
///
/// 		e_1 op_1 e_2 op_2 e_3 ... op_n e_n
///
/// where e_n is the nth primary expression in the sequence and op_n is the nth binary operator in
/// the sequence, this function will parse the given sequence, starting at op_1, given the AST node
/// representing e_1 in the form of the 'LHS' paremeter. This function is named ParseBinOpRHS since
/// it parses the Right-Hand Side of binary operators, stopping at the first binary operator whose
/// precednece is lower than 'ExprPrec'. Hence why the Left Hand Side must have already been parsed.
///
/// An AST node (tree) is returned representing the structure of the expression parsed. If a sequence
/// ends in a binary operator with no Right-Hand Side, a nullptr is returned. To parse an
/// entire expression, starting with the first primary expression and not the first binary operator,
/// call ParseExpression.
///
/// @param ExprPrec the minimum allowed binary operator precedence to parse
/// @param LHS the expression on the left-hand side of the current binary operator to parse
/// @return an AST node representing the entire expression parsed up to, but not including,
///         the first binary operator with too low of a predence, or up to the end of the expression
///         if no such binary operator is encountered, or nullptr is a binary operator has no Right-Hand
///         Side
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                              std::unique_ptr<ExprAST> LHS);

/// Parse a sequence of primary expressions conjoined by binary operators. This is very similar to
/// ParseBinOpRHS, in fact it just parses the leftmost primary expression, then calls ParseBinOrRHS
/// with the remainder of the expression. This function is necessary because ParseBinOpRHS expects the
/// current token to be a binary operator so that it can parse the Right-Hand Side of the binary operator
/// and compare the precedence of the current binary operator to the predence of the next binary operator,
/// so this function handles parsing the left-most expression before the first binary operator.
///
/// @return an AST node (tree) represending the parsed expression
static std::unique_ptr<ExprAST> ParseExpression();

#endif

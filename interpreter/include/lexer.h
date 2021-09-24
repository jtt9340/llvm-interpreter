#include "ExprAST.h" // SourceLocation

#ifndef LEXER_H
#define LEXER_H

/// The lexer returns tokens [0-255] if it is an unknown character, otherwise
/// one of these for known things.
enum Token {
  /// EOF: We have reacted the end of the file
  tok_eof = -1,
  /// An incorrect token was parsed
  tok_err = -2,

  // functions
  /// The "def" keyword
  tok_def = -3,
  /// The "extern" keyword
  tok_extern = -4,

  // primary
  /// An identifier, such as a variable or function name
  tok_identifier = -5,
  /// A double-precision floating point number
  tok_number = -6,

  // control flow
  /// The "if" keyword
  tok_if = -7,
  /// The "then" keyword
  tok_then = -8,
  /// The "else" keyword
  tok_else = -9,
  /// The "for" keyword
  tok_for = -10,
  /// The "in" keyword
  tok_in = -11,

  // user-defined operators
  /// The "binary" keyword
  tok_binary = -12,
  /// The "unary" keyword
  tok_unary = -13,

  // user-defined local variables
  /// the "let" keyword
  tok_let = -14
};

/// Return a string representation of enum Token suitable for printing
/// to the console. Think of this function as an override the the 'toString()'
/// method in Java for the Token enum.
///
/// @return a string representation of the given token
const std::string tokenToString(Token tok);

/// Read from standard input until a token is read.
///
/// This function lexes standard input, reading from standard input
/// until it can return an entire token. A token is one of
///
///         EOF
///         A keyword
///         An identifier (e.g. a variable or function name)
///         A (floating point) number
///         Another unrecognized character
///
/// Once a token is found, the integer corresping to that token is returned.
/// If another unrecognized character is found, its ASCII value is returned.
///
/// @return an integer, either an enum Token if the lexed token is a recognized
///         token, or the ASCII value of the first character read
static std::pair<int, SourceLocation> gettok();

/// Get the next token as lexed by gettok(), updating the internal token buffer.
/// When getting the next token, this funcion should be preferred to gettok()
/// since gettok() does not update the internal token buffer, and getting
/// the internal token buffer out of sync with the parser state could lead
/// to all sorts of trouble.
///
/// @return the next token lexed by gettok()
std::pair<int, SourceLocation> getNextToken();

/// Get the current token in the token buffer without updating it.
///
/// The difference between this function and getNextToken() is that
/// getNextToken() will first read in a token from standard input,
/// update the token buffer to store it, then return the new token.
/// This function just simply returns whateer token is currently stored
/// in the buffer, or uninitialzed data if not token has been read yet.
///
/// @return the token currently in the token buffer
int getCurrentToken();

/// Get the name of the current identifier (variable or function name)
/// last read by the lexer. This will be the empty string if no identifier
/// has been read.
///
/// @return the last read identifier as a read-only reference
const std::string &getIdentifierStr();

/// Get the current numeric value last read in by the lexer. This will be
/// uninitialized data if no numeric literal has been lexed yet.
///
/// @return the last read numeric literal
double getNumVal();

#endif // LEXER_h

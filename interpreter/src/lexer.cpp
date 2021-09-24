#include <cctype>   // std::isspace, std::isalpha, std::isalnum, std::isdigit
#include <cstdio>   // std::getchar, EOF
#include <cstdlib>  // std::strtod
#include <iostream> // std::cerr, std::endl
#include <sstream>  // std::ostringstream
#include <unordered_map> // std::unordered_map

#include <llvm/IR/Function.h> // llvm::Function

#include "ExprAST.h" // SourceLocation
#include "lexer.h"   // gettok, enum Token
#include "util.h"    // loop, LogError, LogErrorP

static std::string IdentifierStr; // Filled in if tok_identifier
static double NumVal;             // Filled in if tok_number

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
  case tok_for:
    output << "for";
    break;
  case tok_in:
    output << "in";
    break;
  case tok_binary:
    output << "binary";
    break;
  case tok_unary:
    output << "unary";
    break;
  case tok_let:
    output << "let";
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
std::pair<int, SourceLocation> getNextToken() {
  // gettok() is forward-declared in lexer.h so we can call it here even
  // though the definition does not appear until below
  auto charLocPair = gettok();
  CurTok = charLocPair.first;
  return charLocPair;
}

int getCurrentToken() { return CurTok; }

static std::pair<int, const SourceLocation> advance() {
  int LastChar = std::getchar();
  static SourceLocation LexLoc(1, 0);

  if (LastChar == '\n' || LastChar == '\r') {
    LexLoc.line()++;
    LexLoc.col() = 0;
  } else
    LexLoc.col()++;
  return std::make_pair(LastChar, LexLoc);
}

// gettok - Return the next token from standard input.
static std::pair<int, SourceLocation> gettok() {
  static int LastChar = ' ';
  SourceLocation CurLoc;

  // Skip any whitespace.
  while (std::isspace(LastChar)) {
    auto charLocPair = advance();
    LastChar = charLocPair.first;
    CurLoc = charLocPair.second;
  }

  // Parse identifier and specific keywords
  if (std::isalpha(LastChar) || LastChar == '_' ||
      LastChar == '$') { // Identifier: [a-zA-Z$_][a-zA-Z0-9$_]*
    IdentifierStr = LastChar;
    LastChar = advance().first;
    while (std::isalnum(LastChar) || LastChar == '_' || LastChar == '$') {
      IdentifierStr += LastChar;
	  LastChar = advance().first;
    }

    if (IdentifierStr == "def")
      return std::make_pair(tok_def, CurLoc);
    if (IdentifierStr == "extern")
      return std::make_pair(tok_extern, CurLoc);
    if (IdentifierStr == "if")
      return std::make_pair(tok_if, CurLoc);
    if (IdentifierStr == "then")
      return std::make_pair(tok_then, CurLoc);
    if (IdentifierStr == "else")
      return std::make_pair(tok_else, CurLoc);
    if (IdentifierStr == "for")
      return std::make_pair(tok_for, CurLoc);
    if (IdentifierStr == "in")
      return std::make_pair(tok_in, CurLoc);
    if (IdentifierStr == "binary")
      return std::make_pair(tok_binary, CurLoc);
    if (IdentifierStr == "unary")
      return std::make_pair(tok_unary, CurLoc);
    if (IdentifierStr == "let")
      return std::make_pair(tok_let, CurLoc);
    return std::make_pair(tok_identifier, CurLoc);
  }

  // Parse numeric values and store them in NumVal
  std::string NumStr;

  if (LastChar == '.') {
    std::pair<int, SourceLocation> charLocPair;
    // If the numeric value started with a '.', then we must accept at least one
    // number and can only accept numbers
    do {
      NumStr += LastChar;
      charLocPair = advance();
      LastChar = charLocPair.first;
    } while (std::isdigit(LastChar));

    // It is an error to have a letter immediately follow a number
    if (std::isalpha(LastChar))
      return std::make_pair(tok_err, charLocPair.second);

    // Additional byte storage for the parts of NumStr that couldn't be parsed
    // as a number, used to indicate an invalid number token
    char *NumStrEnd = nullptr;

    NumVal = std::strtod(NumStr.c_str(), &NumStrEnd);
    if (NumStrEnd) {
      // There were parts of NumStr that could not be parsed as a double, so
      // this is an invalid token
      return std::make_pair(tok_err, CurLoc);
    }

    return std::make_pair(tok_number, CurLoc);
  } else if (std::isdigit(LastChar)) {
    std::pair<int, SourceLocation> charLocPair;
    // Have we encountered a decimal point yet?
    bool DecimalPointFound = false;

    // Read either numbers or .'s until one . is found.
    do {
      NumStr += LastChar;
      charLocPair = advance();
      LastChar = charLocPair.first;
      if (!DecimalPointFound && LastChar == '.') {
        // This is our first decimal point so we accept it
        DecimalPointFound = true;
      } else if (LastChar == '.') {
        // This is not our first decimal point so this is an error
        return std::make_pair(tok_err, charLocPair.second);
      }
    } while (std::isdigit(LastChar) || LastChar == '.');

    // It is an error to have a letter immediately follow a number
    if (std::isalpha(LastChar))
      return std::make_pair(tok_err, charLocPair.second);

    NumVal = std::strtod(NumStr.c_str(), 0);
    return std::make_pair(tok_number, CurLoc);
  }

  // Comments start with a # and go until the end of the line, at which point we
  // return the next token
  if (LastChar == '#') {
    do {
      auto charLocPair = advance();
      LastChar = charLocPair.first;
      CurLoc = charLocPair.second;
    } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

    if (LastChar != EOF)
      return gettok();
  }

  // Handle operators and EOF

  // Check for EOF but don't eat the EOF
  if (LastChar == EOF)
    return std::make_pair(tok_eof, CurLoc);

  // Otherwise, just return the character as its ASCII value
  int ThisChar = LastChar;
  LastChar = advance().first;
  return std::make_pair(ThisChar, CurLoc);
}

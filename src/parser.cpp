#include <sstream> // std::ostringstream
#include <unordered_map>

#include "lexer.h"
#include "logging.h"
#include "parser.h"

#define loop for (;;) // Infinite loop

// This holds the precedence for each binary operator that is defined
static std::unordered_map<char, int> BinopPrecedence;

void SetupBinopPrecedences() {
  // Create the binary operators, specifying their precedences.
  // The lower the number, the lower the precedence.
  // TODO: Add more binary operators
  BinopPrecedence['<'] = 10; // Lowest precedence
  BinopPrecedence['>'] = 10;
  BinopPrecedence['+'] = 20;
  BinopPrecedence['-'] = 20;
  BinopPrecedence['*'] = 40;
  BinopPrecedence['/'] = 40; // Highest precedence
}

int InstallBinopPrecedence(const char Op, const int Precedence) {
  return BinopPrecedence[Op] = Precedence;
}

/// numberexpr ::= number
std::unique_ptr<ExprAST> ParseNumberExpr() {
  auto Result = std::make_unique<NumberExprAST>(getNumVal());
  getNextToken(); // consume the number
  return std::move(Result);
}

/// parenexpr ::= '(' expression  ')'
std::unique_ptr<ExprAST> ParseParenExpr() {
  getNextToken();             // consume '('
  auto V = ParseExpression(); // the 'expression' part in our production above
  if (!V)
    return nullptr; // we did not find an expression, just an (

  if (getCurrentToken() != ')')
    return LogError("expected ')'");
  getNextToken(); // consume ')'
  return V;
}

/// identifier
///   ::= identifier                      Variable references.
///   ::= identifier '(' expression ')'   Function calls.
std::unique_ptr<ExprAST> ParseIdentifierExpr() {
  const std::string IdName = getIdentifierStr();

  getNextToken(); // Consume the identififer

  if (getCurrentToken() != '(') {
    // This is a variable reference, not a function call
    return std::make_unique<VariableExprAST>(IdName);
  }

  // Otherwise, this is a function call
  getNextToken(); // Consume the '('
  std::vector<std::unique_ptr<ExprAST>> Args;
  if (getCurrentToken() != ')') {
    loop {
      if (auto Arg = ParseExpression())
        Args.push_back(std::move(Arg));
      else
        return nullptr; // Expected an expression

      if (getCurrentToken() == ')')
        break; // We've finished parsing the function call

      if (getCurrentToken() != ',')
        return LogError("Expected ')' or ',' in argument list");
      getNextToken(); // Consume the function argument expression
    }
  }

  getNextToken(); // Consume the ')'

  return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

/// primary
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
/// Determine the type of expression we are parsing.
std::unique_ptr<ExprAST> ParsePrimary() {
  switch (getCurrentToken()) {
  case tok_identifier:
    return ParseIdentifierExpr();
  case tok_number:
    return ParseNumberExpr();
  case '(':
    return ParseParenExpr(); // An expression arbitrarily wrapped in parentheses
  case tok_if:
    return ParseIfExpr();
  case tok_for:
    return ParseForExpr();
  default:
    return LogError("unknown token when expecting an expression");
  }
}

// Get the precedence of the pending binary operator token
int GetTokPrecedence() {
  const int CurTok = getCurrentToken();
  // If CurTok is NOT an ASCII character, assuming an ASCII or UTF-8 encoding
  if (CurTok < 0 || CurTok >= 128)
    return -1;

  const int TokPrec = BinopPrecedence[CurTok];
  // Ensure the pending binary operator token is a recognized binary operator
  return TokPrec <= 0 ? -1 : TokPrec;
}

/// binoprhs
///   ::= ('+' primary)*
std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                       std::unique_ptr<ExprAST> LHS) {
  // If this is a binary operator, find its precedence
  loop {
    int TokPrec = GetTokPrecedence();

    // If this is a binary operator that binds with at least as tightly as the
    // current binary operator, consume it, otherwise we are done.
    if (TokPrec < ExprPrec)
      return LHS;

    // At this point, we are indeed parsing a binary operator with a high enough
    // precedence. If it wasn't a binary operator, TokPrec would be -1 which
    // would indeed be < Exprec, so we wouldn't have gotten here by now.
    int BinOp = getCurrentToken();
    getNextToken(); // eat the binary operator

    // Parse the primary expression after the binary operator.
    auto RHS = ParsePrimary();
    if (!RHS)
      return nullptr;

    // If the current binary operator has lower precedence than the binary
    // operator after the RHS, then let the current RHS be the LHS in the next
    // binary operator.
    int NextPrec = GetTokPrecedence();
    if (TokPrec < NextPrec) {
      RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
      if (!RHS)
        return nullptr;
    }

    // Otherwise, the current binary operator takes precedence, so let's build
    // an AST node containing the current binary operator and its operands.
    LHS =
        std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
  } // Loop back to the top, looking for more binary operators until there are
    // no more expressions to parse.
}

/// expression
///   ::= primary binoprhs
///
std::unique_ptr<ExprAST> ParseExpression() {
  auto LHS = ParsePrimary();
  // Attempt to parse an expression; if it is successfull (a valid token)
  // then parse a potential RHS in case it is a binary operator
  return LHS ? ParseBinOpRHS(0, std::move(LHS)) : nullptr;
}

/// prototype
///   ::= id '(' id* ')'
///   ::= binary LETTER number? (id, id)
std::unique_ptr<PrototypeAST> ParsePrototype() {
  std::string FnName;

  enum { identifier, unary, binary } Kind;
  unsigned BinaryPrecedence = 30;

  switch (getCurrentToken()) {
  case tok_identifier:
    FnName = getIdentifierStr();
    Kind = identifier;
    getNextToken();
    break;
  case tok_binary:
    getNextToken();
    // If CurTok is NOT an ASCII character, assuming an ASCII or UTF-8 encoding
    if (getCurrentToken() < 0 || getCurrentToken() > 128) {
      std::ostringstream errMsg("Expected binary operator but found ",
                                std::ios_base::ate);
      errMsg << tokenToString(static_cast<Token>(getCurrentToken()));
      return LogErrorP(errMsg.str().c_str());
    }
    FnName = std::string("binary") + static_cast<char>(getCurrentToken());
    Kind = binary;
    getNextToken();

    // Read the precedence if present
    if (getCurrentToken() == tok_number) {
      if (getNumVal() < 1 || getNumVal() > 100) {
        std::ostringstream errMsg("Invalid predence ", std::ios_base::ate);
        errMsg << getNumVal() << ": must be >= 1 or <= 100";
        return LogErrorP(errMsg.str().c_str());
      }
      BinaryPrecedence = static_cast<unsigned>(getNumVal());
      getNextToken();
    }
    break;
  default:
    std::ostringstream errMsg("Expected function name in prototype but found ",
                              std::ios_base::ate);
    errMsg << tokenToString(static_cast<Token>(getCurrentToken()));
    return LogErrorP(errMsg.str().c_str());
  }

  if (getCurrentToken() != '(') {
    std::ostringstream errMsg("Expected '(' in prototype but found ");
    errMsg << tokenToString(static_cast<Token>(getCurrentToken()));
    return LogErrorP(errMsg.str().c_str());
  }

  // Read the list of argument names.
  std::vector<std::string> ArgNames;
  while (getNextToken() == tok_identifier)
    ArgNames.push_back(getIdentifierStr());
  if (getCurrentToken() != ')') {
    std::ostringstream errMsg("Expected ')' in prototype but found ");
    errMsg << tokenToString(static_cast<Token>(getCurrentToken()));
    return LogErrorP(errMsg.str().c_str());
  }

  // We successfully parsed a function prototype.
  getNextToken(); // Eat the ')'

  // Verify that an operator was declared to have the correct
  // number of operands
  if (Kind && ArgNames.size() != Kind) {
    std::ostringstream errMsg("Invalid number of operands for operator ");
    errMsg << FnName << ": expected " << Kind << " but got " << ArgNames.size();
    return LogErrorP(errMsg.str().c_str());
  }

  return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames), Kind != 0,
                                        BinaryPrecedence);
}

/// definition ::= 'def' prototype expression
std::unique_ptr<FunctionAST> ParseDefinition() {
  getNextToken(); // eat the 'def' keyword
  auto Proto = ParsePrototype();
  if (!Proto)
    return nullptr;

  if (auto E = ParseExpression())
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
  return nullptr;
}

/// external ::= 'extern' prototype
std::unique_ptr<PrototypeAST> ParseExtern() {
  getNextToken(); // eat the 'extern' keyword
  return ParsePrototype();
}

/// ifexpr ::= 'if' expression 'then' expression 'else' expression
std::unique_ptr<ExprAST> ParseIfExpr() {
  getNextToken(); // Assume CurTok is tok_if and consume it

  // <cond>
  auto Cond = ParseExpression();
  if (!Cond)
    return nullptr;

  if (getCurrentToken() != tok_then) {
    std::ostringstream errMsg("Expected 'then' keyword but found ",
                              std::ios_base::ate);
    errMsg << tokenToString(static_cast<Token>(getCurrentToken()));
    return LogError(errMsg.str().c_str());
  }
  getNextToken(); // Consume 'then'

  // <then>
  auto Then = ParseExpression();
  if (!Then)
    return nullptr;

  if (getCurrentToken() != tok_else) {
    std::ostringstream errMsg("Expected 'else' keyword but found ",
                              std::ios_base::ate);
    errMsg << tokenToString(static_cast<Token>(getCurrentToken()));
    return LogError(errMsg.str().c_str());
  }
  getNextToken(); // Consume 'else'

  // <else>
  auto Else = ParseExpression();
  if (!Else)
    return nullptr;

  return std::make_unique<IfExprAST>(std::move(Cond), std::move(Then),
                                     std::move(Else));
}

/// forexpr ::= 'for' identifier '=' expr ',' expr (',' expr)? 'in' expression
/// the (',' expr)? is for the optional step which is assumed to be 1 if not
/// included
std::unique_ptr<ExprAST> ParseForExpr() {
  // Assume the current token is the "for" keyword and consume it
  getNextToken();

  if (getCurrentToken() != tok_identifier) {
    std::ostringstream errMsg(
        "Expected identifier after 'for' keyword, but instead got:\n\t",
        std::ios_base::ate);
    errMsg << tokenToString(static_cast<Token>(getCurrentToken()));
    return LogError(errMsg.str().c_str());
  }

  std::string IdName = getIdentifierStr();
  getNextToken(); // Consume the identifier

  if (getCurrentToken() != '=') {
    std::ostringstream errMsg("Expected '=' after identifier after 'for' "
                              "keyword, but instead got:\n\t",
                              std::ios_base::ate);
    errMsg << tokenToString(static_cast<Token>(getCurrentToken()));
    return LogError(errMsg.str().c_str());
  }

  getNextToken(); // Consume '='

  auto Start = ParseExpression();
  if (!Start)
    return nullptr;

  if (getCurrentToken() != ',') {
    std::ostringstream errMsg(
        "Expected ',' after for initializer, but instead got:\n\t",
        std::ios_base::ate);
    errMsg << tokenToString(static_cast<Token>(getCurrentToken()));
    return LogError(errMsg.str().c_str());
  }

  getNextToken(); // Consume ','

  auto End = ParseExpression();
  if (!End)
    return nullptr;

  // The 'step' value is optional. Will assume 1 in the emitting of
  // LLVM IR if not provided
  std::unique_ptr<ExprAST> Step;
  if (getCurrentToken() == ',') {
    getNextToken(); // Consume ','
    Step = ParseExpression();
    if (!Step)
      return nullptr;
  }

  if (getCurrentToken() != tok_in) {
    std::ostringstream errMsg(
        "Expected 'in' keyword after 'for' statement, but instead got:\n\t",
        std::ios_base::ate);
    errMsg << tokenToString(static_cast<Token>(getCurrentToken()));
    return LogError(errMsg.str().c_str());
  }

  getNextToken(); // Consume 'in'

  auto Body = ParseExpression();
  if (!Body)
    return nullptr;

  return std::make_unique<ForExprAST>(IdName, std::move(Start), std::move(End),
                                      std::move(Step), std::move(Body));
}

/// toplevelexpr ::= expression
std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
  if (auto E = ParseExpression()) {
    // Make an anonymous function prototype.
    auto Proto = std::make_unique<PrototypeAST>("__anon_expr",
                                                std::vector<std::string>());
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
  }
  return nullptr;
}

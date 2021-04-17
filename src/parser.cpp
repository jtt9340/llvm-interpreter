#include <unordered_map>

#include "parser.h"
#include "lexer.h"
#include "logging.h"

#define loop for(;;)	// Infinite loop

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

/// numberexpr ::= number
std::unique_ptr<ExprAST> ParseNumberExpr() {
	auto Result = std::make_unique<NumberExprAST>(getNumVal());
	getNextToken(); // consume the number
	return std::move(Result);
}

/// parenexpr ::= '(' expression  ')'
std::unique_ptr<ExprAST> ParseParenExpr() {
	getNextToken(); // consume '('
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
	const std::string &IdName = getIdentifierStr();

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

		// If this is a binary operator that binds with at least as tightly as the current binary operator,
		// consume it, otherwise we are done.
		if (TokPrec < ExprPrec)
			return LHS;

		// At this point, we are indeed parsing a binary operator with a high enough precedence.
		// If it wasn't a binary operator, TokPrec would be -1 which would indeed be < Exprec,
		// so we wouldn't have gotten here by now.
		int BinOp = getCurrentToken();
		getNextToken(); // eat the binary operator

		// Parse the primary expression after the binary operator.
		auto RHS = ParsePrimary();
		if (!RHS)
			return nullptr;

		// If the current binary operator has lower precedence than the binary operator
		// after the RHS, then let the current RHS be the LHS in the next binary operator.
		int NextPrec = GetTokPrecedence();
		if (TokPrec < NextPrec) {
			RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
			if (!RHS)
				return nullptr;
		}

		// Otherwise, the current binary operator takes precedence, so let's build an AST node
		// containing the current binary operator and its operands.
		LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
	} // Loop back to the top, looking for more binary operators until there are no more expressions to parse.
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
std::unique_ptr<PrototypeAST> ParsePrototype() {
	if (getCurrentToken() != tok_identifier)
		return LogErrorP("Expected function name in prototype");

	const std::string &FnName = getIdentifierStr();
	getNextToken(); // Eat the function name

	if (getCurrentToken() != '(')
		return LogErrorP("Expected '(' in prototype");

	// Read the list of argument names.
	std::vector<std::string> ArgNames;
	while (getNextToken() == tok_identifier)
		ArgNames.push_back(getIdentifierStr());
	if (getCurrentToken() != ')')
		return LogErrorP("Expected ')' in prototype");

	// We successfully parsed a function prototype.
	getNextToken(); // Eat the ')'

	return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

/// definition ::= 'def' prototype expression
std::unique_ptr<FunctionAST> ParseDefinition() {
	getNextToken(); // eat the 'def' keyword
	auto Proto = ParsePrototype();
	if (!Proto) return nullptr;

	if (auto E = ParseExpression())
		return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
	return nullptr;
}

/// external ::= 'extern' prototype
std::unique_ptr<PrototypeAST> ParseExtern() {
	getNextToken(); // eat the 'extern' keyword
	return ParsePrototype();
}

/// toplevelexpr ::= expression
std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
	if (auto E = ParseExpression()) {
		// Make an anonymous function prototype.
		auto Proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
		return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
	}
	return nullptr;
}

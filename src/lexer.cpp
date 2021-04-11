#include <string>		// std::string
#include <cstring>		// std::strlen

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

static std::string                   IdentifierStr;   // Filled in if tok_identifier
static double                        NumVal;          // Filled in if tok_number
static std::unordered_map<char, int> BinopPrecedence; // This holds the precedence for each binary operator that is defined

/// CurTok/getNextToken - Provide a simple token buffer. CurTok is the
/// current token the parser is looking at. getNextToken reads another
/// token from the lexer and updates CurTok with its results.
static int CurTok;
int getNextToken() {
	// gettok() is forward-declared in lexer.h so we can call it here even
	// though the definition does not appear until below
	return CurTok = gettok();
}

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

/// numberexpr ::= number
static std::unique_ptr<ExprAST> ParseNumberExpr() {
	auto Result = std::make_unique<NumberExprAST>(NumVal);
	getNextToken(); // consume the number
	return std::move(Result);
}

/// parenexpr ::= '(' expression  ')'
static std::unique_ptr<ExprAST> ParseParenExpr() {
	getNextToken(); // consume '('
	auto V = ParseExpression(); // the 'expression' part in our production above
	if (!V)
		return nullptr; // we did not find an expression, just an (

	if (CurTok != ')')
		return LogError("expected ')'");
	getNextToken(); // consume ')'
	return V;
}

/// identifier
///   ::= identifier                      Variable references.
///   ::= identifier '(' expression ')'   Function calls.
static std::unique_ptr<ExprAST> ParseIdentifierExpr() {
	std::string IdName = IdentifierStr;

	getNextToken(); // Consume the identififer

	if (CurTok != '(') {
		// This is a variable reference, not a function call
		return std::make_unique<VariableExprAST>(IdName);
	}

	// Otherwise, this is a function call
	getNextToken(); // Consume the '('
	std::vector<std::unique_ptr<ExprAST>> Args;
	if (CurTok != ')') {
		loop {
			if (auto Arg = ParseExpression())
				Args.push_back(std::move(Arg));
			else
				return nullptr; // Expected an expression

			if (CurTok == ')')
				break; // We've finished parsing the function call

			if (CurTok != ',')
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
static std::unique_ptr<ExprAST> ParsePrimary() {
	switch (CurTok) {
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
static int GetTokPrecedence() {
	// If CurTok is NOT an ASCII character, assuming an ASCII or UTF-8 encoding
	if (CurTok < 0 || CurTok >= 128)
		return -1;

	int TokPrec = BinopPrecedence[CurTok];
	// Ensure the pending binary operator token is a recognized binary operator
	return TokPrec <= 0 ? -1 : TokPrec;
}

/// binoprhs
///   ::= ('+' primary)*
static std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
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
		int BinOp = CurTok;
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
static std::unique_ptr<ExprAST> ParseExpression() {
	auto LHS = ParsePrimary();
	// Attempt to parse an expression; if it is successfull (a valid token)
	// then parse a potential RHS in case it is a binary operator 
	return LHS ? ParseBinOpRHS(0, std::move(LHS)) : nullptr;
}

/// prototype
///   ::= id '(' id* ')'
static std::unique_ptr<PrototypeAST> ParsePrototype() {
	if (CurTok != tok_identifier)
		return LogErrorP("Expected function name in prototype");

	std::string FnName = IdentifierStr;
	getNextToken(); // Eat the function name

	if (CurTok != '(')
		return LogErrorP("Expected '(' in prototype");

	// Read the list of argument names.
	std::vector<std::string> ArgNames;
	while (getNextToken() == tok_identifier)
		ArgNames.push_back(IdentifierStr);
	if (CurTok != ')')
		return LogErrorP("Expected ')' in prototype");

	// We successfully parsed a function prototype.
	getNextToken(); // Eat the ')'

	return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

/// definition ::= 'def' prototype expression
static std::unique_ptr<FunctionAST> ParseDefinition() {
	getNextToken(); // eat the 'def' keyword
	auto Proto = ParsePrototype();
	if (!Proto) return nullptr;

	if (auto E = ParseExpression())
		return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
	return nullptr;
}

/// external ::= 'extern' prototype
static std::unique_ptr<PrototypeAST> ParseExtern() {
	getNextToken(); // eat the 'extern' keyword
	return ParsePrototype();
}

/// toplevelexpr ::= expression
static std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
	if (auto E = ParseExpression()) {
		// Make an anonymous function prototype.
		auto Proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
		return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
	}
	return nullptr;
}

static void HandleDefinition() {
	const auto defn = ParseDefinition();
	if (defn) {
		const auto *ir = defn->codegen();
		if (ir) {
			std::cerr << "Generate LLVM IR for function definition:" << std::endl;
			ir->print(llvm::errs());
			std::cerr << std::endl;
		}
	} else {
		// Skip token to handle errors.
		getNextToken();
	}
}

static void HandleExtern() {
	const auto externDeclaration = ParseExtern();
	if (externDeclaration) {
		const auto* ir = externDeclaration->codegen();
		if (ir) {
			std::cerr << "Generate LLVM IR for extern function declaration:" << std::endl;
			ir->print(llvm::errs());
			std::cerr << std::endl;
		}
	} else {
		// Skip token to handle errors.
		getNextToken();
	}
}

static void HandleTopLevelExpression() {
	// Evaluate a top-level expression in an anonymous function.
	const auto expr = ParseTopLevelExpr();
	if (expr) {
		const auto *ir = expr->codegen();
		if (ir) {
			std::cerr << "Generate LLVM IR for top-level expression:" << std::endl;
			ir->print(llvm::errs());
			std::cerr << std::endl;
		}
	} else {
		// Skip token to handle errors.
		getNextToken();
	}
}

/// top ::= definition | external | expression | ';'
void MainLoop(const char *ProgName) {
	loop {
		std::cerr << ProgName << "> ";
		switch (CurTok) {
		case tok_eof:
			return;
		case ';': // ignore top-level semicolons
			getNextToken(); // eat the ';'
			break;
		case tok_def:
			HandleDefinition();
			break;
		case tok_extern:
			HandleExtern();
			break;
		default:
			HandleTopLevelExpression();
		}
	}
}

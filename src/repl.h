#ifndef REPL_H
#define REPL_H

#include "lexer.h" // getCurrentToken, tok_eof, tok_def, tok_extern

#define loop for(;;)          // Infinite loop

/// What to do when a function definition is encountered at the REPL.
void HandleDefinition();

/// What to do when an extern function delcaration is encountered at the REPL.
void HandleExtern();

/// What to do when any other expression that is not a function definition or
/// extern function declaration is encountered at the REPL.
void HandleTopLevelExpression();

/// Run a REPL for interpreting expressions. This function runs an interactive
/// prompt in an infinite loop while there are still tokens to lex/parse.
///
/// top ::= definition | external | expression | ';'
/// 
/// The prompt given to the user can be contolled by the 'ProgName' parameter.
///
/// @param ProgName the name of the program do display to the user in the
///                 interactive REPL
static void MainLoop(const char *ProgName) {
	loop {
		std::cerr << ProgName << "> ";
		switch (getCurrentToken()) {
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

#endif

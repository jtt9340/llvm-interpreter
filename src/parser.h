#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

/// Update the internal binary operator precedence table with the appropriate
/// values.
///
/// This function should be called before parsing any expressions involving
/// binary operators as parsing such expressions relies on the internal
/// binary operator precedence table being properly filled in.
void SetupBinopPrecedences();

/// Add a binary operator 'Op' to the internal binary operator precedence table
/// with the value Precedence. Return the precedence the binary operator Op was
/// set to.
///
/// @param Op the binary operator to add a precedence for
/// @param Precedence the precedence to add the binary operator for
/// @return the precedence the binary operator was installed with
int InstallBinopPrecedence(const char Op, const int Precedence);

/// Transform the floating point number tokenized by gettok() into an AST node
/// that represents a numerical expression.
///
/// Calling gettok() will store the number it tokenizes into a global variable,
/// if the token it finds is a parse-able (floating-point) number. Calling this
/// function will then use that global variable to turn that into an AST node
/// for a numerical expression, so
///
///         (1) gettok() should be called before calling this function to get
///             the next numerical token in the source code
///         (2) the user should check that gettok() indeed found a tok_number,
///             (can be determined with getNextToken()) or else the numerical
///             value that this function will use to build the AST node will be
///             inaccurate.
///
/// @return an AST node wrapping a numerical value
std::unique_ptr<ExprAST> ParseNumberExpr();

/// Consume a '(' token, then another expression, and then another ')' token,
/// and build an AST node representing the value in parenthesis.
///
/// This function assumes the current token that was lexed by getNextToken() is
/// an opening parenthesis. Then, it tries to find another expression to parse,
/// returning a nullptr if there is not one. This would mean this function
/// encountered an opening parenthesis but nothing after. Finally, it looks for
/// a closing parenthesis, again returning a nullptr if it does not find one.
///
/// @return an AST node that represents the value after evaluating the
///         expression in parentheses, or a nullptr as described above
std::unique_ptr<ExprAST> ParseParenExpr();

/// Create an AST node representing either a variable reference or a function
/// call.
///
/// If this function finds a word without an opening parenthesis, it is a
/// variable reference and the AST node returned represents the name of the
/// variable. If parenthesis follow the word, then it is a function call and the
/// AST node returned represents this.
///
/// This function assumes the current token that was lexed by getNextToken() is
/// a tok_identifier, so it must only be called when getNextToken() returns
/// tok_identifier.
///
/// @return an AST node represting either a variable reference or a function
/// call
std::unique_ptr<ExprAST> ParseIdentififerExpr();

/// TODO Documentation
std::unique_ptr<ExprAST> ParseLetExpr();

/// This function will parse one of the following:
///
///         (1) A variable reference
///         (2) A function call
///         (3) A numerical expression
///         (4) A parenthetical expression
///
/// Based on the current token as returned by getNextToken(). This is a "top
/// level" AST node builder that calls one of the other Parse* functions to
/// create an AST node.
///
/// @return an AST node based on the current token lexed by getNextToken()
std::unique_ptr<ExprAST> ParsePrimary();

/// If the current token as returned by getNextToken() is a binary operator,
/// return the precedence of that operator, otherwise return -1.
///
/// This function is used for implementing the order of operations, PEMDAS, when
/// parsing expressions with binary operators to ensure that, e.g. * is
/// evaluated before +.
///
/// The higher the precedence, the higher priority that binary operator will
/// have when being evaluated.
///
/// @return the predence of the current binary operator stored in the token
/// returned by getNextToken(),
///         or -1 if the current token is not a valid binary operator
int GetTokPrecedence();

/// Parse the current token as returned by getNextToken() as a unary operator.
/// A unary operator is parsed as any character that is not an ASCII character
/// or is not an opening parenthesis or comma.
///
/// If the current token cannot be parsed as a unary operator, then just parse
/// the current token as a primary expression (see ParsePrimary).
///
/// The returned AST node represents the unary operator and the parimary
/// expression the unary operator is being applied to (the operand). If parsing
/// fails, nullptr is returned.
///
/// @return an AST node representing the unary operator and the primary
/// expression,
///         or just the primary expression if there is no unary operator, or
///         nullptr if there was an error with parsing
std::unique_ptr<ExprAST> ParseUnary();

/// Parse a sequence of primary expressions (see ParsePrimary) conjoined by
/// binary operators, after having already parsed the initial primary expression
/// in the sequence. In other words, given a sequence of primary expressions and
/// binary operators
///
///         e_1 op_1 e_2 op_2 e_3 ... op_n e_n
///
/// where e_n is the nth primary expression in the sequence and op_n is the nth
/// binary operator in the sequence, this function will parse the given
/// sequence, starting at op_1, given the AST node representing e_1 in the form
/// of the 'LHS' paremeter. This function is named ParseBinOpRHS since it parses
/// the Right-Hand Side of binary operators, stopping at the first binary
/// operator whose precednece is lower than 'ExprPrec'. Hence why the Left Hand
/// Side must have already been parsed.
///
/// An AST node (tree) is returned representing the structure of the expression
/// parsed. If a sequence ends in a binary operator with no Right-Hand Side, a
/// nullptr is returned. To parse an entire expression, starting with the first
/// primary expression and not the first binary operator, call ParseExpression.
///
/// @param ExprPrec the minimum allowed binary operator precedence to parse
/// @param LHS the expression on the left-hand side of the current binary
/// operator to parse
/// @return an AST node representing the entire expression parsed up to, but not
/// including,
///         the first binary operator with too low of a predence, or up to the
///         end of the expression if no such binary operator is encountered, or
///         nullptr is a binary operator has no Right-Hand Side
std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,
                                       std::unique_ptr<ExprAST> LHS);

/// Parse a sequence of primary expressions conjoined by binary operators. This
/// is very similar to ParseBinOpRHS, in fact it just parses the leftmost
/// primary expression, then calls ParseBinOrRHS with the remainder of the
/// expression. This function is necessary because ParseBinOpRHS expects the
/// current token to be a binary operator so that it can parse the Right-Hand
/// Side of the binary operator and compare the precedence of the current binary
/// operator to the predence of the next binary operator, so this function
/// handles parsing the left-most expression before the first binary operator.
///
/// @return an AST node (tree) represending the parsed expression
std::unique_ptr<ExprAST> ParseExpression();

/// Parse a function prototype definition, e.g.
///
///         foo(a b)
///
/// And return an AST node that encapsulates this function prototype definition.
///
/// This function expects the current token as returned by getNextToken() to be
/// an identifier, or else this function will return a nullptr. This function
/// will also return a nullprt if an opening parenthesis isn't found after the
/// initial function name, and if a closing parenthesis isn't found.
///
/// @return an AST node representing the function prototype declaration, or
/// nullptr if the current
///         token is not an identifier, or if opening and closing parentheses
///         cannot be found
std::unique_ptr<PrototypeAST> ParsePrototype();

/// Parse a complete function definition, which looks like
///
///     def func_name(param1 param2) expr
///
/// and return an AST node that encapsulates this function definition.
/// This function expects the current token as returned by getNextToken() to be
/// tok_def, and returns a nullptr if the 'func_name(param1 param2)' portion
/// or the expression ('expr') portion of a function is not present/parse-able.
///
/// @return an AST node representing a complete function definition, which
///         starts with the 'def' keyword, followed by a function prototype,
///         followed by an arbitrarily complex expression; or nullptr if a
///         function prototype or expression is not able to be parsed
std::unique_ptr<FunctionAST> ParseDefinition();

/// Parse an extern function declaration, which is the 'extern' keyword followed
/// by a function prototype definition, and return an AST node representing the
/// function prototype. This function expects the current token as returned by
/// getNextToken() to be tok_extern, then expects to parse a function prototype
/// definition as parsed by ParsePrototype.
///
/// @return an AST node representing the function prototype definition for the
///         parsed extern function declaration, or nullptr if there are no
///         opening or closing parenthesis as part of the extern function
///         declaration
std::unique_ptr<PrototypeAST> ParseExtern();

/// Parse an if expression, which is of the form
///
/// if <cond> then <then-clause> else <else-clause>;
///
/// <cond>, <then-clause>, and <else-clause> are all expressions. An AST node is
/// returned that stores the AST node representations of <cond>, <then-clause>,
/// and <else-clause>.
///
/// This function expects the current token as returned by getNextToken() to be
/// tok_if, and returns nullptr if an if expression could not be parsed
/// properly.
///
/// @return an AST node representing the parsed if expression, or nullptr if one
///         could not be parsed
std::unique_ptr<ExprAST> ParseIfExpr();

/// Parse a for loop, which is of the form
///
/// for <var-name> = <init>, <cond> (, <step>)? in <body>;
///
/// <init>, <cond>, <step>, and <body> are all expressions and <var-name> is an
/// identifier. An AST node is retured encapsulating all of the information in
/// the <> brackets: the name of the iterator variable, the initial expression,
/// the conditional expression, the optional step added to the iterator variable
/// at the end of each iteration, and the body of the for loop.
///
/// This function expects the current token as returned by getNextToken() to be
/// tok_for, and returns nullptr if any of the expressions in <> brackets cannot
/// be parsed properly.
///
/// @return an AST node representing the parsed for loop, or nullptr if any of
///         the expressions that compose a for loop could not be parsed properly
std::unique_ptr<ExprAST> ParseForExpr();

/// Parse an expression declared outside of a function. This function exists to
/// allow the user to interact with the interpreter using a REPL, and just
/// creates an AST node for an anonymous function definition using the parsed
/// expression.
///
/// @return an AST node for a complete function definition containing an
///         expression that is parsed at the top-level, i.e. outside of a
///         function
std::unique_ptr<FunctionAST> ParseTopLevelExpr();

#endif

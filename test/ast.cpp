#include <iostream>
#include <thread>

#include <cstdio>
#include <cstdlib>

#include "BinaryExprAST.h"
#include "CallExprAST.h"
#include "ForExprAST.h"
#include "FunctionAST.h"
#include "IfExprAST.h"
#include "LetExprAST.h"
#include "NumberExprAST.h"
#include "UnaryExprAST.h"
#include "VariableExprAST.h"
#include "util.h"

using std::size_t;

template <typename T0, typename T1 = T0> void assertEq(T0 &lhs, T1 &rhs) {
  if (lhs != rhs) {
    std::cerr << "Assertion failed: " << lhs << " != " << rhs << std::endl;
    std::exit(EXIT_FAILURE);
  }
}

void testShowableToString() {
  Showable s;

  constexpr size_t expectedBufSize = 50;
  char expected[expectedBufSize];
  std::snprintf(expected, expectedBufSize, "Showable@%p",
                static_cast<void *>(&s));

  const auto actual = s.toString();

  assertEq(expected, actual);
}

void testBinaryExprASTToString() {
  // Simple expression
  std::unique_ptr<NumberExprAST> lhs = std::make_unique<NumberExprAST>(5),
                                 rhs = std::make_unique<NumberExprAST>(7);

  BinaryExprAST expr('*', std::move(lhs), std::move(rhs));

  const char *expected = "NumberExprAST(5) * NumberExprAST(7)";
  auto actual = expr.toString();

  assertEq(expected, actual);

  // More complex expression
  lhs = std::make_unique<NumberExprAST>(9);
  std::vector<std::unique_ptr<ExprAST>> testArgs;
  testArgs.emplace_back(std::make_unique<NumberExprAST>(1.2));
  testArgs.emplace_back(std::make_unique<NumberExprAST>(2.5));
  testArgs.emplace_back(std::make_unique<NumberExprAST>(3.8));

  expr = BinaryExprAST(
      '/', std::move(lhs),
      std::make_unique<CallExprAST>("some_function", std::move(testArgs)));

  expected = "NumberExprAST(9) / CallExprAST(some_function(NumberExprAST(1.2), "
             "NumberExprAST(2.5), NumberExprAST(3.8)))";
  actual = expr.toString();

  assertEq(expected, actual);
}

void testCallExprASTToString() {
  std::vector<std::unique_ptr<ExprAST>> testArgs;
  testArgs.emplace_back(std::make_unique<NumberExprAST>(1));
  testArgs.emplace_back(
      std::make_unique<BinaryExprAST>('+', std::make_unique<NumberExprAST>(2),
                                      std::make_unique<NumberExprAST>(3)));
  testArgs.emplace_back(std::make_unique<NumberExprAST>(4));

  CallExprAST expr("foo", std::move(testArgs));

  const char *expected = "CallExprAST(foo(NumberExprAST(1), NumberExprAST(2) + "
                         "NumberExprAST(3), NumberExprAST(4)))";
  const auto actual = expr.toString();

  assertEq(expected, actual);
}

void testForExprASTToString() {
  const std::string inductionVariableName = "i";
  const VariableExprAST inductionVariable(inductionVariableName);
  const NumberExprAST init(0);
  const NumberExprAST one(1);
  const NumberExprAST five(5);

  ForExprAST expr(inductionVariableName, std::make_unique<NumberExprAST>(init),
                  std::make_unique<BinaryExprAST>(
                      '<', std::make_unique<VariableExprAST>(inductionVariable),
                      std::make_unique<NumberExprAST>(five)),
                  nullptr,
                  std::make_unique<BinaryExprAST>(
                      '+', std::make_unique<VariableExprAST>(inductionVariable),
                      std::make_unique<NumberExprAST>(one)));

  const char *expected = "ForExprAST(i = NumberExprAST(0), VariableExprAST(i) "
                         "< NumberExprAST(5),\n"
                         "\tVariableExprAST(i) + NumberExprAST(1)\n"
                         ")";
  auto actual = expr.toString();

  assertEq(expected, actual);

  expr =
      ForExprAST(inductionVariableName, std::make_unique<NumberExprAST>(init),
                 std::make_unique<BinaryExprAST>(
                     '<', std::make_unique<VariableExprAST>(inductionVariable),
                     std::make_unique<NumberExprAST>(five)),
                 std::make_unique<NumberExprAST>(0.5),
                 std::make_unique<BinaryExprAST>(
                     '+', std::make_unique<VariableExprAST>(inductionVariable),
                     std::make_unique<NumberExprAST>(one)));

  expected = "ForExprAST(i = NumberExprAST(0), VariableExprAST(i) < "
             "NumberExprAST(5), NumberExprAST(0.5),\n"
             "\tVariableExprAST(i) + NumberExprAST(1)\n"
             ")";
  actual = expr.toString();

  assertEq(expected, actual);
}

void testFunctionASTToString() {
  std::unique_ptr<PrototypeAST> header = std::make_unique<PrototypeAST>(
      "foo", std::vector<std::string>({"a", "b"}));
  std::unique_ptr<ExprAST> body = std::make_unique<BinaryExprAST>(
      '-',
      std::make_unique<BinaryExprAST>('+',
                                      std::make_unique<VariableExprAST>("a"),
                                      std::make_unique<VariableExprAST>("b")),
      std::make_unique<NumberExprAST>(2));

  FunctionAST func(std::move(header), std::move(body));

  const char *expected =
      "FunctionAST(\n"
      "\tPrototypeAST(foo(a, b)),\n"
      "\tVariableExprAST(a) + VariableExprAST(b) - NumberExprAST(2)\n"
      ")";
  const auto actual = func.toString();

  assertEq(expected, actual);
}

void testIfExprASTToString() {
  std::unique_ptr<ExprAST> elseIf = std::make_unique<IfExprAST>(
      std::make_unique<BinaryExprAST>('<', std::make_unique<NumberExprAST>(3),
                                      std::make_unique<NumberExprAST>(4)),
      std::make_unique<NumberExprAST>(4), std::make_unique<NumberExprAST>(5));

  IfExprAST ifExpr(
      std::make_unique<BinaryExprAST>('<', std::make_unique<NumberExprAST>(1),
                                      std::make_unique<NumberExprAST>(2)),
      std::make_unique<NumberExprAST>(3), std::move(elseIf));

  const char *expected = "IfExprAST(NumberExprAST(1) < NumberExprAST(2)\n"
                         "\t? NumberExprAST(3)\n"
                         "\t: IfExprAST(NumberExprAST(3) < NumberExprAST(4)\n"
                         "\t\t? NumberExprAST(4)\n"
                         "\t\t: NumberExprAST(5)\n"
                         "\t)\n"
                         ")";
  const auto actual = ifExpr.toString();

  assertEq(expected, actual);
}

void testLetExprASTToString() {
  std::pair<std::string, std::unique_ptr<ExprAST>> a{
      "a",
      std::make_unique<IfExprAST>(std::make_unique<BinaryExprAST>(
                                      '<', std::make_unique<NumberExprAST>(1),
                                      std::make_unique<NumberExprAST>(2)),
                                  std::make_unique<NumberExprAST>(3),
                                  std::make_unique<NumberExprAST>(4))};
  // I should really create a typedef instead of using decltype
  decltype(a) b{"b", std::make_unique<NumberExprAST>(10)};

  std::unique_ptr<ExprAST> letExprBody = std::make_unique<BinaryExprAST>(
      '*', std::make_unique<VariableExprAST>("a"),
      std::make_unique<VariableExprAST>("b"));

  std::vector<decltype(a)> variables;
  variables.emplace_back(std::move(a));
  variables.emplace_back(std::move(b));

  LetExprAST letExpr(std::move(variables), std::move(letExprBody));

  const char *expected = "LetExprAST(\n"
                         "\ta = IfExprAST(NumberExprAST(1) < NumberExprAST(2)\n"
                         "\t\t? NumberExprAST(3)\n"
                         "\t\t: NumberExprAST(4)\n"
                         "\t),\n"
                         "\tb = NumberExprAST(10);\n"
                         "\tVariableExprAST(a) * VariableExprAST(b)\n"
                         ")";
  const auto actual = letExpr.toString();

  assertEq(expected, actual);
}

void testNumberExprASTToString() {
  const double number = 0.75;
  const NumberExprAST expr(number);

  constexpr size_t expectedBufSize = 20;
  char expected[expectedBufSize];
  std::snprintf(expected, expectedBufSize, "NumberExprAST(%.2f)", number);

  const auto actual = expr.toString();

  assertEq(expected, actual);
}

void testPrototypeASTToString() {
  const std::string funcName = "foo";
  PrototypeAST proto("foo", {"a", "b", "c"});

  const char *expected = "PrototypeAST(foo(a, b, c))";
  const auto actual = proto.toString();

  assertEq(expected, actual);
}

void testUnaryExprASTToString() {
  std::unique_ptr<ExprAST> ifExpr = std::make_unique<IfExprAST>(
      std::make_unique<BinaryExprAST>('<', std::make_unique<NumberExprAST>(1),
                                      std::make_unique<NumberExprAST>(2)),
      std::make_unique<NumberExprAST>(3), std::make_unique<NumberExprAST>(4));

  UnaryExprAST expr('-', std::move(ifExpr));

  const char *expected = "-IfExprAST(NumberExprAST(1) < NumberExprAST(2)\n"
                         "\t? NumberExprAST(3)\n"
                         "\t: NumberExprAST(4)\n"
                         ")";
  const auto actual = expr.toString();

  assertEq(expected, actual);
}

void testVariableExprASTToString() {
  constexpr size_t varNameSize = 5;
  const char varName[varNameSize] = "foo";
  const VariableExprAST expr(varName);

  constexpr size_t expectedBufSize = 17 + varNameSize;
  char expected[expectedBufSize];
  std::snprintf(expected, expectedBufSize, "VariableExprAST(%s)", varName);

  const auto actual = expr.toString();

  assertEq(expected, actual);
}

int main(int argc, const char **argv) {
  constexpr void (*unitTests[])() = {
      testShowableToString,       testBinaryExprASTToString,
      testCallExprASTToString,    testForExprASTToString,
      testFunctionASTToString,    testNumberExprASTToString,
      testIfExprASTToString,      testLetExprASTToString,
      testPrototypeASTToString,   testUnaryExprASTToString,
      testVariableExprASTToString};
  constexpr size_t numUnitTests = sizeof(unitTests) / sizeof(*unitTests);
  std::array<std::thread, numUnitTests> threads;

  for (size_t i = 0; i < numUnitTests; i++)
    threads[i] = std::thread(unitTests[i]);

  // Can't use the enhanced for loop syntax here since that copies threads,
  // and the std::thread copy constructor is private
  for (auto threadIt = threads.begin(); threadIt != threads.end(); threadIt++)
    threadIt->join();

  std::printf("Passed! (%s)\n", argv[0]);
  std::exit(EXIT_SUCCESS);
}

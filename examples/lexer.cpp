#include <iostream> // std::cout, std::endl

#include "../src/lexer.h" // getNextToken, tokenToString

int main() {
  const int tok = getNextToken();
  std::cout << tokenToString(static_cast<Token>(tok)) << std::endl;
  return 0;
}

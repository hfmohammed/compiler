#include "Lexer.h"
#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "usage: gazc <input.gaz>\n";
    return 1;
  }
  std::ifstream in(argv[1]);
  std::stringstream ss;
  ss << in.rdbuf();
  try {
    auto toks = gaz::lex(ss.str());
    std::cout << toks.size() << " tokens\n";
  } catch (std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}

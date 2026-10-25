#pragma once
#include "AST.h"
#include "Lexer.h"

namespace gaz {

// Recursive-descent parser. Throws std::runtime_error("SyntaxError ...").
Program parse(const std::vector<Token> &toks);

} // namespace gaz

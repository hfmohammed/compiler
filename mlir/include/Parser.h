#pragma once
#include "AST.h"
#include <string>

namespace gaz {

// Parses Gazprea source with the ANTLR-generated lexer/parser (see
// grammar/Gazprea.g4) and builds the AST. Throws std::runtime_error
// ("SyntaxError line:col: ...") on lexical or syntax errors.
Program parse(const std::string &source);

} // namespace gaz

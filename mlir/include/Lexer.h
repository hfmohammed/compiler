#pragma once
#include <string>
#include <vector>

namespace gaz {

enum class Tok {
  Eof, Ident, IntLit, RealLit, CharLit,
  // keywords
  KwFunction, KwProcedure, KwReturns, KwReturn, KwVar, KwConst, KwIf, KwElse,
  KwLoop, KwWhile, KwBreak, KwContinue, KwCall, KwAs, KwTrue, KwFalse,
  KwNot, KwAnd, KwOr, KwXor,
  KwInteger, KwReal, KwBoolean, KwCharacter,
  KwStdInput, KwStdOutput,
  // punctuation
  LParen, RParen, LBrace, RBrace, Semi, Comma, Assign, Arrow, LArrow,
  Lt, Gt, Le, Ge, EqEq, Ne, Plus, Minus, Star, Slash, Percent, Caret,
};

struct Token {
  Tok kind = Tok::Eof;
  std::string text;
  int line = 1, col = 1;
};

// Tokenizes the whole source; throws std::runtime_error on a bad character.
std::vector<Token> lex(const std::string &src);
const char *tokName(Tok t);

} // namespace gaz

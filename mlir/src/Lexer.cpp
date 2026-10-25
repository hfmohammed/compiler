#include "Lexer.h"
#include <cctype>
#include <stdexcept>
#include <unordered_map>

namespace gaz {

static const std::unordered_map<std::string, Tok> keywords = {
    {"function", Tok::KwFunction}, {"procedure", Tok::KwProcedure},
    {"returns", Tok::KwReturns},   {"return", Tok::KwReturn},
    {"var", Tok::KwVar},           {"const", Tok::KwConst},
    {"if", Tok::KwIf},             {"else", Tok::KwElse},
    {"loop", Tok::KwLoop},         {"while", Tok::KwWhile},
    {"break", Tok::KwBreak},       {"continue", Tok::KwContinue},
    {"call", Tok::KwCall},         {"as", Tok::KwAs},
    {"true", Tok::KwTrue},         {"false", Tok::KwFalse},
    {"not", Tok::KwNot},           {"and", Tok::KwAnd},
    {"or", Tok::KwOr},             {"xor", Tok::KwXor},
    {"integer", Tok::KwInteger},   {"real", Tok::KwReal},
    {"boolean", Tok::KwBoolean},   {"character", Tok::KwCharacter},
    {"std_input", Tok::KwStdInput}, {"std_output", Tok::KwStdOutput},
};

const char *tokName(Tok t) {
  switch (t) {
  case Tok::Eof: return "end of file";
  case Tok::Ident: return "identifier";
  case Tok::IntLit: return "integer literal";
  case Tok::RealLit: return "real literal";
  case Tok::CharLit: return "character literal";
  case Tok::LParen: return "'('";
  case Tok::RParen: return "')'";
  case Tok::LBrace: return "'{'";
  case Tok::RBrace: return "'}'";
  case Tok::Semi: return "';'";
  case Tok::Comma: return "','";
  case Tok::Assign: return "'='";
  default: return "token";
  }
}

std::vector<Token> lex(const std::string &s) {
  std::vector<Token> out;
  size_t i = 0;
  int line = 1, col = 1;
  auto adv = [&](size_t n = 1) {
    while (n--) {
      if (s[i] == '\n') { line++; col = 1; } else col++;
      i++;
    }
  };
  auto fail = [&](const std::string &m) {
    throw std::runtime_error("SyntaxError " + std::to_string(line) + ":" +
                             std::to_string(col) + ": " + m);
  };
  auto push = [&](Tok k, std::string t, int l, int c) {
    out.push_back({k, std::move(t), l, c});
  };

  while (i < s.size()) {
    char c = s[i];
    if (isspace((unsigned char)c)) { adv(); continue; }
    if (c == '/' && i + 1 < s.size() && s[i + 1] == '/') {
      while (i < s.size() && s[i] != '\n') adv();
      continue;
    }
    if (c == '/' && i + 1 < s.size() && s[i + 1] == '*') {
      adv(2);
      while (i + 1 < s.size() && !(s[i] == '*' && s[i + 1] == '/')) adv();
      if (i + 1 >= s.size()) fail("unterminated block comment");
      adv(2);
      continue;
    }
    int l = line, cl = col;
    if (isalpha((unsigned char)c) || c == '_') {
      size_t st = i;
      while (i < s.size() && (isalnum((unsigned char)s[i]) || s[i] == '_')) adv();
      std::string w = s.substr(st, i - st);
      auto it = keywords.find(w);
      push(it == keywords.end() ? Tok::Ident : it->second, w, l, cl);
      continue;
    }
    if (isdigit((unsigned char)c) ||
        (c == '.' && i + 1 < s.size() && isdigit((unsigned char)s[i + 1]))) {
      size_t st = i;
      bool real = false;
      while (i < s.size() && isdigit((unsigned char)s[i])) adv();
      if (i < s.size() && s[i] == '.' && i + 1 < s.size() &&
          isdigit((unsigned char)s[i + 1])) {
        real = true;
        adv();
        while (i < s.size() && isdigit((unsigned char)s[i])) adv();
      }
      if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
        size_t j = i + 1;
        if (j < s.size() && (s[j] == '+' || s[j] == '-')) j++;
        if (j < s.size() && isdigit((unsigned char)s[j])) {
          real = true;
          adv(j - i);
          while (i < s.size() && isdigit((unsigned char)s[i])) adv();
        }
      }
      push(real ? Tok::RealLit : Tok::IntLit, s.substr(st, i - st), l, cl);
      continue;
    }
    if (c == '\'') {
      adv();
      if (i >= s.size()) fail("unterminated character literal");
      std::string v;
      if (s[i] == '\\') {
        adv();
        if (i >= s.size()) fail("bad escape");
        switch (s[i]) {
        case 'n': v = "\n"; break;
        case 't': v = "\t"; break;
        case 'r': v = "\r"; break;
        case '0': v = std::string(1, '\0'); break;
        case '\\': v = "\\"; break;
        case '\'': v = "'"; break;
        case '"': v = "\""; break;
        default: fail("unknown escape sequence");
        }
      } else v = std::string(1, s[i]);
      adv();
      if (i >= s.size() || s[i] != '\'') fail("unterminated character literal");
      adv();
      push(Tok::CharLit, v, l, cl);
      continue;
    }
    auto two = [&](char a, char b) {
      return c == a && i + 1 < s.size() && s[i + 1] == b;
    };
    if (two('-', '>')) { adv(2); push(Tok::Arrow, "->", l, cl); continue; }
    if (two('<', '-')) { adv(2); push(Tok::LArrow, "<-", l, cl); continue; }
    if (two('<', '=')) { adv(2); push(Tok::Le, "<=", l, cl); continue; }
    if (two('>', '=')) { adv(2); push(Tok::Ge, ">=", l, cl); continue; }
    if (two('=', '=')) { adv(2); push(Tok::EqEq, "==", l, cl); continue; }
    if (two('!', '=')) { adv(2); push(Tok::Ne, "!=", l, cl); continue; }
    Tok k;
    switch (c) {
    case '(': k = Tok::LParen; break;
    case ')': k = Tok::RParen; break;
    case '{': k = Tok::LBrace; break;
    case '}': k = Tok::RBrace; break;
    case ';': k = Tok::Semi; break;
    case ',': k = Tok::Comma; break;
    case '=': k = Tok::Assign; break;
    case '<': k = Tok::Lt; break;
    case '>': k = Tok::Gt; break;
    case '+': k = Tok::Plus; break;
    case '-': k = Tok::Minus; break;
    case '*': k = Tok::Star; break;
    case '/': k = Tok::Slash; break;
    case '%': k = Tok::Percent; break;
    case '^': k = Tok::Caret; break;
    default: fail(std::string("unexpected character '") + c + "'");
    }
    adv();
    push(k, std::string(1, c), l, cl);
  }
  out.push_back({Tok::Eof, "", line, col});
  return out;
}

} // namespace gaz

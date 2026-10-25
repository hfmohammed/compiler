#include "Parser.h"
#include <stdexcept>

namespace gaz {
namespace {

class Parser {
public:
  explicit Parser(const std::vector<Token> &t) : toks(t) {}

  Program program() {
    Program p;
    while (!at(Tok::Eof)) p.funcs.push_back(func());
    return p;
  }

private:
  const std::vector<Token> &toks;
  size_t pos = 0;

  const Token &cur() const { return toks[pos]; }
  bool at(Tok k) const { return cur().kind == k; }
  bool accept(Tok k) {
    if (!at(k)) return false;
    pos++;
    return true;
  }
  [[noreturn]] void fail(const std::string &m) const {
    throw std::runtime_error("SyntaxError " + std::to_string(cur().line) + ":" +
                             std::to_string(cur().col) + ": " + m);
  }
  const Token &expect(Tok k) {
    if (!at(k)) fail(std::string("expected ") + tokName(k) + ", found '" + cur().text + "'");
    return toks[pos++];
  }

  bool isTypeTok(Tok k) const {
    return k == Tok::KwInteger || k == Tok::KwReal || k == Tok::KwBoolean ||
           k == Tok::KwCharacter;
  }
  Type type() {
    switch (cur().kind) {
    case Tok::KwInteger: pos++; return Type::Integer;
    case Tok::KwReal: pos++; return Type::Real;
    case Tok::KwBoolean: pos++; return Type::Boolean;
    case Tok::KwCharacter: pos++; return Type::Character;
    default: fail("expected a type");
    }
  }

  // ---- declarations ----
  Func func() {
    Func f;
    f.line = cur().line;
    if (accept(Tok::KwFunction)) f.isProcedure = false;
    else if (accept(Tok::KwProcedure)) f.isProcedure = true;
    else fail("expected 'function' or 'procedure'");
    f.name = expect(Tok::Ident).text;
    expect(Tok::LParen);
    if (!at(Tok::RParen)) {
      do {
        accept(Tok::KwConst);
        Param p;
        p.type = type();
        p.name = expect(Tok::Ident).text;
        f.params.push_back(p);
      } while (accept(Tok::Comma));
    }
    expect(Tok::RParen);
    if (accept(Tok::KwReturns)) f.ret = type();
    if (accept(Tok::Assign)) {
      f.exprBody = expr();
      expect(Tok::Semi);
    } else {
      expect(Tok::LBrace);
      while (!at(Tok::RBrace)) f.body.push_back(stmt());
      expect(Tok::RBrace);
    }
    return f;
  }

  // ---- statements ----
  StmtP mk(Stmt::Kind k) {
    auto s = std::make_unique<Stmt>();
    s->kind = k;
    s->line = cur().line;
    return s;
  }

  // Body of an if/loop: a block, or a single statement.
  std::vector<StmtP> body() {
    std::vector<StmtP> v;
    if (accept(Tok::LBrace)) {
      while (!at(Tok::RBrace)) v.push_back(stmt());
      expect(Tok::RBrace);
    } else {
      v.push_back(stmt());
    }
    return v;
  }

  StmtP stmt() {
    auto k = cur().kind;
    if (k == Tok::LBrace) {
      auto s = mk(Stmt::Block);
      s->body = body();
      return s;
    }
    if (k == Tok::KwConst || k == Tok::KwVar || isTypeTok(k)) {
      auto s = mk(Stmt::Decl);
      if (accept(Tok::KwConst)) s->isConst = true;
      else accept(Tok::KwVar);
      if (isTypeTok(cur().kind)) s->declType = type();
      else s->hasType = false;
      s->name = expect(Tok::Ident).text;
      if (accept(Tok::Assign)) s->expr = expr();
      else if (!s->hasType) fail("cannot infer a type without an initializer");
      expect(Tok::Semi);
      return s;
    }
    if (k == Tok::KwIf) {
      auto s = mk(Stmt::If);
      pos++;
      expect(Tok::LParen);
      s->expr = expr();
      expect(Tok::RParen);
      s->body = body();
      if (accept(Tok::KwElse)) s->elseBody = body();
      return s;
    }
    if (k == Tok::KwLoop) {
      auto s = mk(Stmt::Loop);
      pos++;
      if (accept(Tok::KwWhile)) {
        s->loopKind = Stmt::PreTest;
        expect(Tok::LParen);
        s->expr = expr();
        expect(Tok::RParen);
        s->body = body();
      } else {
        s->body = body();
        if (accept(Tok::KwWhile)) {
          s->loopKind = Stmt::PostTest;
          expect(Tok::LParen);
          s->expr = expr();
          expect(Tok::RParen);
          expect(Tok::Semi);
        }
      }
      return s;
    }
    if (k == Tok::KwBreak || k == Tok::KwContinue) {
      auto s = mk(k == Tok::KwBreak ? Stmt::Break : Stmt::Continue);
      pos++;
      expect(Tok::Semi);
      return s;
    }
    if (k == Tok::KwReturn) {
      auto s = mk(Stmt::Return);
      pos++;
      if (!at(Tok::Semi)) s->expr = expr();
      expect(Tok::Semi);
      return s;
    }
    if (k == Tok::KwCall) {
      auto s = mk(Stmt::CallS);
      pos++;
      s->name = expect(Tok::Ident).text;
      expect(Tok::LParen);
      if (!at(Tok::RParen)) {
        do s->args.push_back(expr()); while (accept(Tok::Comma));
      }
      expect(Tok::RParen);
      expect(Tok::Semi);
      return s;
    }
    // identifier '=' expr ';'   |   identifier '<-' std_input ';'
    if (k == Tok::Ident && toks[pos + 1].kind == Tok::Assign) {
      auto s = mk(Stmt::Assign);
      s->name = cur().text;
      pos += 2;
      s->expr = expr();
      expect(Tok::Semi);
      return s;
    }
    if (k == Tok::Ident && toks[pos + 1].kind == Tok::LArrow) {
      auto s = mk(Stmt::Input);
      s->name = cur().text;
      pos += 2;
      expect(Tok::KwStdInput);
      expect(Tok::Semi);
      return s;
    }
    // expr '->' std_output ';'
    auto s = mk(Stmt::Output);
    s->expr = expr();
    expect(Tok::Arrow);
    expect(Tok::KwStdOutput);
    expect(Tok::Semi);
    return s;
  }

  // ---- expressions (precedence climbing) ----
  ExprP node(Expr::Kind k, std::string text = "") {
    auto e = std::make_unique<Expr>();
    e->kind = k;
    e->line = cur().line;
    e->text = std::move(text);
    return e;
  }
  ExprP bin(const std::string &op, ExprP l, ExprP r) {
    auto e = node(Expr::Binary, op);
    e->args.push_back(std::move(l));
    e->args.push_back(std::move(r));
    return e;
  }

  ExprP expr() { return orExpr(); }

  ExprP orExpr() {
    auto l = andExpr();
    while (at(Tok::KwOr) || at(Tok::KwXor)) {
      std::string op = cur().text;
      pos++;
      l = bin(op, std::move(l), andExpr());
    }
    return l;
  }
  ExprP andExpr() {
    auto l = eqExpr();
    while (accept(Tok::KwAnd)) l = bin("and", std::move(l), eqExpr());
    return l;
  }
  ExprP eqExpr() {
    auto l = relExpr();
    while (at(Tok::EqEq) || at(Tok::Ne)) {
      std::string op = cur().text;
      pos++;
      l = bin(op, std::move(l), relExpr());
    }
    return l;
  }
  ExprP relExpr() {
    auto l = addExpr();
    while (at(Tok::Lt) || at(Tok::Gt) || at(Tok::Le) || at(Tok::Ge)) {
      std::string op = cur().text;
      pos++;
      l = bin(op, std::move(l), addExpr());
    }
    return l;
  }
  ExprP addExpr() {
    auto l = mulExpr();
    while (at(Tok::Plus) || at(Tok::Minus)) {
      std::string op = cur().text;
      pos++;
      l = bin(op, std::move(l), mulExpr());
    }
    return l;
  }
  ExprP mulExpr() {
    auto l = unaryExpr();
    while (at(Tok::Star) || at(Tok::Slash) || at(Tok::Percent)) {
      std::string op = cur().text;
      pos++;
      l = bin(op, std::move(l), unaryExpr());
    }
    return l;
  }
  // `^` binds tighter than unary operators in Gazprea.
  ExprP unaryExpr() {
    if (at(Tok::Minus) || at(Tok::Plus) || at(Tok::KwNot)) {
      auto e = node(Expr::Unary, cur().text);
      pos++;
      e->args.push_back(unaryExpr());
      return e;
    }
    return powExpr();
  }
  ExprP powExpr() {
    auto l = primary();
    if (accept(Tok::Caret)) return bin("^", std::move(l), powExpr());
    return l;
  }

  ExprP primary() {
    switch (cur().kind) {
    case Tok::IntLit: { auto e = node(Expr::IntLit, cur().text); pos++; return e; }
    case Tok::RealLit: { auto e = node(Expr::RealLit, cur().text); pos++; return e; }
    case Tok::CharLit: { auto e = node(Expr::CharLit, cur().text); pos++; return e; }
    case Tok::KwTrue: { auto e = node(Expr::BoolLit, "true"); pos++; return e; }
    case Tok::KwFalse: { auto e = node(Expr::BoolLit, "false"); pos++; return e; }
    case Tok::LParen: {
      pos++;
      auto e = expr();
      expect(Tok::RParen);
      return e;
    }
    case Tok::KwAs: {
      auto e = node(Expr::Cast);
      pos++;
      expect(Tok::Lt);
      e->castTo = type();
      expect(Tok::Gt);
      expect(Tok::LParen);
      e->args.push_back(expr());
      expect(Tok::RParen);
      return e;
    }
    case Tok::Ident: {
      std::string name = cur().text;
      auto e = node(Expr::Var, name);
      pos++;
      if (accept(Tok::LParen)) {
        e->kind = Expr::Call;
        if (!at(Tok::RParen)) {
          do e->args.push_back(expr()); while (accept(Tok::Comma));
        }
        expect(Tok::RParen);
      }
      return e;
    }
    default:
      fail("unexpected '" + cur().text + "' in expression");
    }
  }
};

} // namespace

Program parse(const std::vector<Token> &toks) { return Parser(toks).program(); }

} // namespace gaz

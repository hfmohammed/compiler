#include "GazpreaLexer.h"
#include "GazpreaParser.h"
#include "Parser.h"
#include <stdexcept>

namespace gaz {
namespace {

using P = GazpreaParser;

class ThrowingListener : public antlr4::BaseErrorListener {
public:
  void syntaxError(antlr4::Recognizer *, antlr4::Token *, size_t line,
                   size_t col, const std::string &msg,
                   std::exception_ptr) override {
    throw std::runtime_error("SyntaxError " + std::to_string(line) + ":" +
                             std::to_string(col) + ": " + msg);
  }
};

class Builder {
public:
  Program program(P::ProgramContext *ctx) {
    Program p;
    for (auto *f : ctx->func()) p.funcs.push_back(func(f));
    return p;
  }

private:
  static int lineOf(antlr4::ParserRuleContext *c) {
    return (int)c->getStart()->getLine();
  }

  Type type(P::TypeContext *t) {
    switch (t->getStart()->getType()) {
    case P::KW_INTEGER: return Type::Integer;
    case P::KW_REAL: return Type::Real;
    case P::KW_BOOLEAN: return Type::Boolean;
    default: return Type::Character;
    }
  }

  Func func(P::FuncContext *c) {
    Func f;
    f.line = lineOf(c);
    f.isProcedure = c->kind->getType() == P::PROCEDURE;
    f.name = c->name->getText();
    for (auto *p : c->param()) f.params.push_back({type(p->type()), p->ID()->getText()});
    if (c->type()) f.ret = type(c->type());
    if (c->expr()) f.exprBody = expr(c->expr());
    else f.body = stmts(c->stmt());
    return f;
  }

  std::vector<StmtP> stmts(const std::vector<P::StmtContext *> &v) {
    std::vector<StmtP> out;
    for (auto *s : v) out.push_back(stmt(s));
    return out;
  }

  // The body of an if/loop: a braced block is flattened, anything else is a
  // single statement.
  std::vector<StmtP> body(P::StmtContext *s) {
    if (auto *b = dynamic_cast<P::BlockStmtContext *>(s)) return stmts(b->stmt());
    std::vector<StmtP> v;
    v.push_back(stmt(s));
    return v;
  }

  StmtP mk(Stmt::Kind k, antlr4::ParserRuleContext *c) {
    auto s = std::make_unique<Stmt>();
    s->kind = k;
    s->line = lineOf(c);
    return s;
  }

  StmtP decl(antlr4::ParserRuleContext *c, P::TypeContext *t, antlr4::tree::TerminalNode *id,
             P::ExprContext *init, bool isConst) {
    auto s = mk(Stmt::Decl, c);
    s->isConst = isConst;
    if (t) s->declType = type(t);
    else s->hasType = false;
    s->name = id->getText();
    if (init) s->expr = expr(init);
    else if (!t)
      throw std::runtime_error("SyntaxError " + std::to_string(s->line) +
                               ": cannot infer a type without an initializer");
    return s;
  }

  StmtP stmt(P::StmtContext *c) {
    if (auto *x = dynamic_cast<P::BlockStmtContext *>(c)) {
      auto s = mk(Stmt::Block, c);
      s->body = stmts(x->stmt());
      return s;
    }
    if (auto *x = dynamic_cast<P::QualDeclContext *>(c))
      return decl(c, x->type(), x->ID(), x->expr(), x->qual->getType() == P::CONST);
    if (auto *x = dynamic_cast<P::TypedDeclContext *>(c))
      return decl(c, x->type(), x->ID(), x->expr(), false);
    if (auto *x = dynamic_cast<P::IfStmtContext *>(c)) {
      auto s = mk(Stmt::If, c);
      s->expr = expr(x->expr());
      s->body = body(x->thenS);
      if (x->elseS) s->elseBody = body(x->elseS);
      return s;
    }
    if (auto *x = dynamic_cast<P::WhileLoopContext *>(c)) {
      auto s = mk(Stmt::Loop, c);
      s->loopKind = Stmt::PreTest;
      s->expr = expr(x->expr());
      s->body = body(x->stmt());
      return s;
    }
    if (auto *x = dynamic_cast<P::LoopContext *>(c)) {
      auto s = mk(Stmt::Loop, c);
      s->body = body(x->stmt());
      if (x->expr()) {
        s->loopKind = Stmt::PostTest;
        s->expr = expr(x->expr());
      }
      return s;
    }
    if (dynamic_cast<P::BreakStmtContext *>(c)) return mk(Stmt::Break, c);
    if (dynamic_cast<P::ContinueStmtContext *>(c)) return mk(Stmt::Continue, c);
    if (auto *x = dynamic_cast<P::ReturnStmtContext *>(c)) {
      auto s = mk(Stmt::Return, c);
      if (x->expr()) s->expr = expr(x->expr());
      return s;
    }
    if (auto *x = dynamic_cast<P::CallStmtContext *>(c)) {
      auto s = mk(Stmt::CallS, c);
      s->name = x->ID()->getText();
      for (auto *e : x->expr()) s->args.push_back(expr(e));
      return s;
    }
    if (auto *x = dynamic_cast<P::AssignStmtContext *>(c)) {
      auto s = mk(Stmt::Assign, c);
      s->name = x->ID()->getText();
      s->expr = expr(x->expr());
      return s;
    }
    if (auto *x = dynamic_cast<P::InputStmtContext *>(c)) {
      auto s = mk(Stmt::Input, c);
      s->name = x->ID()->getText();
      return s;
    }
    auto *x = dynamic_cast<P::OutputStmtContext *>(c);
    auto s = mk(Stmt::Output, c);
    s->expr = expr(x->expr());
    return s;
  }

  ExprP node(Expr::Kind k, antlr4::ParserRuleContext *c, std::string text = "") {
    auto e = std::make_unique<Expr>();
    e->kind = k;
    e->line = lineOf(c);
    e->text = std::move(text);
    return e;
  }
  ExprP binary(antlr4::ParserRuleContext *c, const std::string &op,
               std::vector<P::ExprContext *> operands) {
    auto e = node(Expr::Binary, c, op);
    for (auto *o : operands) e->args.push_back(expr(o));
    return e;
  }

  static std::string decodeChar(const std::string &lit) {
    std::string body = lit.substr(1, lit.size() - 2);
    if (body[0] != '\\') return body;
    switch (body[1]) {
    case 'n': return "\n";
    case 't': return "\t";
    case 'r': return "\r";
    case '0': return std::string(1, '\0');
    default: return std::string(1, body[1]); // \\ \' \"
    }
  }

  ExprP expr(P::ExprContext *c) {
    if (auto *x = dynamic_cast<P::ParenExprContext *>(c)) return expr(x->expr());
    if (auto *x = dynamic_cast<P::CallExprContext *>(c)) {
      auto e = node(Expr::Call, c, x->ID()->getText());
      for (auto *a : x->expr()) e->args.push_back(expr(a));
      return e;
    }
    if (auto *x = dynamic_cast<P::CastExprContext *>(c)) {
      auto e = node(Expr::Cast, c);
      e->castTo = type(x->type());
      e->args.push_back(expr(x->expr()));
      return e;
    }
    if (auto *x = dynamic_cast<P::PowExprContext *>(c)) return binary(c, "^", x->expr());
    if (auto *x = dynamic_cast<P::UnaryExprContext *>(c)) {
      auto e = node(Expr::Unary, c, x->op->getText());
      e->args.push_back(expr(x->expr()));
      return e;
    }
    if (auto *x = dynamic_cast<P::MulExprContext *>(c)) return binary(c, x->op->getText(), x->expr());
    if (auto *x = dynamic_cast<P::AddExprContext *>(c)) return binary(c, x->op->getText(), x->expr());
    if (auto *x = dynamic_cast<P::RelExprContext *>(c)) return binary(c, x->op->getText(), x->expr());
    if (auto *x = dynamic_cast<P::EqExprContext *>(c)) return binary(c, x->op->getText(), x->expr());
    if (auto *x = dynamic_cast<P::AndExprContext *>(c)) return binary(c, "and", x->expr());
    if (auto *x = dynamic_cast<P::OrExprContext *>(c)) return binary(c, x->op->getText(), x->expr());
    if (dynamic_cast<P::IntLitContext *>(c)) return node(Expr::IntLit, c, c->getText());
    if (dynamic_cast<P::RealLitContext *>(c)) return node(Expr::RealLit, c, c->getText());
    if (dynamic_cast<P::CharLitContext *>(c)) return node(Expr::CharLit, c, decodeChar(c->getText()));
    if (dynamic_cast<P::BoolLitContext *>(c)) return node(Expr::BoolLit, c, c->getText());
    return node(Expr::Var, c, c->getText());
  }
};

} // namespace

Program parse(const std::string &source) {
  ThrowingListener listener;
  antlr4::ANTLRInputStream input(source);
  GazpreaLexer lexer(&input);
  lexer.removeErrorListeners();
  lexer.addErrorListener(&listener);
  antlr4::CommonTokenStream tokens(&lexer);
  GazpreaParser parser(&tokens);
  parser.removeErrorListeners();
  parser.addErrorListener(&listener);
  auto *tree = parser.program();
  return Builder().program(tree);
}

} // namespace gaz

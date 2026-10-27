#include "GazpreaLexer.h"
#include "GazpreaParser.h"
#include "Parser.h"
#include <cctype>
#include <stdexcept>
#include <unordered_map>

namespace gaz {
namespace {

using P = GazpreaParser;

class ThrowingListener : public antlr4::BaseErrorListener {
public:
  void syntaxError(antlr4::Recognizer *, antlr4::Token *, size_t line,
                   size_t col, const std::string &msg,
                   std::exception_ptr) override {
    throw std::runtime_error("SyntaxError on line " + std::to_string(line) +
                             ": " + msg);
  }
};

class Builder {
public:
  Program program(P::ProgramContext *ctx) {
    Program p;
    aliases.emplace_back();
    for (auto *item : ctx->topLevel()) {
      if (item->typeAliasDecl()) defineAlias(item->typeAliasDecl());
      else p.funcs.push_back(func(item->func()));
    }
    return p;
  }

private:
  std::vector<std::unordered_map<std::string, Type>> aliases;

  static int lineOf(antlr4::ParserRuleContext *c) {
    return (int)c->getStart()->getLine();
  }

  Type type(P::TypeContext *t) {
    switch (t->getStart()->getType()) {
    case P::KW_INTEGER: return Type::Integer;
    case P::KW_REAL: return Type::Real;
    case P::KW_BOOLEAN: return Type::Boolean;
    case P::KW_TUPLE: {
      std::vector<Type> elems;
      for (auto *e : t->type()) elems.push_back(type(e));
      if (elems.size() < 2)
        throw std::runtime_error("TypeError on line " + std::to_string(lineOf(t)) +
                                 ": a tuple needs at least two members");
      return Type::tuple(std::move(elems));
    }
    case P::KW_CHARACTER: return Type::Character;
    default: {
      const std::string name = t->ID()->getText();
      for (auto it = aliases.rbegin(); it != aliases.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return found->second;
      }
      throw std::runtime_error("SymbolError on line " + std::to_string(lineOf(t)) +
                               ": undefined type alias '" + name + "'");
    }
    }
  }

  void defineAlias(P::TypeAliasDeclContext *c) {
    const std::string name = c->ID()->getText();
    Type target = type(c->type());
    auto &scope = aliases.back();
    if (scope.count(name))
      throw std::runtime_error("SymbolError on line " + std::to_string(lineOf(c)) +
                               ": type alias '" + name + "' already declared in this scope");
    scope[name] = std::move(target);
  }

  Func func(P::FuncContext *c) {
    Func f;
    f.line = lineOf(c);
    f.isProcedure = c->kind->getType() == P::PROCEDURE;
    f.name = c->name->getText();
    if (c->expr() && f.isProcedure)
      throw std::runtime_error("SyntaxError on line " + std::to_string(f.line) +
                               ": procedures cannot have an expression body");
    for (auto *p : c->param()) {
      if (!f.isProcedure && p->qual)
        throw std::runtime_error("SyntaxError on line " + std::to_string(f.line) +
                                 ": function parameters cannot have qualifiers");
      Param prm;
      prm.type = type(p->type());
      prm.isVar = p->qual && p->qual->getType() == P::VAR;
      if (p->ID()) prm.name = p->ID()->getText();
      f.params.push_back(prm);
    }
    f.isProto = !c->expr() && c->stmt().empty() && c->getText().back() == ';';
    if (c->type()) f.ret = type(c->type());
    if (c->expr()) f.exprBody = expr(c->expr());
    else f.body = stmts(c->stmt());
    return f;
  }

  std::vector<StmtP> stmts(const std::vector<P::StmtContext *> &v) {
    aliases.emplace_back();
    std::vector<StmtP> out;
    for (auto *s : v) out.push_back(stmt(s));
    aliases.pop_back();
    return out;
  }

  // The body of an if/loop: a braced block is flattened, anything else is a
  // single statement.
  std::vector<StmtP> body(P::StmtContext *s) {
    if (auto *b = dynamic_cast<P::BlockStmtContext *>(s)) return stmts(b->stmt());
    std::vector<StmtP> v;
    aliases.emplace_back();
    v.push_back(stmt(s));
    aliases.pop_back();
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
      throw std::runtime_error("SyntaxError on line " + std::to_string(s->line) +
                               ": cannot infer a type without an initializer");
    return s;
  }

  StmtP stmt(P::StmtContext *c) {
    if (auto *x = dynamic_cast<P::BlockStmtContext *>(c)) {
      auto s = mk(Stmt::Block, c);
      s->body = stmts(x->stmt());
      return s;
    }
    if (auto *x = dynamic_cast<P::TypeAliasStmtContext *>(c)) {
      defineAlias(x->typeAliasDecl());
      return mk(Stmt::TypeAlias, c);
    }
    if (auto *x = dynamic_cast<P::QualDeclContext *>(c))
      return decl(c, x->type(), x->ID(), x->expr(), x->qual->getType() == P::CONST);
    if (auto *x = dynamic_cast<P::TypedDeclContext *>(c))
      return decl(c, x->type(), x->ID(), x->expr(), true); // no qualifier means const
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
    if (auto *x = dynamic_cast<P::RangeLoopContext *>(c)) {
      auto s = mk(Stmt::Loop, c);
      s->loopKind = Stmt::Range;
      s->name = x->ID()->getText();
      s->expr = expr(x->expr(0));
      s->hi = expr(x->expr(1));
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
    if (auto *x = dynamic_cast<P::TupleAssignStmtContext *>(c)) {
      auto s = mk(Stmt::Assign, c);
      s->name = x->ID()->getText();
      s->idx = fieldIndex(x->INT_LIT(), s->line);
      s->expr = expr(x->expr());
      return s;
    }
    if (auto *x = dynamic_cast<P::UnpackStmtContext *>(c)) {
      auto s = mk(Stmt::Unpack, c);
      for (auto *id : x->ID()) s->names.push_back(id->getText());
      s->expr = expr(x->expr());
      return s;
    }
    if (auto *x = dynamic_cast<P::InputStmtContext *>(c)) {
      auto s = mk(Stmt::Input, c);
      s->name = x->ID()->getText();
      if (x->INT_LIT()) s->idx = fieldIndex(x->INT_LIT(), s->line);
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

  // Decodes the escapes of a character or string literal body.
  static std::string decode(const std::string &body, int line) {
    auto bad = [&](const std::string &m) {
      return std::runtime_error("LiteralError on line " + std::to_string(line) + ": " + m);
    };
    std::string out;
    for (size_t i = 0; i < body.size(); i++) {
      if (body[i] != '\\') { out += body[i]; continue; }
      char c = body[++i];
      switch (c) {
      case '0': out += '\0'; break;
      case 'a': out += '\a'; break;
      case 'b': out += '\b'; break;
      case 't': out += '\t'; break;
      case 'n': out += '\n'; break;
      case 'r': out += '\r'; break;
      case '"': case '\'': case '\\': out += c; break;
      case 'x': {
        if (i + 2 >= body.size() || !isxdigit((unsigned char)body[i + 1]) ||
            !isxdigit((unsigned char)body[i + 2]))
          throw bad("malformed \\x escape");
        out += (char)std::stoi(body.substr(i + 1, 2), nullptr, 16);
        i += 2;
        break;
      }
      default: throw bad(std::string("unknown escape sequence '\\") + c + "'");
      }
    }
    return out;
  }
  static std::string decodeChar(const std::string &lit, int line) {
    std::string v = decode(lit.substr(1, lit.size() - 2), line);
    if (v.size() != 1)
      throw std::runtime_error("LiteralError on line " + std::to_string(line) +
                               ": character literal must hold one character");
    return v;
  }

  static int fieldIndex(antlr4::tree::TerminalNode *lit, int line) {
    int v = 0;
    try {
      v = std::stoi(lit->getText());
    } catch (std::out_of_range &) {
      v = 0;
    }
    if (v < 1) // 0 and anything that does not fit are never valid field indices
      throw std::runtime_error("IndexError on line " + std::to_string(line) +
                               ": tuple field indices start at 1");
    return v;
  }

  ExprP expr(P::ExprContext *c) {
    if (auto *x = dynamic_cast<P::ParenExprContext *>(c)) return expr(x->expr());
    if (auto *x = dynamic_cast<P::TupleLitContext *>(c)) {
      auto e = node(Expr::TupleLit, c);
      for (auto *a : x->expr()) e->args.push_back(expr(a));
      return e;
    }
    if (auto *x = dynamic_cast<P::TupleAccessContext *>(c)) {
      auto e = node(Expr::TupleAccess, c, x->ID()->getText());
      e->idx = fieldIndex(x->INT_LIT(), e->line);
      return e;
    }
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
    if (dynamic_cast<P::CharLitContext *>(c))
      return node(Expr::CharLit, c, decodeChar(c->getText(), lineOf(c)));
    if (dynamic_cast<P::StringLitContext *>(c)) {
      std::string t = c->getText();
      return node(Expr::StrLit, c, decode(t.substr(1, t.size() - 2), lineOf(c)));
    }
    if (dynamic_cast<P::StreamStateExprContext *>(c)) return node(Expr::StreamState, c);
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

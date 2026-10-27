#pragma once
#include <memory>
#include <string>
#include <vector>

namespace gaz {

// A Gazprea type: a scalar, void, or a tuple of types (nesting allowed).
struct Type {
  enum Kind { Integer, Real, Boolean, Character, Void, Tuple };
  Kind kind = Void;
  std::vector<Type> elems; // tuple member types

  Type() = default;
  Type(Kind k) : kind(k) {}
  static Type tuple(std::vector<Type> e) {
    Type t(Tuple);
    t.elems = std::move(e);
    return t;
  }
  bool isTuple() const { return kind == Tuple; }
  bool isNumeric() const { return kind == Integer || kind == Real; }
  friend bool operator==(const Type &a, const Type &b) {
    return a.kind == b.kind && a.elems == b.elems;
  }
  friend bool operator!=(const Type &a, const Type &b) { return !(a == b); }
};

struct Expr;
struct Stmt;
using ExprP = std::unique_ptr<Expr>;
using StmtP = std::unique_ptr<Stmt>;

struct Expr {
  enum Kind { IntLit, RealLit, BoolLit, CharLit, StrLit, StreamState, Var, Unary,
              Binary, Call, Cast, TupleLit, TupleAccess } kind;
  int line = 0;
  // literals / identifiers / operators (as spelled in source)
  std::string text;
  Type castTo = Type::Integer;
  int idx = 0; // TupleAccess: 1-based field index (text holds the variable)
  std::vector<ExprP> args; // operands or call arguments
  Type type = Type::Void;  // filled in during code generation
};

struct Stmt {
  enum Kind { Decl, Assign, Unpack, Output, Input, If, Loop, Break, Continue,
              Return, CallS, Block, TypeAlias } kind;
  int line = 0;
  std::string name;       // Decl / Assign / Input target, CallS callee
  int idx = 0;            // Assign / Input: 1-based tuple field (0 = whole variable)
  std::vector<std::string> names; // Unpack targets
  Type declType = Type::Integer;
  bool isConst = false;
  bool hasType = true;    // `var x = 1;` infers the type
  ExprP expr;             // init / rhs / condition / return value / output value
  ExprP hi;               // Range loop: upper bound (expr is the lower bound)
  std::vector<ExprP> args; // CallS arguments
  std::vector<StmtP> body; // Block statements, or loop/if "then" body
  std::vector<StmtP> elseBody;
  // Loop shape: infinite, `while (c)`, or post-test `... while (c);`
  enum LoopKind { Forever, PreTest, PostTest, Range } loopKind = Forever;
};

struct Param {
  Type type;
  std::string name;
  bool isVar = false; // `var` parameters are passed by reference
};

struct Func {
  std::string name;
  bool isProcedure = false;
  std::vector<Param> params;
  Type ret = Type::Void;
  int line = 0;
  bool isProto = false;    // forward declaration without a body
  ExprP exprBody;          // `function f() returns t = expr;`
  std::vector<StmtP> body; // block body
};

struct Program {
  std::vector<StmtP> globals;
  std::vector<Func> funcs;
};

} // namespace gaz

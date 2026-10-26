#pragma once
#include <memory>
#include <string>
#include <vector>

namespace gaz {

enum class Type { Integer, Real, Boolean, Character, Void };

struct Expr;
struct Stmt;
using ExprP = std::unique_ptr<Expr>;
using StmtP = std::unique_ptr<Stmt>;

struct Expr {
  enum Kind { IntLit, RealLit, BoolLit, CharLit, StrLit, StreamState, Var, Unary,
              Binary, Call, Cast } kind;
  int line = 0;
  // literals / identifiers / operators (as spelled in source)
  std::string text;
  Type castTo = Type::Integer;
  std::vector<ExprP> args; // operands or call arguments
  Type type = Type::Void;  // filled in during code generation
};

struct Stmt {
  enum Kind { Decl, Assign, Output, Input, If, Loop, Break, Continue, Return,
              CallS, Block } kind;
  int line = 0;
  std::string name;       // Decl / Assign / Input target, CallS callee
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
};

struct Func {
  std::string name;
  bool isProcedure = false;
  std::vector<Param> params;
  Type ret = Type::Void;
  int line = 0;
  ExprP exprBody;          // `function f() returns t = expr;`
  std::vector<StmtP> body; // block body
};

struct Program {
  std::vector<Func> funcs;
};

} // namespace gaz

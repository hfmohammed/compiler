#include "MLIRGen.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include <stdexcept>
#include <unordered_map>

namespace gaz {
namespace {

using mlir::Block;
using mlir::Location;
using mlir::Value;

struct Var {
  Value mem; // rank-0 memref holding the value
  Type type;
  bool isConst;
};

struct FuncInfo {
  std::vector<Type> params;
  Type ret;
  bool isProc = false;
};

class Gen {
public:
  Gen(mlir::MLIRContext &c) : ctx(c), b(&c) {}

  mlir::OwningOpRef<mlir::ModuleOp> run(const Program &prog) {
    module = mlir::ModuleOp::create(b.getUnknownLoc());
    b.setInsertionPointToEnd(module->getBody());
    declareRuntime();

    for (auto &f : prog.funcs) {
      if (funcs.count(f.name))
        throw err("SymbolError", f.line, "redefinition of '" + f.name + "'");
      FuncInfo fi{{}, f.ret, f.isProcedure};
      for (auto &p : f.params) fi.params.push_back(p.type);
      funcs[f.name] = fi;
    }
    auto it = funcs.find("main");
    if (it == funcs.end())
      throw err("MainError", 1, "program does not have a main procedure");
    if (!it->second.isProc || !it->second.params.empty() ||
        it->second.ret != Type::Integer)
      throw err("MainError", 1,
                "main must be a procedure with no arguments returning integer");

    for (auto &f : prog.funcs) genFunc(f);
    return std::move(module);
  }

private:
  mlir::MLIRContext &ctx;
  mlir::OpBuilder b;
  mlir::OwningOpRef<mlir::ModuleOp> module;
  std::unordered_map<std::string, FuncInfo> funcs;
  std::vector<std::unordered_map<std::string, Var>> scopes;
  mlir::func::FuncOp curFn;
  Block *entry = nullptr;
  Type curRet = Type::Void;
  bool curIsProc = true;
  Block *trapBlock = nullptr; // shared MathError (overflow) exit of the current function

  std::runtime_error err(const char *kind, int line, const std::string &m) {
    return std::runtime_error(std::string(kind) + " on line " +
                              std::to_string(line) + ": " + m);
  }

  // ---- types ----
  mlir::Type mlirType(Type t) {
    switch (t) {
    case Type::Integer: return b.getI32Type();
    case Type::Real: return b.getF32Type();
    case Type::Boolean: return b.getI1Type();
    case Type::Character: return b.getI8Type();
    default: return mlir::Type();
    }
  }
  static const char *typeName(Type t) {
    switch (t) {
    case Type::Integer: return "integer";
    case Type::Real: return "real";
    case Type::Boolean: return "boolean";
    case Type::Character: return "character";
    default: return "void";
    }
  }

  // ---- runtime declarations ----
  void declareExt(const char *name, std::vector<mlir::Type> in,
                  std::vector<mlir::Type> out) {
    auto fn = mlir::func::FuncOp::create(b, b.getUnknownLoc(), name,
                                         b.getFunctionType(in, out));
    fn.setPrivate();
  }
  void declareRuntime() {
    mlir::Type i32 = b.getI32Type(), f32 = b.getF32Type(), i8 = b.getI8Type();
    declareExt("gaz_print_int", {i32}, {});
    declareExt("gaz_print_real", {f32}, {});
    declareExt("gaz_print_bool", {i32}, {});
    declareExt("gaz_print_char", {i8}, {});
    declareExt("gaz_read_int", {}, {i32});
    declareExt("gaz_read_real", {}, {f32});
    declareExt("gaz_read_char", {}, {i8});
    declareExt("gaz_idiv", {i32, i32}, {i32});
    declareExt("gaz_imod", {i32, i32}, {i32});
    declareExt("gaz_ipow", {i32, i32}, {i32});
    declareExt("gaz_rpow", {f32, f32}, {f32});
    declareExt("gaz_rmod", {f32, f32}, {f32});
    declareExt("gaz_overflow", {}, {});
  }
  Value rtCall(Location loc, const char *name, std::vector<mlir::Type> res,
               std::vector<Value> args) {
    auto c = mlir::func::CallOp::create(b, loc, name, res, args);
    return res.empty() ? Value() : c.getResult(0);
  }

  // ---- constants & conversions ----
  Value constInt(Location loc, mlir::Type t, int64_t v) {
    return mlir::arith::ConstantOp::create(b, loc, b.getIntegerAttr(t, v));
  }

  // Implicit conversion (integer -> real only).
  Value coerce(Location loc, int line, Value v, Type from, Type to,
               const char *what) {
    if (from == to) return v;
    if (from == Type::Integer && to == Type::Real)
      return mlir::arith::SIToFPOp::create(b, loc, b.getF32Type(), v);
    throw err("TypeError", line,
              std::string("cannot convert ") + typeName(from) + " to " +
                  typeName(to) + " in " + what);
  }

  // Explicit `as<T>(e)`.
  Value cast(Location loc, int line, Value v, Type from, Type to) {
    if (from == to) return v;
    auto i32 = b.getI32Type();
    switch (from) {
    case Type::Boolean:
      if (to == Type::Integer) return mlir::arith::ExtUIOp::create(b, loc, i32, v);
      if (to == Type::Character)
        return mlir::arith::ExtUIOp::create(b, loc, b.getI8Type(), v);
      if (to == Type::Real)
        return mlir::arith::UIToFPOp::create(b, loc, b.getF32Type(), v);
      break;
    case Type::Character:
      if (to == Type::Integer) return mlir::arith::ExtUIOp::create(b, loc, i32, v);
      if (to == Type::Boolean)
        return mlir::arith::CmpIOp::create(b, loc, mlir::arith::CmpIPredicate::ne,
                                           v, constInt(loc, b.getI8Type(), 0));
      if (to == Type::Real)
        return mlir::arith::UIToFPOp::create(b, loc, b.getF32Type(), v);
      break;
    case Type::Integer:
      if (to == Type::Real)
        return mlir::arith::SIToFPOp::create(b, loc, b.getF32Type(), v);
      if (to == Type::Boolean)
        return mlir::arith::CmpIOp::create(b, loc, mlir::arith::CmpIPredicate::ne,
                                           v, constInt(loc, i32, 0));
      if (to == Type::Character)
        return mlir::arith::TruncIOp::create(b, loc, b.getI8Type(), v);
      break;
    case Type::Real:
      if (to == Type::Integer)
        return mlir::arith::FPToSIOp::create(b, loc, i32, v);
      break;
    default: break;
    }
    throw err("TypeError", line,
              std::string("cannot cast ") + typeName(from) + " to " + typeName(to));
  }

  // ---- scopes ----
  Var *lookup(const std::string &n) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      auto f = it->find(n);
      if (f != it->end()) return &f->second;
    }
    return nullptr;
  }
  // Stack slots live in the entry block so loops don't grow the stack.
  Value newSlot(Location loc, Type t) {
    mlir::OpBuilder::InsertionGuard g(b);
    b.setInsertionPointToStart(entry);
    return mlir::memref::AllocaOp::create(b, loc,
                                          mlir::MemRefType::get({}, mlirType(t)));
  }

  // ---- expressions ----
  struct Val {
    Value v;
    Type t;
  };

  // `procOk`: a procedure call is legal here (right-hand side of a declaration
  // or assignment, possibly under unary operators and casts).
  Val genExpr(const Expr &e, bool procOk = false) {
    Location loc = b.getUnknownLoc();
    switch (e.kind) {
    case Expr::IntLit: {
      long long v = std::stoll(e.text);
      if (v > 2147483647LL)
        throw err("LiteralError", e.line, "integer literal does not fit in 32 bits");
      return {constInt(loc, b.getI32Type(), v), Type::Integer};
    }
    case Expr::RealLit:
      return {mlir::arith::ConstantOp::create(
                  b, loc, b.getF32FloatAttr(std::stof(e.text))),
              Type::Real};
    case Expr::BoolLit:
      return {constInt(loc, b.getI1Type(), e.text == "true"), Type::Boolean};
    case Expr::CharLit:
      return {constInt(loc, b.getI8Type(), (unsigned char)e.text[0]),
              Type::Character};
    case Expr::Var: {
      Var *var = lookup(e.text);
      if (!var) throw err("SymbolError", e.line, "undefined variable '" + e.text + "'");
      return {mlir::memref::LoadOp::create(b, loc, var->mem), var->type};
    }
    case Expr::Cast: {
      Val x = genExpr(*e.args[0], procOk);
      return {cast(loc, e.line, x.v, x.t, e.castTo), e.castTo};
    }
    case Expr::Unary: return genUnary(e, procOk);
    case Expr::Binary: return genBinary(e);
    case Expr::Call: {
      Val r = genCall(e.text, e.args, e.line, procOk ? CallPos::ExprProcOk : CallPos::Expr);
      if (r.t == Type::Void)
        throw err("CallError", e.line, "'" + e.text + "' returns no value");
      return r;
    }
    }
    throw err("TypeError", e.line, "unsupported expression");
  }

  enum class CallPos { Stmt, Expr, ExprProcOk };

  Val genCall(const std::string &name, const std::vector<ExprP> &args, int line,
              CallPos pos) {
    Location loc = b.getUnknownLoc();
    auto it = funcs.find(name);
    if (it == funcs.end())
      throw err("SymbolError", line, "call to undefined function '" + name + "'");
    const FuncInfo &fi = it->second;
    if (pos == CallPos::Stmt && !fi.isProc)
      throw err("CallError", line, "'call' applied to function '" + name + "'");
    if (fi.isProc) {
      if (!curIsProc)
        throw err("CallError", line, "function cannot call procedure '" + name + "'");
      if (pos == CallPos::Expr)
        throw err("CallError", line,
                  "call to procedure '" + name + "' is not allowed in this position");
    }
    if (fi.params.size() != args.size())
      throw err("TypeError", line, "wrong number of arguments to '" + name + "'");
    std::vector<Value> vals;
    for (size_t i = 0; i < args.size(); i++) {
      Val a = genExpr(*args[i]);
      vals.push_back(coerce(loc, line, a.v, a.t, fi.params[i], "argument"));
    }
    std::vector<mlir::Type> res;
    if (fi.ret != Type::Void) res.push_back(mlirType(fi.ret));
    auto c = mlir::func::CallOp::create(b, loc, "gaz_" + name, res, vals);
    return {res.empty() ? Value() : c.getResult(0), fi.ret};
  }

  // 32-bit signed arithmetic that raises MathError on overflow: compute in i64
  // and branch to a shared trap block if the result does not fit.
  Value checkedInt(Location loc, const std::string &op, Value l, Value r) {
    auto i64 = b.getI64Type();
    Value wl = mlir::arith::ExtSIOp::create(b, loc, i64, l);
    Value wr = mlir::arith::ExtSIOp::create(b, loc, i64, r);
    Value w = op == "+"   ? mlir::arith::AddIOp::create(b, loc, wl, wr).getResult()
              : op == "-" ? mlir::arith::SubIOp::create(b, loc, wl, wr).getResult()
                          : mlir::arith::MulIOp::create(b, loc, wl, wr).getResult();
    Value n = mlir::arith::TruncIOp::create(b, loc, b.getI32Type(), w);
    Value back = mlir::arith::ExtSIOp::create(b, loc, i64, n);
    Value ok = mlir::arith::CmpIOp::create(b, loc, mlir::arith::CmpIPredicate::eq, back, w);
    if (!trapBlock) {
      trapBlock = newBlock();
      mlir::OpBuilder::InsertionGuard g(b);
      b.setInsertionPointToStart(trapBlock);
      rtCall(loc, "gaz_overflow", {}, {});
      mlir::LLVM::UnreachableOp::create(b, loc);
    }
    Block *cont = newBlock();
    mlir::cf::CondBranchOp::create(b, loc, ok, cont, trapBlock);
    b.setInsertionPointToStart(cont);
    return n;
  }

  Val genUnary(const Expr &e, bool procOk) {
    Location loc = b.getUnknownLoc();
    Val x = genExpr(*e.args[0], procOk);
    if (e.text == "not") {
      if (x.t != Type::Boolean) throw err("TypeError", e.line, "'not' needs a boolean");
      return {mlir::arith::XOrIOp::create(b, loc, x.v, constInt(loc, b.getI1Type(), 1)),
              Type::Boolean};
    }
    if (x.t != Type::Integer && x.t != Type::Real)
      throw err("TypeError", e.line, "unary '" + e.text + "' needs a number");
    if (e.text == "+") return x;
    if (x.t == Type::Real)
      return {mlir::arith::NegFOp::create(b, loc, x.v), Type::Real};
    return {checkedInt(loc, "-", constInt(loc, b.getI32Type(), 0), x.v), Type::Integer};
  }

  Val genBinary(const Expr &e) {
    Location loc = b.getUnknownLoc();
    const std::string &op = e.text;
    Val l = genExpr(*e.args[0]), r = genExpr(*e.args[1]);
    using P = mlir::arith::CmpIPredicate;
    using FP = mlir::arith::CmpFPredicate;

    if (op == "and" || op == "or" || op == "xor") {
      if (l.t != Type::Boolean || r.t != Type::Boolean)
        throw err("TypeError", e.line, "'" + op + "' needs boolean operands");
      Value v = op == "and"  ? mlir::arith::AndIOp::create(b, loc, l.v, r.v).getResult()
                : op == "or" ? mlir::arith::OrIOp::create(b, loc, l.v, r.v).getResult()
                             : mlir::arith::XOrIOp::create(b, loc, l.v, r.v).getResult();
      return {v, Type::Boolean};
    }

    if (op == "==" || op == "!=") {
      if (l.t != r.t) { // allow integer/real mixing
        if (l.t == Type::Integer && r.t == Type::Real)
          l = {coerce(loc, e.line, l.v, l.t, Type::Real, "comparison"), Type::Real};
        else if (l.t == Type::Real && r.t == Type::Integer)
          r = {coerce(loc, e.line, r.v, r.t, Type::Real, "comparison"), Type::Real};
        else
          throw err("TypeError", e.line, std::string("cannot compare ") +
                                             typeName(l.t) + " with " + typeName(r.t));
      }
      bool eq = op == "==";
      if (l.t == Type::Real)
        return {mlir::arith::CmpFOp::create(b, loc, eq ? FP::OEQ : FP::UNE, l.v, r.v),
                Type::Boolean};
      return {mlir::arith::CmpIOp::create(b, loc, eq ? P::eq : P::ne, l.v, r.v),
              Type::Boolean};
    }

    // remaining operators are numeric
    if ((l.t != Type::Integer && l.t != Type::Real) ||
        (r.t != Type::Integer && r.t != Type::Real))
      throw err("TypeError", e.line, "operator '" + op + "' needs numeric operands");
    bool real = l.t == Type::Real || r.t == Type::Real;
    if (real) {
      l = {coerce(loc, e.line, l.v, l.t, Type::Real, "expression"), Type::Real};
      r = {coerce(loc, e.line, r.v, r.t, Type::Real, "expression"), Type::Real};
    }

    if (op == "<" || op == ">" || op == "<=" || op == ">=") {
      if (real) {
        FP p = op == "<" ? FP::OLT : op == ">" ? FP::OGT : op == "<=" ? FP::OLE : FP::OGE;
        return {mlir::arith::CmpFOp::create(b, loc, p, l.v, r.v), Type::Boolean};
      }
      P p = op == "<" ? P::slt : op == ">" ? P::sgt : op == "<=" ? P::sle : P::sge;
      return {mlir::arith::CmpIOp::create(b, loc, p, l.v, r.v), Type::Boolean};
    }

    Type rt = real ? Type::Real : Type::Integer;
    auto i32 = b.getI32Type();
    auto f32 = b.getF32Type();
    if (op == "+")
      return {real ? mlir::arith::AddFOp::create(b, loc, l.v, r.v).getResult()
                   : checkedInt(loc, op, l.v, r.v), rt};
    if (op == "-")
      return {real ? mlir::arith::SubFOp::create(b, loc, l.v, r.v).getResult()
                   : checkedInt(loc, op, l.v, r.v), rt};
    if (op == "*")
      return {real ? mlir::arith::MulFOp::create(b, loc, l.v, r.v).getResult()
                   : checkedInt(loc, op, l.v, r.v), rt};
    if (op == "/")
      return {real ? mlir::arith::DivFOp::create(b, loc, l.v, r.v).getResult()
                   : rtCall(loc, "gaz_idiv", {i32}, {l.v, r.v}), rt};
    if (op == "%") {
      if (real) return {rtCall(loc, "gaz_rmod", {f32}, {l.v, r.v}), rt};
      return {rtCall(loc, "gaz_imod", {i32}, {l.v, r.v}), rt};
    }
    if (op == "^")
      return {real ? rtCall(loc, "gaz_rpow", {f32}, {l.v, r.v})
                   : rtCall(loc, "gaz_ipow", {i32}, {l.v, r.v}), rt};
    throw err("TypeError", e.line, "unknown operator '" + op + "'");
  }

  // ---- statements ----
  // After a terminator, further statements land in a fresh unreachable block.
  bool terminated() {
    Block *blk = b.getInsertionBlock();
    return !blk->empty() && blk->back().hasTrait<mlir::OpTrait::IsTerminator>();
  }
  void startDeadBlock() {
    Block *blk = b.createBlock(&curFn.getBody());
    b.setInsertionPointToStart(blk);
  }

  struct LoopCtx {
    Block *cont, *exit;
  };
  std::vector<LoopCtx> loops;

  Block *newBlock() {
    mlir::OpBuilder::InsertionGuard g(b);
    return b.createBlock(&curFn.getBody());
  }
  void branchIfOpen(Block *dest) {
    if (!terminated()) mlir::cf::BranchOp::create(b, b.getUnknownLoc(), dest);
  }
  Value genCond(const Expr &e, const char *what) {
    Val c = genExpr(e);
    if (c.t != Type::Boolean)
      throw err("TypeError", e.line, std::string("'") + what + "' condition must be boolean");
    return c.v;
  }

  void genBlock(const std::vector<StmtP> &stmts) {
    scopes.emplace_back();
    for (auto &s : stmts) {
      if (terminated()) startDeadBlock();
      genStmt(*s);
    }
    scopes.pop_back();
  }

  void genStmt(const Stmt &s) {
    Location loc = b.getUnknownLoc();
    switch (s.kind) {
    case Stmt::Block: genBlock(s.body); return;
    case Stmt::Decl: {
      if (scopes.back().count(s.name))
        throw err("SymbolError", s.line, "'" + s.name + "' already declared in this scope");
      Type t = s.declType;
      Value init;
      if (s.expr) {
        Val x = genExpr(*s.expr, true);
        if (!s.hasType) t = x.t;
        init = coerce(loc, s.line, x.v, x.t, t, "declaration");
      } else {
        // Uninitialised variables default to zero.
        init = t == Type::Real
                   ? mlir::arith::ConstantOp::create(b, loc, b.getF32FloatAttr(0))
                         .getResult()
                   : constInt(loc, mlirType(t), 0);
      }
      Value mem = newSlot(loc, t);
      mlir::memref::StoreOp::create(b, loc, init, mem);
      scopes.back()[s.name] = {mem, t, s.isConst};
      return;
    }
    case Stmt::Assign: {
      Var *v = lookup(s.name);
      if (!v) throw err("SymbolError", s.line, "undefined variable '" + s.name + "'");
      if (v->isConst) throw err("AssignError", s.line, "assignment to const '" + s.name + "'");
      Val x = genExpr(*s.expr, true);
      mlir::memref::StoreOp::create(
          b, loc, coerce(loc, s.line, x.v, x.t, v->type, "assignment"), v->mem);
      return;
    }
    case Stmt::Output: {
      if (!curIsProc) throw err("StatementError", s.line, "I/O is not allowed in a function");
      Val x = genExpr(*s.expr);
      switch (x.t) {
      case Type::Integer: rtCall(loc, "gaz_print_int", {}, {x.v}); break;
      case Type::Real: rtCall(loc, "gaz_print_real", {}, {x.v}); break;
      case Type::Character: rtCall(loc, "gaz_print_char", {}, {x.v}); break;
      case Type::Boolean:
        rtCall(loc, "gaz_print_bool", {},
               {mlir::arith::ExtUIOp::create(b, loc, b.getI32Type(), x.v)});
        break;
      default: throw err("TypeError", s.line, "cannot print this value");
      }
      return;
    }
    case Stmt::Input: {
      if (!curIsProc) throw err("StatementError", s.line, "I/O is not allowed in a function");
      Var *v = lookup(s.name);
      if (!v) throw err("SymbolError", s.line, "undefined variable '" + s.name + "'");
      if (v->isConst) throw err("AssignError", s.line, "input into const '" + s.name + "'");
      Value r;
      switch (v->type) {
      case Type::Integer: r = rtCall(loc, "gaz_read_int", {b.getI32Type()}, {}); break;
      case Type::Real: r = rtCall(loc, "gaz_read_real", {b.getF32Type()}, {}); break;
      case Type::Character: r = rtCall(loc, "gaz_read_char", {b.getI8Type()}, {}); break;
      default: throw err("TypeError", s.line, "cannot read into a boolean");
      }
      mlir::memref::StoreOp::create(b, loc, r, v->mem);
      return;
    }
    case Stmt::CallS: genCall(s.name, s.args, s.line, CallPos::Stmt); return;
    case Stmt::Return: {
      if (curRet == Type::Void) {
        if (s.expr) throw err("TypeError", s.line, "returning a value from a void procedure");
        mlir::func::ReturnOp::create(b, loc, mlir::ValueRange{});
      } else {
        if (!s.expr) throw err("TypeError", s.line, "missing return value");
        Val x = genExpr(*s.expr);
        mlir::func::ReturnOp::create(
            b, loc, coerce(loc, s.line, x.v, x.t, curRet, "return"));
      }
      return;
    }
    case Stmt::If: {
      Value c = genCond(*s.expr, "if");
      Block *thenB = newBlock(), *elseB = newBlock(), *merge = newBlock();
      mlir::cf::CondBranchOp::create(b, loc, c, thenB, elseB);
      b.setInsertionPointToStart(thenB);
      genBlock(s.body);
      branchIfOpen(merge);
      b.setInsertionPointToStart(elseB);
      genBlock(s.elseBody);
      branchIfOpen(merge);
      b.setInsertionPointToStart(merge);
      return;
    }
    case Stmt::Loop: {
      Block *bodyB = newBlock(), *exitB = newBlock();
      Block *condB = s.loopKind == Stmt::Forever ? nullptr : newBlock();
      // `continue` re-tests the condition, or restarts an unconditional loop.
      Block *contB = condB ? condB : bodyB;
      mlir::cf::BranchOp::create(b, loc, s.loopKind == Stmt::PreTest ? condB : bodyB);
      if (condB) {
        b.setInsertionPointToStart(condB);
        Value c = genCond(*s.expr, "loop");
        mlir::cf::CondBranchOp::create(b, loc, c, bodyB, exitB);
      }
      loops.push_back({contB, exitB});
      b.setInsertionPointToStart(bodyB);
      genBlock(s.body);
      branchIfOpen(contB);
      loops.pop_back();
      b.setInsertionPointToStart(exitB);
      return;
    }
    case Stmt::Break:
    case Stmt::Continue: {
      if (loops.empty())
        throw err("StatementError", s.line,
                  s.kind == Stmt::Break ? "'break' outside of a loop"
                                        : "'continue' outside of a loop");
      mlir::cf::BranchOp::create(
          b, loc, s.kind == Stmt::Break ? loops.back().exit : loops.back().cont);
      return;
    }
    }
  }

  // True if every control path through `stmts` reaches a return. Loops never
  // count: the spec treats the end of a loop as reachable.
  static bool alwaysReturns(const std::vector<StmtP> &stmts) {
    for (auto &s : stmts) {
      switch (s->kind) {
      case Stmt::Return: return true;
      case Stmt::Block:
        if (alwaysReturns(s->body)) return true;
        break;
      case Stmt::If:
        if (!s->elseBody.empty() && alwaysReturns(s->body) && alwaysReturns(s->elseBody))
          return true;
        break;
      default: break;
      }
    }
    return false;
  }

  void genFunc(const Func &f) {
    Location loc = b.getUnknownLoc();
    curIsProc = f.isProcedure;
    trapBlock = nullptr;
    if (f.ret != Type::Void && !f.exprBody && !alwaysReturns(f.body))
      throw err("ReturnError", f.line,
                "'" + f.name + "' does not return on every control path");
    std::vector<mlir::Type> in, out;
    for (auto &p : f.params) in.push_back(mlirType(p.type));
    if (f.ret != Type::Void) out.push_back(mlirType(f.ret));
    b.setInsertionPointToEnd(module->getBody());
    curFn = mlir::func::FuncOp::create(b, loc, "gaz_" + f.name,
                                       b.getFunctionType(in, out));
    entry = curFn.addEntryBlock();
    b.setInsertionPointToStart(entry);
    curRet = f.ret;

    scopes.clear();
    loops.clear();
    scopes.emplace_back();
    for (size_t i = 0; i < f.params.size(); i++) {
      Value mem = newSlot(loc, f.params[i].type);
      mlir::memref::StoreOp::create(b, loc, entry->getArgument(i), mem);
      scopes.back()[f.params[i].name] = {mem, f.params[i].type, true};
    }

    if (f.exprBody) {
      Val x = genExpr(*f.exprBody);
      mlir::func::ReturnOp::create(
          b, loc, coerce(loc, f.line, x.v, x.t, f.ret, "function body"));
    } else {
      genBlock(f.body);
      // Falling off the end: implicit return (zero value for non-void).
      if (terminated()) { /* ok */ } else if (f.ret == Type::Void) {
        mlir::func::ReturnOp::create(b, loc, mlir::ValueRange{});
      } else {
        Value z = f.ret == Type::Real
                      ? mlir::arith::ConstantOp::create(b, loc, b.getF32FloatAttr(0))
                            .getResult()
                      : constInt(loc, mlirType(f.ret), 0);
        mlir::func::ReturnOp::create(b, loc, z);
      }
    }
    // Terminate any leftover unreachable blocks.
    for (Block &blk : curFn.getBody()) {
      if (!blk.empty() && blk.back().hasTrait<mlir::OpTrait::IsTerminator>()) continue;
      b.setInsertionPointToEnd(&blk);
      if (f.ret == Type::Void) {
        mlir::func::ReturnOp::create(b, loc, mlir::ValueRange{});
      } else {
        Value z = f.ret == Type::Real
                      ? mlir::arith::ConstantOp::create(b, loc, b.getF32FloatAttr(0))
                            .getResult()
                      : constInt(loc, mlirType(f.ret), 0);
        mlir::func::ReturnOp::create(b, loc, z);
      }
    }
    pruneUnreachable();
  }

  // Dialect conversion never visits unreachable blocks, so drop them (and any
  // blocks that only they reach) before lowering.
  void pruneUnreachable() {
    bool changed = true;
    while (changed) {
      changed = false;
      std::vector<Block *> dead;
      for (Block &blk : curFn.getBody())
        if (&blk != entry && blk.hasNoPredecessors()) dead.push_back(&blk);
      for (Block *blk : dead) {
        blk->dropAllReferences();
        blk->erase();
        changed = true;
      }
    }
  }
};

} // namespace

mlir::OwningOpRef<mlir::ModuleOp> genMLIR(mlir::MLIRContext &ctx,
                                          const Program &prog) {
  return Gen(ctx).run(prog);
}

} // namespace gaz

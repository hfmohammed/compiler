#include "MLIRGen.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <unordered_map>

namespace gaz {
namespace {

using mlir::Block;
using mlir::Location;
using mlir::Value;

// Tuples are flattened: a value of tuple type is the list of its scalar leaves
// (depth first), and a tuple variable owns one stack slot per leaf. Function
// parameters and results are flattened the same way.
struct Val {
  Value v;               // scalar value
  Type t;
  std::vector<Value> fs; // tuple leaves
};

struct Var {
  std::vector<Value> mems; // one rank-0 memref per leaf
  Type type;
  bool isConst;
};

struct FuncInfo {
  std::vector<Type> params;
  std::vector<bool> isVar;
  Type ret;
  bool isProc = false;
  bool defined = false;
  int line = 0;
};

class Gen {
public:
  Gen(mlir::MLIRContext &c) : ctx(c), b(&c) {}

  mlir::OwningOpRef<mlir::ModuleOp> run(const Program &prog) {
    module = mlir::ModuleOp::create(b.getUnknownLoc());
    b.setInsertionPointToEnd(module->getBody());
    declareRuntime();

    for (auto &g : prog.globals) declareGlobal(*g);

    for (auto &f : prog.funcs) declare(f);
    for (auto &[name, fi] : funcs)
      if (!fi.defined)
        throw err("DefinitionError", fi.line, "'" + name + "' is declared but never defined");

    auto it = funcs.find("main");
    if (it == funcs.end())
      throw err("MainError", 1, "program does not have a main procedure");
    if (!it->second.isProc || !it->second.params.empty() ||
        it->second.ret != Type::Integer)
      throw err("MainError", 1,
                "main must be a procedure with no arguments returning integer");

    for (auto &f : prog.funcs)
      if (!f.isProto) genFunc(f);
    return std::move(module);
  }

private:
  mlir::MLIRContext &ctx;
  mlir::OpBuilder b;
  mlir::OwningOpRef<mlir::ModuleOp> module;
  std::unordered_map<std::string, FuncInfo> funcs;

  struct ConstLeaf {
    Type type;
    int64_t integer = 0;
    double real = 0;
  };
  struct ConstVal {
    Type type;
    std::vector<ConstLeaf> leaves;
  };
  std::unordered_map<std::string, ConstVal> globals;
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

  // ---- function table ----
  void declare(const Func &f) {
    FuncInfo fi;
    fi.ret = f.ret;
    fi.isProc = f.isProcedure;
    fi.line = f.line;
    for (auto &p : f.params) {
      fi.params.push_back(p.type);
      fi.isVar.push_back(p.isVar);
    }
    auto it = funcs.find(f.name);
    if (it == funcs.end()) {
      fi.defined = !f.isProto;
      funcs[f.name] = fi;
      return;
    }
    FuncInfo &old = it->second;
    if (old.params != fi.params || old.isVar != fi.isVar || old.ret != fi.ret ||
        old.isProc != fi.isProc)
      throw err("TypeError", f.line,
                "'" + f.name + "' does not match its earlier declaration");
    if (!f.isProto) {
      if (old.defined) throw err("SymbolError", f.line, "redefinition of '" + f.name + "'");
      old.defined = true;
    }
    // a repeated prototype is harmless
  }

  // ---- types ----
  mlir::Type mlirScalar(Type t) {
    switch (t.kind) {
    case Type::Integer: return b.getI32Type();
    case Type::Real: return b.getF32Type();
    case Type::Boolean: return b.getI1Type();
    case Type::Character: return b.getI8Type();
    default: return mlir::Type();
    }
  }
  static void leafTypes(const Type &t, std::vector<Type> &out) {
    if (t.isTuple()) for (auto &e : t.elems) leafTypes(e, out);
    else if (t.kind != Type::Void) out.push_back(t);
  }
  static std::vector<Type> leavesOf(const Type &t) {
    std::vector<Type> v;
    leafTypes(t, v);
    return v;
  }
  static size_t leafCount(const Type &t) { return leavesOf(t).size(); }
  static std::string typeName(const Type &t) {
    switch (t.kind) {
    case Type::Integer: return "integer";
    case Type::Real: return "real";
    case Type::Boolean: return "boolean";
    case Type::Character: return "character";
    case Type::Tuple: {
      std::string s = "tuple(";
      for (size_t i = 0; i < t.elems.size(); i++) s += (i ? ", " : "") + typeName(t.elems[i]);
      return s + ")";
    }
    default: return "void";
    }
  }

  static ConstVal constScalar(Type t, int64_t integer, double real = 0) {
    return {t, {{t, integer, real}}};
  }
  static ConstVal constElem(const ConstVal &value, size_t index) {
    size_t offset = 0;
    for (size_t i = 0; i < index; i++) offset += leafCount(value.type.elems[i]);
    size_t count = leafCount(value.type.elems[index]);
    return {value.type.elems[index],
            std::vector<ConstLeaf>(value.leaves.begin() + offset,
                                   value.leaves.begin() + offset + count)};
  }

  ConstVal coerceConst(int line, const ConstVal &value, const Type &to,
                       const char *what) {
    if (value.type == to) return value;
    if (value.type.isTuple() && to.isTuple() &&
        value.type.elems.size() == to.elems.size()) {
      ConstVal result{to, {}};
      for (size_t i = 0; i < to.elems.size(); i++) {
        ConstVal part = coerceConst(line, constElem(value, i), to.elems[i], what);
        result.leaves.insert(result.leaves.end(), part.leaves.begin(), part.leaves.end());
      }
      return result;
    }
    if (value.type == Type::Integer && to == Type::Real)
      return constScalar(Type::Real, 0, static_cast<float>(value.leaves[0].integer));
    throw err("TypeError", line, "cannot convert " + typeName(value.type) + " to " +
                                    typeName(to) + " in " + what);
  }

  ConstVal castConst(int line, const ConstVal &value, const Type &to) {
    if (value.type == to) return value;
    if (value.type.isTuple() && to.isTuple() &&
        value.type.elems.size() == to.elems.size()) {
      ConstVal result{to, {}};
      for (size_t i = 0; i < to.elems.size(); i++) {
        ConstVal part = castConst(line, constElem(value, i), to.elems[i]);
        result.leaves.insert(result.leaves.end(), part.leaves.begin(), part.leaves.end());
      }
      return result;
    }
    const ConstLeaf &leaf = value.leaves[0];
    if (value.type == Type::Boolean) {
      if (to == Type::Integer) return constScalar(to, leaf.integer != 0);
      if (to == Type::Character) return constScalar(to, leaf.integer != 0);
      if (to == Type::Real) return constScalar(to, 0, leaf.integer != 0 ? 1.0f : 0.0f);
    }
    if (value.type == Type::Character) {
      uint8_t ch = static_cast<uint8_t>(leaf.integer);
      if (to == Type::Integer) return constScalar(to, ch);
      if (to == Type::Boolean) return constScalar(to, ch != 0);
      if (to == Type::Real) return constScalar(to, 0, static_cast<float>(ch));
    }
    if (value.type == Type::Integer) {
      int32_t integer = static_cast<int32_t>(leaf.integer);
      if (to == Type::Real) return constScalar(to, 0, static_cast<float>(integer));
      if (to == Type::Boolean) return constScalar(to, integer != 0);
      if (to == Type::Character) return constScalar(to, static_cast<uint8_t>(integer));
    }
    if (value.type == Type::Real && to == Type::Integer) {
      if (!std::isfinite(leaf.real) || leaf.real < std::numeric_limits<int32_t>::min() ||
          leaf.real > std::numeric_limits<int32_t>::max())
        throw err("MathError", line, "real-to-integer cast is out of range");
      return constScalar(to, static_cast<int32_t>(leaf.real));
    }
    throw err("TypeError", line,
              "cannot cast " + typeName(value.type) + " to " + typeName(to));
  }

  int32_t checkedConstInt(int line, int64_t value) {
    if (value < std::numeric_limits<int32_t>::min() ||
        value > std::numeric_limits<int32_t>::max())
      throw err("MathError", line, "integer overflow in global initializer");
    return static_cast<int32_t>(value);
  }

  bool constEqual(int line, ConstVal left, ConstVal right) {
    if (left.type.isTuple() || right.type.isTuple()) {
      if (!(left.type.isTuple() && right.type.isTuple()) ||
          left.type.elems.size() != right.type.elems.size())
        throw err("TypeError", line, "cannot compare " + typeName(left.type) + " with " +
                                         typeName(right.type));
      for (size_t i = 0; i < left.type.elems.size(); i++)
        if (!constEqual(line, constElem(left, i), constElem(right, i))) return false;
      return true;
    }
    if (left.type != right.type) {
      if (left.type == Type::Integer && right.type == Type::Real)
        left = coerceConst(line, left, Type::Real, "comparison");
      else if (left.type == Type::Real && right.type == Type::Integer)
        right = coerceConst(line, right, Type::Real, "comparison");
      else
        throw err("TypeError", line, "cannot compare " + typeName(left.type) + " with " +
                                         typeName(right.type));
    }
    if (left.type == Type::Real) return left.leaves[0].real == right.leaves[0].real;
    return left.leaves[0].integer == right.leaves[0].integer;
  }

  ConstVal evalConst(const Expr &expr) {
    switch (expr.kind) {
    case Expr::IntLit: {
      long long value = std::stoll(expr.text);
      if (value > std::numeric_limits<int32_t>::max())
        throw err("LiteralError", expr.line, "integer literal does not fit in 32 bits");
      return constScalar(Type::Integer, value);
    }
    case Expr::RealLit: {
      try {
        return constScalar(Type::Real, 0, std::stof(expr.text));
      } catch (std::out_of_range &) {
        throw err("LiteralError", expr.line, "real literal does not fit in 32 bits");
      }
    }
    case Expr::BoolLit: return constScalar(Type::Boolean, expr.text == "true");
    case Expr::CharLit:
      return constScalar(Type::Character, static_cast<unsigned char>(expr.text[0]));
    case Expr::Var: {
      auto it = globals.find(expr.text);
      if (it == globals.end())
        throw err("SymbolError", expr.line,
                  "global initializer references undefined symbol '" + expr.text + "'");
      return it->second;
    }
    case Expr::TupleAccess: {
      auto it = globals.find(expr.text);
      if (it == globals.end())
        throw err("SymbolError", expr.line,
                  "global initializer references undefined symbol '" + expr.text + "'");
      if (!it->second.type.isTuple())
        throw err("TypeError", expr.line, "'" + expr.text + "' is not a tuple");
      if (expr.idx < 1 || static_cast<size_t>(expr.idx) > it->second.type.elems.size())
        throw err("IndexError", expr.line, "tuple field index is out of range");
      return constElem(it->second, expr.idx - 1);
    }
    case Expr::TupleLit: {
      std::vector<Type> types;
      ConstVal result;
      for (auto &arg : expr.args) {
        ConstVal value = evalConst(*arg);
        types.push_back(value.type);
        result.leaves.insert(result.leaves.end(), value.leaves.begin(), value.leaves.end());
      }
      result.type = Type::tuple(std::move(types));
      return result;
    }
    case Expr::Cast: return castConst(expr.line, evalConst(*expr.args[0]), expr.castTo);
    case Expr::Unary: {
      ConstVal value = evalConst(*expr.args[0]);
      if (expr.text == "not") {
        if (value.type != Type::Boolean)
          throw err("TypeError", expr.line, "'not' needs a boolean");
        return constScalar(Type::Boolean, !value.leaves[0].integer);
      }
      if (!value.type.isNumeric())
        throw err("TypeError", expr.line, "unary '" + expr.text + "' needs a number");
      if (expr.text == "+") return value;
      if (value.type == Type::Real)
        return constScalar(Type::Real, 0, static_cast<float>(-value.leaves[0].real));
      return constScalar(Type::Integer,
                         checkedConstInt(expr.line, -value.leaves[0].integer));
    }
    case Expr::Binary: return evalConstBinary(expr);
    case Expr::StrLit:
      throw err("TypeError", expr.line, "string values are not yet supported");
    case Expr::StreamState:
    case Expr::Call:
      throw err("GlobalError", expr.line,
                "global initializer is not a constant expression");
    }
    throw err("GlobalError", expr.line, "unsupported global initializer");
  }

  ConstVal evalConstBinary(const Expr &expr) {
    ConstVal left = evalConst(*expr.args[0]);
    ConstVal right = evalConst(*expr.args[1]);
    const std::string &op = expr.text;

    if (op == "and" || op == "or" || op == "xor") {
      if (left.type != Type::Boolean || right.type != Type::Boolean)
        throw err("TypeError", expr.line, "'" + op + "' needs boolean operands");
      bool a = left.leaves[0].integer != 0;
      bool b = right.leaves[0].integer != 0;
      return constScalar(Type::Boolean,
                         op == "and" ? a && b : op == "or" ? a || b : a != b);
    }
    if (op == "==" || op == "!=") {
      bool equal = constEqual(expr.line, left, right);
      return constScalar(Type::Boolean, op == "==" ? equal : !equal);
    }
    if (!left.type.isNumeric() || !right.type.isNumeric())
      throw err("TypeError", expr.line, "operator '" + op + "' needs numeric operands");

    bool real = left.type == Type::Real || right.type == Type::Real;
    if (real) {
      left = coerceConst(expr.line, left, Type::Real, "expression");
      right = coerceConst(expr.line, right, Type::Real, "expression");
      float a = static_cast<float>(left.leaves[0].real);
      float b = static_cast<float>(right.leaves[0].real);
      if (op == "<" || op == ">" || op == "<=" || op == ">=")
        return constScalar(Type::Boolean,
                           op == "<" ? a < b : op == ">" ? a > b : op == "<=" ? a <= b : a >= b);
      if (op == "+") return constScalar(Type::Real, 0, a + b);
      if (op == "-") return constScalar(Type::Real, 0, a - b);
      if (op == "*") return constScalar(Type::Real, 0, a * b);
      if (op == "/") return constScalar(Type::Real, 0, a / b);
      if (op == "%") return constScalar(Type::Real, 0, std::fmod(a, b));
      if (op == "^") return constScalar(Type::Real, 0, std::pow(a, b));
    } else {
      int32_t a = static_cast<int32_t>(left.leaves[0].integer);
      int32_t b = static_cast<int32_t>(right.leaves[0].integer);
      if (op == "<" || op == ">" || op == "<=" || op == ">=")
        return constScalar(Type::Boolean,
                           op == "<" ? a < b : op == ">" ? a > b : op == "<=" ? a <= b : a >= b);
      if (op == "+") return constScalar(Type::Integer, checkedConstInt(expr.line, int64_t(a) + b));
      if (op == "-") return constScalar(Type::Integer, checkedConstInt(expr.line, int64_t(a) - b));
      if (op == "*") return constScalar(Type::Integer, checkedConstInt(expr.line, int64_t(a) * b));
      if (op == "/" || op == "%") {
        if (b == 0) throw err("MathError", expr.line, op == "/" ? "division by zero" : "modulo by zero");
        if (a == std::numeric_limits<int32_t>::min() && b == -1)
          throw err("MathError", expr.line, "integer overflow in global initializer");
        return constScalar(Type::Integer, op == "/" ? a / b : a % b);
      }
      if (op == "^") {
        if (a == 0 && b <= 0)
          throw err("MathError", expr.line, "zero raised to a non-positive power");
        if (b < 0) {
          if (a == 1) return constScalar(Type::Integer, 1);
          if (a == -1) return constScalar(Type::Integer, b % 2 ? -1 : 1);
          return constScalar(Type::Integer, 0);
        }
        int32_t result = 1;
        for (int32_t exponent = 0; exponent < b; exponent++)
          result = checkedConstInt(expr.line, int64_t(result) * a);
        return constScalar(Type::Integer, result);
      }
    }
    throw err("TypeError", expr.line, "unknown operator '" + op + "'");
  }

  void declareGlobal(const Stmt &decl) {
    if (!decl.isConst)
      throw err("GlobalError", decl.line, "global variables must be const");
    if (!decl.expr)
      throw err("GlobalError", decl.line, "global variables must have an initializer");
    if (globals.count(decl.name))
      throw err("SymbolError", decl.line, "global '" + decl.name + "' already declared");
    ConstVal value = evalConst(*decl.expr);
    Type type = decl.hasType ? decl.declType : value.type;
    globals[decl.name] = coerceConst(decl.line, value, type, "global declaration");
  }
  // flattened mlir types of a value / of a by-reference parameter
  std::vector<mlir::Type> valueTypes(const Type &t) {
    std::vector<mlir::Type> v;
    for (auto &l : leavesOf(t)) v.push_back(mlirScalar(l));
    return v;
  }
  std::vector<mlir::Type> refTypes(const Type &t) {
    std::vector<mlir::Type> v;
    for (auto &l : leavesOf(t)) v.push_back(mlir::MemRefType::get({}, mlirScalar(l)));
    return v;
  }

  // ---- values ----
  static std::vector<Value> leaves(const Val &x) {
    if (x.t.isTuple()) return x.fs;
    if (x.t.kind == Type::Void) return {};
    return {x.v};
  }
  static Val fromLeaves(const Type &t, const std::vector<Value> &ls) {
    Val x;
    x.t = t;
    if (t.isTuple()) x.fs = ls;
    else if (!ls.empty()) x.v = ls[0];
    return x;
  }
  static Val scalar(Value v, Type t) {
    Val x;
    x.v = v;
    x.t = t;
    return x;
  }
  // Member `i` of a tuple value.
  static Val elem(const Val &tv, size_t i) {
    size_t off = 0;
    for (size_t k = 0; k < i; k++) off += leafCount(tv.t.elems[k]);
    size_t n = leafCount(tv.t.elems[i]);
    return fromLeaves(tv.t.elems[i],
                      std::vector<Value>(tv.fs.begin() + off, tv.fs.begin() + off + n));
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
    declareExt("gaz_read_bool", {}, {i32});
    declareExt("gaz_stream_state", {}, {i32});
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
  Value zeroLeaf(Location loc, Type t) {
    if (t.kind == Type::Real)
      return mlir::arith::ConstantOp::create(b, loc, b.getF32FloatAttr(0));
    return constInt(loc, mlirScalar(t), 0);
  }
  Val zeroVal(Location loc, const Type &t) {
    std::vector<Value> ls;
    for (auto &l : leavesOf(t)) ls.push_back(zeroLeaf(loc, l));
    return fromLeaves(t, ls);
  }
  Val materializeConst(Location loc, const ConstVal &value) {
    std::vector<Value> leaves;
    for (const ConstLeaf &leaf : value.leaves) {
      if (leaf.type == Type::Real)
        leaves.push_back(mlir::arith::ConstantOp::create(
            b, loc, b.getF32FloatAttr(static_cast<float>(leaf.real))));
      else
        leaves.push_back(constInt(loc, mlirScalar(leaf.type), leaf.integer));
    }
    return fromLeaves(value.type, leaves);
  }

  // Implicit conversion: integer -> real, applied member-wise to tuples.
  Val coerce(Location loc, int line, const Val &x, const Type &to, const char *what) {
    if (x.t == to) return x;
    if (x.t.isTuple() && to.isTuple() && x.t.elems.size() == to.elems.size()) {
      std::vector<Value> ls;
      for (size_t i = 0; i < to.elems.size(); i++) {
        auto part = leaves(coerce(loc, line, elem(x, i), to.elems[i], what));
        ls.insert(ls.end(), part.begin(), part.end());
      }
      return fromLeaves(to, ls);
    }
    if (x.t == Type::Integer && to == Type::Real)
      return scalar(mlir::arith::SIToFPOp::create(b, loc, b.getF32Type(), x.v), Type::Real);
    throw err("TypeError", line,
              "cannot convert " + typeName(x.t) + " to " + typeName(to) + " in " + what);
  }

  // Explicit `as<T>(e)`.
  Val cast(Location loc, int line, const Val &x, const Type &to) {
    if (x.t == to) return x;
    if (x.t.isTuple() && to.isTuple() && x.t.elems.size() == to.elems.size()) {
      std::vector<Value> ls;
      for (size_t i = 0; i < to.elems.size(); i++) {
        auto part = leaves(cast(loc, line, elem(x, i), to.elems[i]));
        ls.insert(ls.end(), part.begin(), part.end());
      }
      return fromLeaves(to, ls);
    }
    Value v = x.v;
    auto i32 = b.getI32Type();
    using P = mlir::arith::CmpIPredicate;
    switch (x.t.kind) {
    case Type::Boolean:
      if (to == Type::Integer) return scalar(mlir::arith::ExtUIOp::create(b, loc, i32, v), to);
      if (to == Type::Character)
        return scalar(mlir::arith::ExtUIOp::create(b, loc, b.getI8Type(), v), to);
      if (to == Type::Real)
        return scalar(mlir::arith::UIToFPOp::create(b, loc, b.getF32Type(), v), to);
      break;
    case Type::Character:
      if (to == Type::Integer) return scalar(mlir::arith::ExtUIOp::create(b, loc, i32, v), to);
      if (to == Type::Boolean)
        return scalar(mlir::arith::CmpIOp::create(b, loc, P::ne, v, constInt(loc, b.getI8Type(), 0)), to);
      if (to == Type::Real)
        return scalar(mlir::arith::UIToFPOp::create(b, loc, b.getF32Type(), v), to);
      break;
    case Type::Integer:
      if (to == Type::Real)
        return scalar(mlir::arith::SIToFPOp::create(b, loc, b.getF32Type(), v), to);
      if (to == Type::Boolean)
        return scalar(mlir::arith::CmpIOp::create(b, loc, P::ne, v, constInt(loc, i32, 0)), to);
      if (to == Type::Character)
        return scalar(mlir::arith::TruncIOp::create(b, loc, b.getI8Type(), v), to);
      break;
    case Type::Real:
      if (to == Type::Integer)
        return scalar(mlir::arith::FPToSIOp::create(b, loc, i32, v), to);
      break;
    default: break;
    }
    throw err("TypeError", line, "cannot cast " + typeName(x.t) + " to " + typeName(to));
  }

  // ---- scopes & storage ----
  Var *lookup(const std::string &n) {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      auto f = it->find(n);
      if (f != it->end()) return &f->second;
    }
    return nullptr;
  }
  Var &need(const std::string &n, int line) {
    Var *v = lookup(n);
    if (!v) throw err("SymbolError", line, "undefined variable '" + n + "'");
    return *v;
  }
  // Stack slots live in the entry block so loops don't grow the stack.
  Value newSlot(Location loc, Type t) {
    mlir::OpBuilder::InsertionGuard g(b);
    b.setInsertionPointToStart(entry);
    return mlir::memref::AllocaOp::create(b, loc,
                                          mlir::MemRefType::get({}, mlirScalar(t)));
  }
  std::vector<Value> newSlots(Location loc, const Type &t) {
    std::vector<Value> m;
    for (auto &l : leavesOf(t)) m.push_back(newSlot(loc, l));
    return m;
  }
  void store(Location loc, const std::vector<Value> &mems, const Val &x) {
    auto ls = leaves(x);
    for (size_t i = 0; i < mems.size(); i++) mlir::memref::StoreOp::create(b, loc, ls[i], mems[i]);
  }
  Val load(Location loc, const std::vector<Value> &mems, const Type &t) {
    std::vector<Value> ls;
    for (auto m : mems) ls.push_back(mlir::memref::LoadOp::create(b, loc, m));
    return fromLeaves(t, ls);
  }

  // An assignable location: a variable or one of its tuple fields.
  struct LV {
    std::vector<Value> mems;
    Type type;
    bool isConst;
    std::string key; // "t" or "t.2", for aliasing checks
  };
  LV resolveLV(const std::string &name, int idx, int line) {
    Var *found = lookup(name);
    if (!found) {
      if (globals.count(name))
        throw err("AssignError", line, "global '" + name + "' is const");
      throw err("SymbolError", line, "undefined variable '" + name + "'");
    }
    Var &v = *found;
    if (idx == 0) return {v.mems, v.type, v.isConst, name};
    if (!v.type.isTuple())
      throw err("TypeError", line, "'" + name + "' is not a tuple");
    if (idx < 1 || (size_t)idx > v.type.elems.size())
      throw err("IndexError", line, "tuple field index " + std::to_string(idx) + " is out of range");
    size_t off = 0;
    for (int k = 0; k < idx - 1; k++) off += leafCount(v.type.elems[k]);
    size_t n = leafCount(v.type.elems[idx - 1]);
    return {std::vector<Value>(v.mems.begin() + off, v.mems.begin() + off + n),
            v.type.elems[idx - 1], v.isConst, name + "." + std::to_string(idx)};
  }

  // ---- expressions ----
  // `procOk`: a procedure call is legal here (right-hand side of a declaration
  // or assignment, possibly under unary operators and casts).
  Val genExpr(const Expr &e, bool procOk = false) {
    Location loc = b.getUnknownLoc();
    switch (e.kind) {
    case Expr::IntLit: {
      long long v = std::stoll(e.text);
      if (v > 2147483647LL)
        throw err("LiteralError", e.line, "integer literal does not fit in 32 bits");
      return scalar(constInt(loc, b.getI32Type(), v), Type::Integer);
    }
    case Expr::RealLit: {
      float v;
      try {
        v = std::stof(e.text);
      } catch (std::out_of_range &) {
        throw err("LiteralError", e.line, "real literal does not fit in 32 bits");
      }
      return scalar(mlir::arith::ConstantOp::create(b, loc, b.getF32FloatAttr(v)), Type::Real);
    }
    case Expr::StrLit:
      throw err("TypeError", e.line, "string values are only supported as stream output");
    case Expr::StreamState:
      return scalar(rtCall(loc, "gaz_stream_state", {b.getI32Type()}, {}), Type::Integer);
    case Expr::BoolLit:
      return scalar(constInt(loc, b.getI1Type(), e.text == "true"), Type::Boolean);
    case Expr::CharLit:
      return scalar(constInt(loc, b.getI8Type(), (unsigned char)e.text[0]), Type::Character);
    case Expr::Var: {
      if (Var *v = lookup(e.text)) return load(loc, v->mems, v->type);
      auto global = globals.find(e.text);
      if (global != globals.end()) return materializeConst(loc, global->second);
      throw err("SymbolError", e.line, "undefined variable '" + e.text + "'");
    }
    case Expr::TupleAccess: {
      if (!lookup(e.text)) {
        auto global = globals.find(e.text);
        if (global == globals.end())
          throw err("SymbolError", e.line, "undefined variable '" + e.text + "'");
        if (!global->second.type.isTuple())
          throw err("TypeError", e.line, "'" + e.text + "' is not a tuple");
        if (e.idx < 1 || static_cast<size_t>(e.idx) > global->second.type.elems.size())
          throw err("IndexError", e.line, "tuple field index is out of range");
        return materializeConst(loc, constElem(global->second, e.idx - 1));
      }
      LV lv = resolveLV(e.text, e.idx, e.line);
      return load(loc, lv.mems, lv.type);
    }
    case Expr::TupleLit: {
      std::vector<Type> ts;
      std::vector<Value> ls;
      for (auto &a : e.args) {
        Val x = genExpr(*a);
        ts.push_back(x.t);
        auto part = leaves(x);
        ls.insert(ls.end(), part.begin(), part.end());
      }
      return fromLeaves(Type::tuple(ts), ls);
    }
    case Expr::Cast: {
      Val x = genExpr(*e.args[0], procOk);
      return cast(loc, e.line, x, e.castTo);
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

  // Name of the variable/field an argument expression denotes, if any.
  static bool lvKey(const Expr &e, std::string &key) {
    if (e.kind == Expr::Var) { key = e.text; return true; }
    if (e.kind == Expr::TupleAccess) { key = e.text + "." + std::to_string(e.idx); return true; }
    return false;
  }
  static bool aliases(const std::string &a, const std::string &b) {
    if (a == b) return true;
    return (a.size() < b.size() && b.compare(0, a.size(), a) == 0 && b[a.size()] == '.') ||
           (b.size() < a.size() && a.compare(0, b.size(), b) == 0 && a[b.size()] == '.');
  }

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

    // Two arguments that may name the same storage, one bound to a var
    // parameter, are an AliasingError.
    for (size_t i = 0; i < args.size(); i++)
      for (size_t j = i + 1; j < args.size(); j++) {
        std::string ki, kj;
        if ((fi.isVar[i] || fi.isVar[j]) && lvKey(*args[i], ki) && lvKey(*args[j], kj) &&
            aliases(ki, kj))
          throw err("AliasingError", line,
                    "arguments '" + ki + "' and '" + kj + "' may alias in call to '" + name + "'");
      }

    std::vector<Value> vals;
    for (size_t i = 0; i < args.size(); i++) {
      if (fi.isVar[i]) {
        const Expr &a = *args[i];
        if (a.kind != Expr::Var && a.kind != Expr::TupleAccess)
          throw err("TypeError", line, "only a variable can be passed to a var parameter");
        LV lv = resolveLV(a.text, a.kind == Expr::Var ? 0 : a.idx, a.line);
        if (lv.isConst)
          throw err("AssignError", line, "const '" + lv.key + "' passed to a var parameter");
        if (lv.type != fi.params[i])
          throw err("TypeError", line,
                    "var parameter needs exactly " + typeName(fi.params[i]) + ", got " + typeName(lv.type));
        vals.insert(vals.end(), lv.mems.begin(), lv.mems.end());
      } else {
        Val x = coerce(loc, line, genExpr(*args[i]), fi.params[i], "argument");
        auto ls = leaves(x);
        vals.insert(vals.end(), ls.begin(), ls.end());
      }
    }
    auto c = mlir::func::CallOp::create(b, loc, "gaz_" + name, valueTypes(fi.ret), vals);
    std::vector<Value> rs(c.getResults().begin(), c.getResults().end());
    return fromLeaves(fi.ret, rs);
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
      return scalar(mlir::arith::XOrIOp::create(b, loc, x.v, constInt(loc, b.getI1Type(), 1)),
                    Type::Boolean);
    }
    if (!x.t.isNumeric())
      throw err("TypeError", e.line, "unary '" + e.text + "' needs a number");
    if (e.text == "+") return x;
    if (x.t == Type::Real) return scalar(mlir::arith::NegFOp::create(b, loc, x.v), Type::Real);
    return scalar(checkedInt(loc, "-", constInt(loc, b.getI32Type(), 0), x.v), Type::Integer);
  }

  // Pairwise equality; members are implicitly cast to a common type.
  Value genEq(Location loc, int line, Val a, Val c) {
    if (a.t.isTuple() || c.t.isTuple()) {
      if (!(a.t.isTuple() && c.t.isTuple()) || a.t.elems.size() != c.t.elems.size())
        throw err("TypeError", line,
                  "cannot compare " + typeName(a.t) + " with " + typeName(c.t));
      Value acc = constInt(loc, b.getI1Type(), 1);
      for (size_t i = 0; i < a.t.elems.size(); i++)
        acc = mlir::arith::AndIOp::create(b, loc, acc, genEq(loc, line, elem(a, i), elem(c, i)));
      return acc;
    }
    if (a.t != c.t) {
      if (a.t == Type::Integer && c.t == Type::Real) a = coerce(loc, line, a, Type::Real, "comparison");
      else if (a.t == Type::Real && c.t == Type::Integer) c = coerce(loc, line, c, Type::Real, "comparison");
      else
        throw err("TypeError", line,
                  "cannot compare " + typeName(a.t) + " with " + typeName(c.t));
    }
    if (a.t == Type::Real)
      return mlir::arith::CmpFOp::create(b, loc, mlir::arith::CmpFPredicate::OEQ, a.v, c.v);
    return mlir::arith::CmpIOp::create(b, loc, mlir::arith::CmpIPredicate::eq, a.v, c.v);
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
      return scalar(v, Type::Boolean);
    }

    if (op == "==" || op == "!=") {
      Value eq = genEq(loc, e.line, l, r);
      if (op == "!=") eq = mlir::arith::XOrIOp::create(b, loc, eq, constInt(loc, b.getI1Type(), 1));
      return scalar(eq, Type::Boolean);
    }

    // remaining operators are numeric
    if (!l.t.isNumeric() || !r.t.isNumeric())
      throw err("TypeError", e.line, "operator '" + op + "' needs numeric operands");
    bool real = l.t == Type::Real || r.t == Type::Real;
    if (real) {
      l = coerce(loc, e.line, l, Type::Real, "expression");
      r = coerce(loc, e.line, r, Type::Real, "expression");
    }

    if (op == "<" || op == ">" || op == "<=" || op == ">=") {
      if (real) {
        FP p = op == "<" ? FP::OLT : op == ">" ? FP::OGT : op == "<=" ? FP::OLE : FP::OGE;
        return scalar(mlir::arith::CmpFOp::create(b, loc, p, l.v, r.v), Type::Boolean);
      }
      P p = op == "<" ? P::slt : op == ">" ? P::sgt : op == "<=" ? P::sle : P::sge;
      return scalar(mlir::arith::CmpIOp::create(b, loc, p, l.v, r.v), Type::Boolean);
    }

    Type rt = real ? Type::Real : Type::Integer;
    mlir::Type i32 = b.getI32Type(), f32 = b.getF32Type();
    Value v;
    if (op == "+")
      v = real ? mlir::arith::AddFOp::create(b, loc, l.v, r.v).getResult() : checkedInt(loc, op, l.v, r.v);
    else if (op == "-")
      v = real ? mlir::arith::SubFOp::create(b, loc, l.v, r.v).getResult() : checkedInt(loc, op, l.v, r.v);
    else if (op == "*")
      v = real ? mlir::arith::MulFOp::create(b, loc, l.v, r.v).getResult() : checkedInt(loc, op, l.v, r.v);
    else if (op == "/")
      v = real ? mlir::arith::DivFOp::create(b, loc, l.v, r.v).getResult()
               : rtCall(loc, "gaz_idiv", {i32}, {l.v, r.v});
    else if (op == "%")
      v = real ? rtCall(loc, "gaz_rmod", {f32}, {l.v, r.v}) : rtCall(loc, "gaz_imod", {i32}, {l.v, r.v});
    else if (op == "^")
      v = real ? rtCall(loc, "gaz_rpow", {f32}, {l.v, r.v}) : rtCall(loc, "gaz_ipow", {i32}, {l.v, r.v});
    else
      throw err("TypeError", e.line, "unknown operator '" + op + "'");
    return scalar(v, rt);
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

  void needIO(const Stmt &s) {
    if (!curIsProc) throw err("StatementError", s.line, "I/O is not allowed in a function");
  }

  void genStmt(const Stmt &s) {
    Location loc = b.getUnknownLoc();
    switch (s.kind) {
    case Stmt::TypeAlias: return; // resolved and scoped while building the AST
    case Stmt::Block: genBlock(s.body); return;
    case Stmt::Decl: {
      if (scopes.back().count(s.name))
        throw err("SymbolError", s.line, "'" + s.name + "' already declared in this scope");
      Type t = s.declType;
      Val init;
      if (s.expr) {
        Val x = genExpr(*s.expr, true);
        if (!s.hasType) t = x.t;
        init = coerce(loc, s.line, x, t, "declaration");
      } else {
        init = zeroVal(loc, t); // uninitialised variables default to zero
      }
      auto mems = newSlots(loc, t);
      store(loc, mems, init);
      scopes.back()[s.name] = {mems, t, s.isConst};
      return;
    }
    case Stmt::Assign: {
      LV lv = resolveLV(s.name, s.idx, s.line);
      if (lv.isConst) throw err("AssignError", s.line, "assignment to const '" + lv.key + "'");
      Val x = coerce(loc, s.line, genExpr(*s.expr, true), lv.type, "assignment");
      store(loc, lv.mems, x);
      return;
    }
    case Stmt::Unpack: {
      Val x = genExpr(*s.expr);
      if (!x.t.isTuple())
        throw err("TypeError", s.line, "cannot unpack " + typeName(x.t));
      if (x.t.elems.size() != s.names.size())
        throw err("AssignError", s.line,
                  "unpacking " + std::to_string(s.names.size()) + " variables from a tuple of " +
                      std::to_string(x.t.elems.size()));
      // The right-hand side is fully evaluated first, so swaps work.
      std::vector<std::pair<LV, Val>> writes;
      for (size_t i = 0; i < s.names.size(); i++) {
        LV lv = resolveLV(s.names[i], 0, s.line);
        if (lv.isConst) throw err("AssignError", s.line, "assignment to const '" + lv.key + "'");
        writes.push_back({lv, coerce(loc, s.line, elem(x, i), lv.type, "unpacking")});
      }
      for (auto &[lv, v] : writes) store(loc, lv.mems, v);
      return;
    }
    case Stmt::Output: {
      needIO(s);
      if (s.expr->kind == Expr::StrLit) {
        for (unsigned char ch : s.expr->text)
          rtCall(loc, "gaz_print_char", {}, {constInt(loc, b.getI8Type(), ch)});
        return;
      }
      Val x = genExpr(*s.expr);
      switch (x.t.kind) {
      case Type::Integer: rtCall(loc, "gaz_print_int", {}, {x.v}); break;
      case Type::Real: rtCall(loc, "gaz_print_real", {}, {x.v}); break;
      case Type::Character: rtCall(loc, "gaz_print_char", {}, {x.v}); break;
      case Type::Boolean:
        rtCall(loc, "gaz_print_bool", {},
               {mlir::arith::ExtUIOp::create(b, loc, b.getI32Type(), x.v)});
        break;
      default: throw err("TypeError", s.line, "cannot print a value of type " + typeName(x.t));
      }
      return;
    }
    case Stmt::Input: {
      needIO(s);
      LV lv = resolveLV(s.name, s.idx, s.line);
      if (lv.isConst) throw err("AssignError", s.line, "input into const '" + lv.key + "'");
      Value r;
      switch (lv.type.kind) {
      case Type::Integer: r = rtCall(loc, "gaz_read_int", {b.getI32Type()}, {}); break;
      case Type::Real: r = rtCall(loc, "gaz_read_real", {b.getF32Type()}, {}); break;
      case Type::Character: r = rtCall(loc, "gaz_read_char", {b.getI8Type()}, {}); break;
      case Type::Boolean:
        r = mlir::arith::CmpIOp::create(
            b, loc, mlir::arith::CmpIPredicate::ne,
            rtCall(loc, "gaz_read_bool", {b.getI32Type()}, {}), constInt(loc, b.getI32Type(), 0));
        break;
      default: throw err("TypeError", s.line, "cannot read into " + typeName(lv.type));
      }
      mlir::memref::StoreOp::create(b, loc, r, lv.mems[0]);
      return;
    }
    case Stmt::CallS: genCall(s.name, s.args, s.line, CallPos::Stmt); return;
    case Stmt::Return: {
      if (curRet == Type::Void) {
        if (s.expr) throw err("TypeError", s.line, "returning a value from a void procedure");
        mlir::func::ReturnOp::create(b, loc, mlir::ValueRange{});
      } else {
        if (!s.expr) throw err("TypeError", s.line, "missing return value");
        Val x = coerce(loc, s.line, genExpr(*s.expr), curRet, "return");
        mlir::func::ReturnOp::create(b, loc, leaves(x));
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
      if (s.loopKind == Stmt::Range) { genRangeLoop(s); return; }
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

  // `loop i in lo..hi stmt`: bounds are evaluated once; `i` is a fresh const
  // each iteration, so the body cannot disturb the iteration.
  void genRangeLoop(const Stmt &s) {
    Location loc = b.getUnknownLoc();
    Val lo = genExpr(*s.expr), hi = genExpr(*s.hi);
    if (lo.t != Type::Integer || hi.t != Type::Integer)
      throw err("TypeError", s.line, "range bounds must be integers");
    Value cur = newSlot(loc, Type::Integer), end = newSlot(loc, Type::Integer);
    mlir::memref::StoreOp::create(b, loc, lo.v, cur);
    mlir::memref::StoreOp::create(b, loc, hi.v, end);
    Block *condB = newBlock(), *bodyB = newBlock(), *stepB = newBlock(),
          *addB = newBlock(), *exitB = newBlock();
    mlir::cf::BranchOp::create(b, loc, condB);

    b.setInsertionPointToStart(condB);
    Value c = mlir::arith::CmpIOp::create(
        b, loc, mlir::arith::CmpIPredicate::sle,
        mlir::memref::LoadOp::create(b, loc, cur), mlir::memref::LoadOp::create(b, loc, end));
    mlir::cf::CondBranchOp::create(b, loc, c, bodyB, exitB);

    // Stop before incrementing past `hi` so INT_MAX bounds cannot wrap.
    b.setInsertionPointToStart(stepB);
    Value last = mlir::arith::CmpIOp::create(
        b, loc, mlir::arith::CmpIPredicate::eq,
        mlir::memref::LoadOp::create(b, loc, cur), mlir::memref::LoadOp::create(b, loc, end));
    mlir::cf::CondBranchOp::create(b, loc, last, exitB, addB);
    b.setInsertionPointToStart(addB);
    Value next = mlir::arith::AddIOp::create(b, loc, mlir::memref::LoadOp::create(b, loc, cur),
                                             constInt(loc, b.getI32Type(), 1));
    mlir::memref::StoreOp::create(b, loc, next, cur);
    mlir::cf::BranchOp::create(b, loc, condB);

    b.setInsertionPointToStart(bodyB);
    scopes.emplace_back();
    Value iv = newSlot(loc, Type::Integer);
    mlir::memref::StoreOp::create(b, loc, mlir::memref::LoadOp::create(b, loc, cur), iv);
    scopes.back()[s.name] = {{iv}, Type::Integer, true};
    loops.push_back({stepB, exitB});
    genBlock(s.body);
    branchIfOpen(stepB);
    loops.pop_back();
    scopes.pop_back();
    b.setInsertionPointToStart(exitB);
  }

  void genFunc(const Func &f) {
    Location loc = b.getUnknownLoc();
    curIsProc = f.isProcedure;
    trapBlock = nullptr;
    if (f.ret != Type::Void && !f.exprBody && !alwaysReturns(f.body))
      throw err("ReturnError", f.line,
                "'" + f.name + "' does not return on every control path");

    // by-value parameters arrive as scalar leaves, var parameters as memrefs
    std::vector<mlir::Type> in;
    for (auto &p : f.params) {
      auto ts = p.isVar ? refTypes(p.type) : valueTypes(p.type);
      in.insert(in.end(), ts.begin(), ts.end());
    }
    b.setInsertionPointToEnd(module->getBody());
    curFn = mlir::func::FuncOp::create(b, loc, "gaz_" + f.name,
                                       b.getFunctionType(in, valueTypes(f.ret)));
    entry = curFn.addEntryBlock();
    b.setInsertionPointToStart(entry);
    curRet = f.ret;

    scopes.clear();
    loops.clear();
    scopes.emplace_back();
    size_t arg = 0;
    for (auto &p : f.params) {
      size_t n = leafCount(p.type);
      std::vector<Value> args;
      for (size_t k = 0; k < n; k++) args.push_back(entry->getArgument(arg + k));
      arg += n;
      if (p.name.empty()) continue;
      if (scopes.back().count(p.name))
        throw err("SymbolError", f.line, "duplicate parameter '" + p.name + "'");
      if (p.isVar) {
        scopes.back()[p.name] = {args, p.type, false}; // by reference
      } else {
        auto mems = newSlots(loc, p.type);
        store(loc, mems, fromLeaves(p.type, args));
        scopes.back()[p.name] = {mems, p.type, true};
      }
    }

    if (f.exprBody) {
      Val x = coerce(loc, f.line, genExpr(*f.exprBody), f.ret, "function body");
      mlir::func::ReturnOp::create(b, loc, leaves(x));
    } else {
      genBlock(f.body);
      if (!terminated()) mlir::func::ReturnOp::create(b, loc, leaves(zeroVal(loc, f.ret)));
    }
    // Terminate any leftover unreachable blocks.
    for (Block &blk : curFn.getBody()) {
      if (!blk.empty() && blk.back().hasTrait<mlir::OpTrait::IsTerminator>()) continue;
      b.setInsertionPointToEnd(&blk);
      mlir::func::ReturnOp::create(b, loc, leaves(zeroVal(loc, f.ret)));
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

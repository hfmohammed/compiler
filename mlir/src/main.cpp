#include "MLIRGen.h"
#include "Parser.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/Verifier.h"
#include <fstream>
#include <iostream>
#include <sstream>

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "usage: gazc <input.gaz>\n";
    return 1;
  }
  std::ifstream in(argv[1]);
  std::stringstream ss;
  ss << in.rdbuf();

  mlir::MLIRContext ctx;
  ctx.loadDialect<mlir::arith::ArithDialect, mlir::cf::ControlFlowDialect,
                  mlir::func::FuncDialect, mlir::memref::MemRefDialect>();
  try {
    auto prog = gaz::parse(gaz::lex(ss.str()));
    auto module = gaz::genMLIR(ctx, prog);
    if (mlir::failed(mlir::verify(*module))) return 1;
    module->print(llvm::outs());
  } catch (std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
}

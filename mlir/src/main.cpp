#include "Lowering.h"
#include "MLIRGen.h"
#include "Parser.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/Verifier.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

static void usage() {
  std::cerr << "usage: gazc [-O2] [--emit-mlir | --emit-llvm] <input.gaz> [-o <output>]\n"
               "  -O2           run LLVM's O2 optimization pipeline\n"
               "  --emit-mlir   print the high-level MLIR (before lowering)\n"
               "  --emit-llvm   print LLVM IR (default)\n";
}

int main(int argc, char **argv) {
  enum { Mlir, Llvm } mode = Llvm;
  bool optimize = false;
  const char *input = nullptr, *output = nullptr;
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-O2")) optimize = true;
    else if (!strcmp(argv[i], "--emit-mlir")) mode = Mlir;
    else if (!strcmp(argv[i], "--emit-llvm")) mode = Llvm;
    else if (!strcmp(argv[i], "-o") && i + 1 < argc) output = argv[++i];
    else if (argv[i][0] != '-' && !input) input = argv[i];
    else { usage(); return 2; }
  }
  if (!input) { usage(); return 2; }

  std::ifstream in(input);
  if (!in) {
    std::cerr << "gazc: cannot open " << input << "\n";
    return 2;
  }
  std::stringstream ss;
  ss << in.rdbuf();

  mlir::MLIRContext ctx;
  ctx.loadDialect<mlir::arith::ArithDialect, mlir::cf::ControlFlowDialect,
                  mlir::func::FuncDialect, mlir::memref::MemRefDialect,
                  mlir::LLVM::LLVMDialect>();

  std::error_code ec;
  llvm::raw_fd_ostream fileOut(output ? output : "-", ec);
  if (ec) {
    std::cerr << "gazc: cannot write " << output << "\n";
    return 2;
  }

  try {
    auto prog = gaz::parse(ss.str());
    auto module = gaz::genMLIR(ctx, prog);
    if (mlir::failed(mlir::verify(*module))) return 3;
    if (mode == Mlir) {
      module->print(fileOut);
      return 0;
    }
    if (!gaz::lowerToLLVMDialect(*module)) {
      std::cerr << "gazc: lowering to LLVM failed\n";
      return 3;
    }
    llvm::LLVMContext llvmCtx;
    auto llvmModule = gaz::translateToLLVMIR(*module, llvmCtx);
    if (!llvmModule) {
      std::cerr << "gazc: LLVM IR translation failed\n";
      return 3;
    }
    if (optimize) gaz::optimizeLLVMModule(*llvmModule);
    llvmModule->print(fileOut, nullptr);
  } catch (std::exception &e) {
    std::cerr << e.what() << "\n";
    return 1;
  }
  return 0;
}

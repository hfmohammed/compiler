#pragma once
#include "mlir/IR/BuiltinOps.h"
#include "llvm/IR/LLVMContext.h"
#include <memory>

namespace gaz {

// Converts func/arith/cf/memref to the LLVM dialect in place.
bool lowerToLLVMDialect(mlir::ModuleOp module);

// Translates an LLVM-dialect module to LLVM IR.
std::unique_ptr<llvm::Module> translateToLLVMIR(mlir::ModuleOp module,
                                                llvm::LLVMContext &llvmCtx);

} // namespace gaz

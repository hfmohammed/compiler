#pragma once
#include "AST.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/OwningOpRef.h"

namespace gaz {

// Lowers a parsed Gazprea program to an MLIR module made of func, arith, cf
// and memref operations. Throws std::runtime_error("TypeError ..." /
// "SymbolError ...") on semantic errors.
mlir::OwningOpRef<mlir::ModuleOp> genMLIR(mlir::MLIRContext &ctx,
                                          const Program &prog);

} // namespace gaz
